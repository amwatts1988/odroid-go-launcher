#include "freertos/FreeRTOS.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"
#include "driver/gpio.h"

#include "../components/hardware/display.h"
#include "../components/hardware/backlight.h"
#include "../components/hardware/keypad.h"

#include "gbuf.h"
#include "tf.h"
#include "OpenSans_Regular_11X12.h"

#include "image_cone.h"

#include <stdint.h>
#include <string.h>

void app_main(void)
{
    struct gbuf *fb = display_init();
    nvs_flash_init();
    backlight_init();
    keypad_init();

    struct tf *tf = tf_new();
    tf->font = &font_OpenSans_Regular_11X12;

    uint16_t sample;
    char s[128];

    uint64_t time_last, time_now = 0;
    uint32_t time_delta;
    float fps;

    while (true) {
        time_last = time_now;
        time_now = esp_timer_get_time();
        time_delta = time_now - time_last;
        fps = 1000000.0 / time_delta;

        memset(fb->pixel_data, 0, fb->width * fb->height * fb->bytes_per_pixel);
        short x = DISPLAY_WIDTH/2 - image_cone.width/2;
        short y = DISPLAY_HEIGHT/2 - image_cone.height/2;
        gbuf_blit_gimp_image(fb, (struct gimp_image *)&image_cone, x, y);

        sprintf(s, "FPS: %0.2f", fps);
        tf_draw_str(fb, tf, s, 250, 12);

        sample = keypad_sample();

        sprintf(s, "Up: %s", sample & KEYPAD_UP ? "DOWN" : "UP");
        tf_draw_str(fb, tf, s, 12, 12);

        sprintf(s, "Right: %s", sample & KEYPAD_RIGHT ? "DOWN" : "UP");
        tf_draw_str(fb, tf, s, 12, 24);

        sprintf(s, "Down: %s", sample & KEYPAD_DOWN ? "DOWN" : "UP");
        tf_draw_str(fb, tf, s, 12, 36);

        sprintf(s, "Left: %s", sample & KEYPAD_LEFT ? "DOWN" : "UP");
        tf_draw_str(fb, tf, s, 12, 48);

        sprintf(s, "Select: %s", sample & KEYPAD_SELECT ? "DOWN" : "UP");
        tf_draw_str(fb, tf, s, 12, 60);

        sprintf(s, "Start: %s", sample & KEYPAD_START ? "DOWN" : "UP");
        tf_draw_str(fb, tf, s, 12, 72);

        sprintf(s, "A: %s", sample & KEYPAD_A ? "DOWN" : "UP");
        tf_draw_str(fb, tf, s, 12, 84);

        sprintf(s, "B: %s", sample & KEYPAD_B ? "DOWN" : "UP");
        tf_draw_str(fb, tf, s, 12, 96);

        sprintf(s, "Menu: %s", sample & KEYPAD_MENU ? "DOWN" : "UP");
        tf_draw_str(fb, tf, s, 12, 108);

        sprintf(s, "Volume: %s", sample & KEYPAD_VOLUME ? "DOWN" : "UP");
        tf_draw_str(fb, tf, s, 12, 120);

        display_draw();
    }
}
