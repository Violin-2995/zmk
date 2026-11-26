/* SPDX-License-Identifier: MIT
 * PSKeeb5 right central, V3.0-alpha.4.
 * Dedicated encrypted BLE service; does not use Studio RPC or HID reports.
 */
#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/settings/settings.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/logging/log.h>
#include <errno.h>
#include <zmk/yuan_special.h>
#include <zmk/keymap.h>
#include <zmk/ble.h>
#include <zmk/endpoints.h>
#include <zmk/event_manager.h>
#include <zmk/events/endpoint_changed.h>
#include <zmk/events/ble_active_profile_changed.h>
#include <zmk/split/central.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);
BUILD_ASSERT(ZMK_KEYMAP_LAYERS_LEN <= YUAN_MAX_LAYERS);
BUILD_ASSERT(IS_ENABLED(CONFIG_SETTINGS));
BUILD_ASSERT(sizeof(struct zmk_sync_layer_state) == 20);
BUILD_ASSERT(YUAN_MAX_LAYERS * YUAN_CATALOG_RECORD <= 512);

#define YUAN_UUID(n) BT_UUID_128_ENCODE(0x9e84a100 + (n), 0x65b3, 0x4baf, \
                                      0x99a2, 0x5926d03c1e07ULL)
#define YUAN_SETTINGS "yuan_v3/flags"
#define YUAN_ACK_MS 1800
#define YUAN_SYNC_MS 1000

static uint8_t prefs[YUAN_MAX_LAYERS];
static bool dirty, save_failed;
static uint16_t token = 1;
static int64_t ack_until;
static bool snapshot_valid;
K_MUTEX_DEFINE(yuan_mutex);

struct snapshot {
    uint32_t available, active, special, english, catalog_hash;
    uint8_t display_id, index, transport, profile;
    char name[YUAN_NAME_LEN + 1];
};
static struct snapshot current;
static uint8_t catalog[YUAN_MAX_LAYERS * YUAN_CATALOG_RECORD];

static void invalidate_locked(void) {
    if (++token == 0) token = 1;
    ack_until = 0;
}

static struct snapshot capture_locked(void) {
    struct snapshot s;
    memset(&s, 0, sizeof(s));
    uint8_t order[ZMK_KEYMAP_LAYERS_LEN];
    uint32_t active = 0;
    const uint32_t state = zmk_keymap_layer_state() | yuan_bit(zmk_keymap_layer_default());
    s.catalog_hash = UINT32_C(2166136261);
    for (size_t i = 0; i < ZMK_KEYMAP_LAYERS_LEN; ++i) {
        uint8_t id = zmk_keymap_layer_index_to_id(i);
        order[i] = id;
        s.catalog_hash = (s.catalog_hash ^ id) * UINT32_C(16777619);
        if (id < ZMK_KEYMAP_LAYERS_LEN) {
            char name[YUAN_NAME_LEN + 1];
            yuan_name_copy(name, zmk_keymap_layer_name(id));
            for (size_t j = 0; j < sizeof(name); ++j)
                s.catalog_hash = (s.catalog_hash ^ (uint8_t)name[j]) * UINT32_C(16777619);
            if (state & yuan_bit(id)) active |= yuan_bit(id);
        }
    }
    struct yuan_policy p = yuan_select(order, ARRAY_SIZE(order), active, prefs);
    s.available = p.available; s.active = p.active;
    s.special = p.special; s.english = p.english;
    s.display_id = p.display_id;
    s.index = zmk_keymap_highest_layer_active();
    struct zmk_endpoint_instance ep = zmk_endpoint_get_selected();
    s.transport = (uint8_t)ep.transport;
    s.profile = ep.transport == ZMK_TRANSPORT_BLE ? (uint8_t)ep.ble.profile_index : 255;
    if (s.display_id < ZMK_KEYMAP_LAYERS_LEN)
        yuan_name_copy(s.name, zmk_keymap_layer_name(s.display_id));
    return s;
}

