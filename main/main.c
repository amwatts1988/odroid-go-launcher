#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "esp_err.h"
#include "esp_spiffs.h"

#include "display.h"
#include "backlight.h"
#include "keypad.h"
#include "sdcard.h"

#include "app.h"
#include "graphics.h"
#include "menu.h"
#include "menu_app.h"
#include "menu_wifi.h"
#include "tf.h"
#include "OpenSans_Regular_11X12.h"
#include "statusbar.h"
#include "wifi.h"


void app_main(void)
{
    display_init();
    backlight_init();

    struct tf_t *tf = tf_new(&font_OpenSans_Regular_11X12, 240, TF_ALIGN_CENTER | TF_WORDWRAP);
    const char *s;
    struct tf_metrics_t m;
    struct point_t p;

    s = "Initializing...";
    m = tf_get_str_metrics(tf, s);
    p.x = DISPLAY_WIDTH/2 - tf->width/2;
    p.y = DISPLAY_HEIGHT/2 - m.height/2;
    tf_draw_str(fb, tf, s, p);
    display_update();

    keypad_init();
    ESP_ERROR_CHECK(nvs_flash_init());
    wifi_init();
    sdcard_init("/sdcard");

    esp_vfs_spiffs_conf_t conf = {
      .base_path = "/spiffs",
      .partition_label = NULL,
      .max_files = 5,
      .format_if_mount_failed = true,
    };

    ESP_ERROR_CHECK(esp_vfs_spiffs_register(&conf));
    statusbar_init();

    s = "Press Menu button for the menu.";
    m = tf_get_str_metrics(tf, s);
    p.x = fb->width/2 - tf->width/2;
    p.y = fb->height/2 - m.height/2;
    memset(fb->data + fb->width * 16 * fb->bytes_per_pixel, 0, fb->width * (fb->height - 32) * fb->bytes_per_pixel);
    tf_draw_str(fb, tf, s, p);
    display_update();

    while (true) {
        uint16_t keys = 0, changes = 0, pressed = 0;
        do {
            vTaskDelay(10 / portTICK_PERIOD_MS);
            keys = keypad_debounce(keypad_sample(), &changes);
            pressed = keys & changes;
            statusbar_update();
        } while (!(pressed & KEYPAD_MENU));

        struct rect_t r = {
            .x = fb->width/2 - 240/2,
            .y = fb->height/2 - 180/2,
            .width = 240,
            .height = 180,
        };

        struct menu_t *menu = menu_new(r, NULL);
        menu_append_text(menu, "WiFi Settings", wifi_menu_fn, NULL);
        menu_append_text(menu, "Manage Apps", app_menu_fn, NULL);
        menu_showmodal(menu);
        menu_free(menu);
    }
}
