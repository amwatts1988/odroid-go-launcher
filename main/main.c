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
#include "wifi.h"

#include "app.h"
#include "graphics.h"
#include "tf.h"
#include "OpenSans_Regular_11X12.h"
#include "periodic.h"
#include "statusbar.h"
#include "ui_dialog.h"


void app_main(void)
{
    QueueHandle_t keypad;

    display_init();
    backlight_init();

    tf_t *tf = tf_new(&font_OpenSans_Regular_11X12, 0xFFFF, 240, TF_ALIGN_CENTER | TF_WORDWRAP);
    const char *s;
    tf_metrics_t m;
    point_t p;

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

    keypad = keypad_get_queue();

    s = "Press Menu button for the menu.";
    m = tf_get_str_metrics(tf, s);
    p.x = fb->width/2 - tf->width/2;
    p.y = fb->height/2 - m.height/2;
    memset(fb->data + fb->width * 16 * fb->bytes_per_pixel, 0, fb->width * (fb->height - 32) * fb->bytes_per_pixel);
    tf_draw_str(fb, tf, s, p);
    display_update();

    while (true) {
        keypad_info_t keys;
        while (true) {
            if (keypad_queue_receive(keypad, &keys, 50/portTICK_RATE_MS)) {
                if (keys.pressed & KEYPAD_MENU) {
                    break;
                }
            }
            periodic_tick();
        }

        rect_t r = {
            .x = DISPLAY_WIDTH/2 - 240/2,
            .y = DISPLAY_HEIGHT/2 - 180/2,
            .width = 240,
            .height = 180,
        };
        
        ui_dialog_t *d = ui_dialog_new(NULL, r, "The quick brown fox jumps over the lazy dog.");
        d->keypad = keypad;
        rect_t lr = {
            .x = 0,
            .y = 0,
            .width = 240 - 4,
            .height = 16,
        };
        ui_dialog_add_button(d, lr, "button 1", NULL);

        lr.y += 17;
        lr.width = 50;
        ui_dialog_add_label(d, lr, "a label");

        lr.x = 60;
        ui_dialog_add_button(d, lr, "button2", NULL);

        lr.x = 120;
        ui_dialog_add_button(d, lr, "button 3", NULL);

        lr.x = 0;
        lr.y += 17;
        lr.width = 240 - 4;
        ui_dialog_add_edit(d, lr, "edit 1", 64);

        lr.y += 17;
        lr.height = 80;
        ui_list_t *list = ui_dialog_add_list(d, lr);
        ui_list_append_text(list, "This is some text", NULL);
        ui_list_append_text(list, "This is also some text", NULL);
        ui_list_append_text(list, "Line 3", NULL);
        ui_list_append_text(list, "Line 4", NULL);
        ui_list_append_text(list, "Line 5", NULL);
        ui_list_append_separator(list);
        ui_list_append_text(list, "Line 6", NULL);
        ui_list_append_text(list, "Line 7", NULL);
        ui_list_append_text(list, "Line 8", NULL);

        ui_dialog_showmodal(d);
        ui_dialog_destroy(d);
    }
}
