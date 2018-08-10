#include <math.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "keypad.h"
#include "display.h"
#include "gbuf.h"

#include "dialog.h"
#include "OpenSans_Regular_11X12.h"
#include "statusbar.h"
#include "tf.h"


dialog_t *dialog_new(rect_t r, const char *title)
{
    dialog_t *d = calloc(1, sizeof(dialog_t));
    memcpy(&d->r, &r, sizeof(rect_t));
    d->g = gbuf_new(r.width, r.height, fb->bytes_per_pixel, fb->endian);
    if (title) {
        d->title = strdup(title);
    }
    return d;
}

void dialog_destroy(dialog_t *d)
{
    gbuf_free(d->g);
    free(d);
}

void ui_dialog_draw(dialog_t *d)
{
    assert(d->visible);

    d->cr.x = d->r.x + 2;
    d->cr.y = d->r.y + 2;
    d->cr.width = d->r.width - 4;
    d->cr.height = d->r.height - 4;

    fill_rectangle(fb, d->r, 0x0000);
    draw_rectangle(fb, d->r, DRAW_STYLE_SOLID, 0xFFFF);

    if (d->title) {
        tf_t *tf = tf_new(&font_OpenSans_Regular_11X12, d->cr.width, TF_ALIGN_CENTER | TF_ELIDE);
        tf_metrics_t m = tf_get_str_metrics(tf, d->title);
        rect_t r = {
            .x = d->r.x,
            .y = d->r.y,
            .width = d->r.width,
            .height = m.height + 6,
        };
        fill_rectangle(fb, r, 0x001F);
        draw_rectangle(fb, r, DRAW_STYLE_SOLID, 0xFFFF);

        point_t p = {
            .x = d->r.x + 2,
            .y = d->r.y + 3,
        };
        tf_draw_str(fb, tf, d->title, p);

        d->cr.y += m.height + 5;
        d->cr.height -= m.height + 5;
    }
}

void dialog_showmodal(dialog_t *d)
{
    d->close = false;

    rect_t r = {
        .x = 0,
        .y = 0,
        .width = d->r.width,
        .height = d->r.height,
    };
    blit(d->g, r, fb, d->r);

    d->visible = true;
    ui_dialog_draw(d);

    for (int i = 0; i < d->num_controls; i++) {
        control_t *control = d->controls[i];
        if (!d->active && control->type != CONTROL_LABEL) {
            d->active = control;
        }
        control->draw(control);
        control->dirty = false;
    }

    display_update_rect(d->r);

    uint16_t keys = 0, changes = 0, pressed;
    do {
        vTaskDelay(10 / portTICK_PERIOD_MS);
        keys = keypad_debounce(keypad_sample(), &changes);
        pressed = keys & changes;
        statusbar_update();

        control_t *new_active = NULL;
        if (pressed & KEYPAD_UP) {
            new_active = dialog_find_control(d, DIRECTION_UP);
        } else if (pressed & KEYPAD_RIGHT) {
            new_active = dialog_find_control(d, DIRECTION_RIGHT);
        } else if (pressed & KEYPAD_DOWN) {
            new_active = dialog_find_control(d, DIRECTION_DOWN);
        } else if (pressed & KEYPAD_LEFT) {
            new_active = dialog_find_control(d, DIRECTION_LEFT);
        }

        if (new_active) {
            control_t *old_active = d->active;
            d->active = new_active;
            old_active->draw(old_active);
            new_active->draw(new_active);
        }

        if (pressed & KEYPAD_A && d->active->onselect) {
            d->active->onselect(d->active);
        }

        bool dirty = false;
        for (int i = 0; i < d->num_controls; i++) {
            control_t *control = d->controls[i];
            if (control->dirty) {
                dirty = true;
                control->dirty = false;
            }
        }
        if (dirty) {
            display_update_rect(d->r);
        }
    } while (!(pressed & KEYPAD_B));

    blit(fb, d->r, d->g, r);
    display_update_rect(d->r);

    d->visible = false;
}

void dialog_hide(dialog_t *d)
{
    d->close = true;
}

void dialog_insert_control(dialog_t *d, int index, control_t *control)
{
    assert(control != NULL);
    assert(d->num_controls < MAX_CONTROLS);
    if (index < 0) {
        index = d->num_controls + index;
    }
    assert(index >= 0 && index <= d->num_controls);

    memmove(&d->controls[index + 1], &d->controls[index], sizeof(struct control_t *) * (d->num_controls - index));
    d->controls[index] = control;
    control->d = d;
    d->num_controls += 1;
}

void dialog_append_control(dialog_t *d, control_t *control)
{
    dialog_insert_control(d, d->num_controls, control);
}

control_t *dialog_remove_control(dialog_t *d, int index)
{
    assert(d->num_controls > 0);
    if (index < 0) {
        index = d->num_controls + index;
    }
    assert(index >= 0 && index <= d->num_controls);

    control_t *control = d->controls[index];
    memmove(&d->controls[index], &d->controls[index + 1], sizeof(struct control_t *) * (d->num_controls - index - 1));
    control->d = NULL;
    return control;
}

control_t *dialog_find_control(dialog_t *d, direction_t dir)
{
    control_t *active = d->active;
    control_t *closest = NULL;
    float closest_distance = INFINITY;

    if (!active) {
        return NULL;
    }

    short active_cx = active->r.x + active->r.width / 2;
    short active_cy = active->r.y + active->r.height / 2;

    for (size_t i = 0; i < d->num_controls; i++) {
        control_t *control = d->controls[i];
        if (control == active || control->type == CONTROL_LABEL) {
            continue;
        }
        short control_cx = control->r.x + control->r.width / 2;
        short control_cy = control->r.y + control->r.height /2;

        if ((dir == DIRECTION_LEFT  && control->r.x + control->r.width - 1 < active->r.x) ||
            (dir == DIRECTION_RIGHT && control->r.x > active->r.x + active->r.width - 1) ||
            (dir == DIRECTION_UP    && control->r.y + control->r.height - 1 < active->r.y) ||
            (dir == DIRECTION_DOWN  && control->r.y > active->r.y + active->r.height - 1)) {
            short dx = abs(active_cx - control_cx);
            short dy = abs(active_cy - control_cy);
            float distance = sqrtf(dx * dx + dy * dy);
            if (closest == NULL || distance < closest_distance) {
                closest = control;
                closest_distance = distance;
            }
        }
    }
    return closest;
}