static void refresh_locked(void) {
    struct snapshot s = capture_locked();
    if (!snapshot_valid || current.available != s.available ||
        current.active != s.active || current.special != s.special ||
        current.english != s.english || current.catalog_hash != s.catalog_hash ||
        current.display_id != s.display_id || current.index != s.index ||
        current.transport != s.transport || current.profile != s.profile ||
        memcmp(current.name, s.name, sizeof(s.name))) {
        current = s;
        snapshot_valid = true;
        invalidate_locked();
    }
}

static bool confirmed_locked(void) {
    return current.transport == ZMK_TRANSPORT_BLE &&
           (current.active & current.english) && ack_until > k_uptime_get();
}

/* Match the actual selected output connection, not just any bonded computer.
 * zmk_ble_active_profile_conn() returns a referenced connection. */
static bool selected_peer(struct bt_conn *conn) {
    struct zmk_endpoint_instance ep = zmk_endpoint_get_selected();
    if (!conn || ep.transport != ZMK_TRANSPORT_BLE) return false;
    struct bt_conn *selected = zmk_ble_active_profile_conn();
    bool match = selected != NULL && selected == conn;
    if (selected) bt_conn_unref(selected);
    return match;
}

struct zmk_sync_layer_state yuan_build_sync(uint8_t index) {
    struct zmk_sync_layer_state out = {.index = index, .yuan_magic = YUAN_MAGIC,
                                       .yuan_id = YUAN_INVALID_ID};
    k_mutex_lock(&yuan_mutex, K_FOREVER);
    refresh_locked();
    struct yuan_policy p = {.available = current.available, .active = current.active,
                            .special = current.special, .english = current.english,
                            .display_id = current.display_id};
    out.yuan_id = current.display_id;
    out.yuan_flags = yuan_screen_flags(p, confirmed_locked());
    memcpy(out.yuan_name, current.name, sizeof(out.yuan_name));
    k_mutex_unlock(&yuan_mutex);
    return out;
}

static int yuan_load(const char *name, size_t len, settings_read_cb read_cb, void *arg) {
    const char *next;
    if (!settings_name_steq(name, "flags", &next) || next) return -ENOENT;
    if (len != sizeof(prefs)) return -EINVAL;
    uint8_t buf[YUAN_MAX_LAYERS];
    int rc = read_cb(arg, buf, sizeof(buf));
    if (rc != sizeof(buf)) return rc < 0 ? rc : -EINVAL;
    for (size_t i = 0; i < ARRAY_SIZE(buf); ++i)
        if (buf[i] & ~YUAN_VALID_PREFS) return -EINVAL;
    k_mutex_lock(&yuan_mutex, K_FOREVER);
    memcpy(prefs, buf, sizeof(prefs));
    snapshot_valid = false;
    invalidate_locked();
    k_mutex_unlock(&yuan_mutex);
    return 0;
}
SETTINGS_STATIC_HANDLER_DEFINE(yuan_v3, "yuan_v3", NULL, yuan_load, NULL, NULL);

static void save_work_cb(struct k_work *work);
static K_WORK_DEFINE(save_work, save_work_cb);
static void save_work_cb(struct k_work *work) {
    (void)work;
    uint8_t copy[YUAN_MAX_LAYERS];
    k_mutex_lock(&yuan_mutex, K_FOREVER);
    memcpy(copy, prefs, sizeof(copy));
    k_mutex_unlock(&yuan_mutex);
    int rc = settings_save_one(YUAN_SETTINGS, copy, sizeof(copy));
    k_mutex_lock(&yuan_mutex, K_FOREVER);
    bool changed = memcmp(copy, prefs, sizeof(copy)) != 0;
    save_failed = rc != 0;
    dirty = changed || save_failed;
    k_mutex_unlock(&yuan_mutex);
    if (rc) LOG_ERR("YUAN preferences save failed: %d", rc);
    if (changed) k_work_submit(&save_work);
}
static void update_pref_locked(uint8_t id, uint8_t value) {
    if (prefs[id] != value) {
        prefs[id] = value;
        dirty = true;
        invalidate_locked();
        snapshot_valid = false;
    }
    if (dirty) k_work_submit(&save_work);
}
void yuan_forget_layer(uint8_t id) {
    if (id >= YUAN_MAX_LAYERS) return;
    k_mutex_lock(&yuan_mutex, K_FOREVER);
    update_pref_locked(id, 0);
    k_mutex_unlock(&yuan_mutex);
}
void yuan_forget_all(void) {
    k_mutex_lock(&yuan_mutex, K_FOREVER);
    memset(prefs, 0, sizeof(prefs));
    dirty = true;
    snapshot_valid = false;
    invalidate_locked();
    k_work_submit(&save_work);
    k_mutex_unlock(&yuan_mutex);
}

