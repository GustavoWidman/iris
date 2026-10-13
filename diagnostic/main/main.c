#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "services/gap/ble_svc_gap.h"
static const char *TAG = "BJ_DIAG";
static uint8_t addr_type;
static uint16_t conn = BLE_HS_CONN_HANDLE_NONE, handle;
static void scan(void);
static int event(struct ble_gap_event *e, void *arg);

static int
chars(uint16_t c, const struct ble_gatt_error *err, const struct ble_gatt_chr *chr, void *arg) {
    if (err->status == 0) {
        char uuid[BLE_UUID_STR_LEN];
        ble_uuid_to_str(&chr->uuid.u, uuid);
        ESP_LOGI(
            TAG, "GATT uuid=%s handle=%u props=0x%02x", uuid, chr->val_handle, chr->properties);
        if (ble_uuid_cmp(&chr->uuid.u, BLE_UUID16_DECLARE(0xee01)) == 0) {
            handle = chr->val_handle;
            ESP_LOGI(TAG, "FOUND EE01, ready for serial commands: on, off, rgb R G B, disconnect");
        }
    } else
        ESP_LOGI(
            TAG, "Characteristic discovery complete status=%d ready=%d", err->status, handle != 0);
    return 0;
}

static int event(struct ble_gap_event *e, void *arg) {
    if (e->type == BLE_GAP_EVENT_DISC) {
        struct ble_hs_adv_fields f;
        if (ble_hs_adv_parse_fields(&f, e->disc.data, e->disc.length_data) != 0)
            return 0;
        if (f.name_len) {
            char name[64];
            snprintf(name, sizeof(name), "%.*s", f.name_len, f.name);
            ESP_LOGI(TAG,
                     "ADV name=%s addr=%02x:%02x:%02x:%02x:%02x:%02x type=%u RSSI=%d",
                     name,
                     e->disc.addr.val[5],
                     e->disc.addr.val[4],
                     e->disc.addr.val[3],
                     e->disc.addr.val[2],
                     e->disc.addr.val[1],
                     e->disc.addr.val[0],
                     e->disc.addr.type,
                     e->disc.rssi);
            if (!strcmp(name, "BJ_LED")) {
                ble_gap_disc_cancel();
                int rc = ble_gap_connect(addr_type, &e->disc.addr, 10000, NULL, event, NULL);
                ESP_LOGI(TAG, "BJ_LED connect requested rc=%d", rc);
            }
        }
    } else if (e->type == BLE_GAP_EVENT_CONNECT) {
        ESP_LOGI(TAG, "CONNECT status=%d", e->connect.status);
        if (!e->connect.status) {
            conn = e->connect.conn_handle;
            ble_gattc_disc_all_chrs(conn, 1, 65535, chars, NULL);
        } else
            scan();
    } else if (e->type == BLE_GAP_EVENT_DISCONNECT) {
        ESP_LOGI(TAG, "DISCONNECT reason=%d", e->disconnect.reason);
        conn = BLE_HS_CONN_HANDLE_NONE;
        handle = 0;
        scan();
    } else if (e->type == BLE_GAP_EVENT_DISC_COMPLETE) {
        ESP_LOGI(TAG, "Scan complete, retrying");
        scan();
    }
    return 0;
}

static void scan(void) {
    struct ble_gap_disc_params p = {0};
    p.passive = 0;
    p.itvl = 160;
    p.window = 80;
    p.filter_duplicates = 1;
    int rc = ble_gap_disc(addr_type, 20000, &p, event, NULL);
    ESP_LOGI(TAG, "scan rc=%d", rc);
}

static void synced(void) {
    ble_hs_id_infer_auto(0, &addr_type);
    scan();
}

static void host(void *p) {
    nimble_port_run();
    nimble_port_freertos_deinit();
}

static void commands(void *p) {
    char line[80];
    size_t n = 0;
    for (;;) {
        int c = getchar();
        if (c == EOF) {
            vTaskDelay(pdMS_TO_TICKS(20));
            continue;
        }
        if (c == '\n' || c == '\r') {
            line[n] = 0;
            n = 0;
            unsigned r, g, b;
            uint8_t packet[7];
            int len = 0;
            if (!strcmp(line, "on") || !strcmp(line, "off")) {
                uint8_t a[] = {0x69, 0x96, 2, 1, !strcmp(line, "on")};
                memcpy(packet, a, 5);
                len = 5;
            } else if (sscanf(line, "rgb %u %u %u", &r, &g, &b) == 3 && r <= 255 && g <= 255 &&
                       b <= 255) {
                uint8_t a[] = {0x69, 0x96, 5, 2, r, g, b};
                memcpy(packet, a, 7);
                len = 7;
            } else if (!strcmp(line, "disconnect")) {
                if (conn != BLE_HS_CONN_HANDLE_NONE)
                    ble_gap_terminate(conn, BLE_ERR_REM_USER_CONN_TERM);
            }
            if (len) {
                int rc = (handle && conn != BLE_HS_CONN_HANDLE_NONE)
                             ? ble_gattc_write_no_rsp_flat(conn, handle, packet, len)
                             : -1;
                ESP_LOGI(TAG,
                         "WRITE bytes=%d rc=%d (transport only, no device state feedback)",
                         len,
                         rc);
            }
        } else if (n < sizeof(line) - 1)
            line[n++] = c;
    }
}

void app_main(void) {
    esp_err_t e = nvs_flash_init();
    if (e == ESP_ERR_NVS_NO_FREE_PAGES || e == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }
    ESP_ERROR_CHECK(nimble_port_init());
    ble_hs_cfg.sync_cb = synced;
    ble_svc_gap_device_name_set("BJ diagnostic");
    nimble_port_freertos_init(host);
    xTaskCreate(commands, "commands", 4096, NULL, 5, NULL);
    ESP_LOGI(TAG, "BJ_LED diagnostic ready, no automatic LED writes");
}