static ssize_t read_status(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                           void *buf, uint16_t len, uint16_t offset) {
    uint8_t wire[YUAN_STATUS_SIZE] = {YUAN_PROTOCOL};
    k_mutex_lock(&yuan_mutex, K_FOREVER);
    refresh_locked();
    if (selected_peer(conn)) {
        wire[1] = 1u | (dirty ? 2u : 0u) | (confirmed_locked() ? 4u : 0u) |
                  (save_failed ? 8u : 0u);
        sys_put_le16(token, wire + 2);
        sys_put_le32(current.active, wire + 4);
        sys_put_le32(current.available, wire + 8);
        sys_put_le32(current.special, wire + 12);
        sys_put_le32(current.english, wire + 16);
    }
    k_mutex_unlock(&yuan_mutex);
    return bt_gatt_attr_read(conn, attr, buf, len, offset, wire, sizeof(wire));
}

/* Protocol 2: 32 fixed 16-byte records, 512 bytes (ATT attribute limit).
 * ID is implicit in record slot. Byte 0 is order; next 15 bytes are UTF-8.
 * Host validates long reads against the status token/available mask. */
static ssize_t read_catalog(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                            void *buf, uint16_t len, uint16_t offset) {
    k_mutex_lock(&yuan_mutex, K_FOREVER);
    if (!selected_peer(conn)) {
        k_mutex_unlock(&yuan_mutex);
        return BT_GATT_ERR(BT_ATT_ERR_AUTHORIZATION);
    }
    memset(catalog, 0, sizeof(catalog));
    for (uint8_t id = 0; id < YUAN_MAX_LAYERS; ++id) {
        uint8_t *rec = catalog + id * YUAN_CATALOG_RECORD;
        rec[0] = YUAN_INVALID_ID;
    }
    for (uint8_t i = 0; i < ZMK_KEYMAP_LAYERS_LEN; ++i) {
        uint8_t id = zmk_keymap_layer_index_to_id(i);
        if (id >= ZMK_KEYMAP_LAYERS_LEN) continue;
        uint8_t *rec = catalog + id * YUAN_CATALOG_RECORD;
        yuan_catalog_record_write(rec, i, zmk_keymap_layer_name(id));
    }
    ssize_t rc = bt_gatt_attr_read(conn, attr, buf, len, offset, catalog, sizeof(catalog));
    k_mutex_unlock(&yuan_mutex);
    return rc;
}

/* Write with response. [version,op,token_le16,id,flags] for preference;
 * [version,2,token_le16,confirmed] for a short-lived host acknowledgment.
 * No keymap edits and no flash writes on acknowledgments. */
static ssize_t write_cmd(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                         const void *buf, uint16_t len, uint16_t offset, uint8_t flags) {
    (void)attr;
    if (offset || (flags & BT_GATT_WRITE_FLAG_PREPARE))
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
    if (len != 5 && len != 6) return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
    const uint8_t *b = buf;
    if (b[0] != YUAN_PROTOCOL) return BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
    k_mutex_lock(&yuan_mutex, K_FOREVER);
    refresh_locked();
    ssize_t rc = len;
    if (!selected_peer(conn)) {
        rc = BT_GATT_ERR(BT_ATT_ERR_AUTHORIZATION);
    } else if (sys_get_le16(b + 2) != token) {
        rc = BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
    } else if (b[1] == 1 && len == 6 && b[4] < YUAN_MAX_LAYERS &&
               (current.available & yuan_bit(b[4])) && !(b[5] & ~YUAN_VALID_PREFS)) {
        update_pref_locked(b[4], b[5]);
    } else if (b[1] == 2 && len == 5 && b[4] <= 1) {
        ack_until = b[4] && (current.active & current.english) ?
                    k_uptime_get() + YUAN_ACK_MS : 0;
    } else {
        rc = BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
    }
    k_mutex_unlock(&yuan_mutex);
    return rc;
}
BT_GATT_SERVICE_DEFINE(yuan_service,
    BT_GATT_PRIMARY_SERVICE(BT_UUID_DECLARE_128(YUAN_UUID(0))),
    BT_GATT_CHARACTERISTIC(BT_UUID_DECLARE_128(YUAN_UUID(1)), BT_GATT_CHRC_READ,
                           BT_GATT_PERM_READ_ENCRYPT, read_status, NULL, NULL),
    BT_GATT_CHARACTERISTIC(BT_UUID_DECLARE_128(YUAN_UUID(2)), BT_GATT_CHRC_READ,
                           BT_GATT_PERM_READ_ENCRYPT, read_catalog, NULL, NULL),
    BT_GATT_CHARACTERISTIC(BT_UUID_DECLARE_128(YUAN_UUID(3)), BT_GATT_CHRC_WRITE,
                           BT_GATT_PERM_WRITE_ENCRYPT, NULL, write_cmd, NULL)
);

static int endpoint_changed(const zmk_event_t *eh) {
    (void)eh;
    k_mutex_lock(&yuan_mutex, K_FOREVER);
    invalidate_locked();
    snapshot_valid = false;
    k_mutex_unlock(&yuan_mutex);
    return ZMK_EV_EVENT_BUBBLE;
}
ZMK_LISTENER(yuan_endpoint, endpoint_changed);
ZMK_SUBSCRIPTION(yuan_endpoint, zmk_endpoint_changed);
ZMK_SUBSCRIPTION(yuan_endpoint, zmk_ble_active_profile_changed);

static void on_disconnect(struct bt_conn *conn, uint8_t reason) {
    (void)conn; (void)reason;
    k_mutex_lock(&yuan_mutex, K_FOREVER);
    invalidate_locked();
    snapshot_valid = false;
    k_mutex_unlock(&yuan_mutex);
}
BT_CONN_CB_DEFINE(yuan_conn_cb) = {.disconnected = on_disconnect};

static void sync_work_cb(struct k_work *work);
static K_WORK_DELAYABLE_DEFINE(sync_work, sync_work_cb);
static void sync_work_cb(struct k_work *work) {
    (void)work;
    static struct zmk_sync_layer_state previous;
    static bool initialized;
    static int64_t last_sent;
    static uint8_t repeats;
    int64_t now = k_uptime_get();
    struct zmk_sync_layer_state s = yuan_build_sync(zmk_keymap_highest_layer_active());
    bool changed = !initialized || memcmp(&s, &previous, sizeof(s));
    bool heartbeat = (s.yuan_flags & YUAN_SPECIAL) && now - last_sent >= YUAN_SYNC_MS;
    if (changed) repeats = 3;
    if (changed || heartbeat || repeats) {
        if (zmk_split_central_sync_layer(NULL) == 0) {
            previous = s;
            initialized = true;
            last_sent = now;
            if (repeats) --repeats;
        }
    }
    k_work_reschedule(&sync_work, K_MSEC(400));
}
static int yuan_init(void) {
    k_work_schedule(&sync_work, K_MSEC(1200));
    return 0;
}
SYS_INIT(yuan_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
