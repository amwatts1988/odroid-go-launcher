#include <string.h>

#include "display.h"
#include "keypad.h"
#include "OpenSans_Regular_11X12.h"
#include "statusbar.h"
#include "tf.h"
#include "ui_controls.h"
#include "ui_dialog.h"
#include "ui_osk.h"

#define BORDER_WIDTH (2)


/* ui_button */

static void button_draw(ui_control_t *control)
{
    ui_button_t *button = (ui_button_t *)control;

    button->tf->clip = button->r;
    button->tf->clip.x += button->d->cr.x;
    button->tf->clip.y += button->d->cr.y;

    fill_rectangle(fb, button->tf->clip, 0x632C);
    if (control == control->d->active) {
        draw_rectangle(fb, button->tf->clip, DRAW_STYLE_SOLID, 0xFFFF);
    }

    if (button->text) {
        tf_metrics_t m = tf_get_str_metrics(button->tf, button->text);
        point_t p = {
            .x = button->d->cr.x + button->r.x + BORDER_WIDTH,
            .y = button->d->cr.y + button->r.y + button->r.height/2 - m.height/2 + 1,
        };
        tf_draw_str(fb, button->tf, button->text, p);
    }

    button->dirty = true;
}

static void button_free(ui_control_t *control)
{
    ui_button_t *button = (ui_button_t *)control;

    if (button->text) {
        free(button->text);
    }
    tf_free(button->tf);
    free(button);
}

ui_button_t *ui_dialog_add_button(ui_dialog_t *d, rect_t r, const char *text, ui_control_onselect_t onselect)
{
    ui_button_t *button = calloc(1, sizeof(ui_button_t));

    button->type = CONTROL_BUTTON;
    button->d = d;
    button->r = r;
    button->draw = button_draw;
    button->onselect = onselect;
    button->free = button_free;
    button->text = strdup(text);
    button->tf = tf_new(&font_OpenSans_Regular_11X12, button->r.width - 2 * BORDER_WIDTH, TF_ELIDE);

    ui_dialog_add_control(d, (ui_control_t *)button);

    return button;
}


/* ui_edit */

static void edit_draw(ui_control_t *control)
{
    ui_edit_t *edit = (ui_edit_t *)control;

    edit->tf->clip = edit->r;
    edit->tf->clip.x += edit->d->cr.x;
    edit->tf->clip.y += edit->d->cr.y;

    fill_rectangle(fb, edit->tf->clip, 0x3186);
    if (control == control->d->active) {
        draw_rectangle(fb, edit->tf->clip, DRAW_STYLE_SOLID, 0xFFFF);
    }

    if (edit->text) {
        tf_metrics_t m = tf_get_str_metrics(edit->tf, edit->text);
        point_t p = {
            .x = edit->d->cr.x + edit->r.x + BORDER_WIDTH,
            .y = edit->d->cr.y + edit->r.y + edit->r.height/2 - m.height/2 + 1,
        };
        tf_draw_str(fb, edit->tf, edit->text, p);
    }

    edit->dirty = true;
}

static void edit_onselect(ui_control_t *control)
{
    ui_edit_t *edit = (ui_edit_t *)control;

    ui_osk_t *osk = ui_osk_new(edit);
    ui_osk_showmodal(osk);
    ui_osk_free(osk);

    control->draw(control);
    display_update_rect(control->d->cr);
}

static void edit_free(ui_control_t *control)
{
    ui_edit_t *edit = (ui_edit_t *)control;

    if (edit->text) {
        free(edit->text);
    }
    free(edit);
}

ui_edit_t *ui_dialog_add_edit(ui_dialog_t *d, rect_t r, const char *text, size_t text_len)
{
    ui_edit_t *edit = calloc(1, sizeof(ui_edit_t));

    edit->type = CONTROL_EDIT;
    edit->d = d;
    edit->r = r;
    edit->draw = edit_draw;
    edit->onselect = edit_onselect;
    edit->free = edit_free;
    edit->text = malloc(text_len);
    assert(edit->text != NULL);
    strncpy(edit->text, text, text_len);
    edit->text[text_len - 1] = '\0';
    edit->text_len = text_len;
    edit->tf = tf_new(&font_OpenSans_Regular_11X12, edit->r.width - 2 * BORDER_WIDTH, TF_ELIDE);

    ui_dialog_add_control(d, (ui_control_t *)edit);

    return edit;
}


/* ui_label */

static void label_draw(ui_control_t *control)
{
    ui_label_t *label = (ui_label_t *)control;

    label->tf->clip = label->r;
    label->tf->clip.x += label->d->cr.x;
    label->tf->clip.y += label->d->cr.y;

    if (label->text) {
        tf_metrics_t m = tf_get_str_metrics(label->tf, label->text);
        point_t p = {
            .x = label->d->cr.x + label->r.x + BORDER_WIDTH,
            .y = label->d->cr.y + label->r.y + label->r.height/2 - m.height/2,
        };
        tf_draw_str(fb, label->tf, label->text, p);
    }

    label->dirty = true;
}

static void label_free(ui_control_t *control)
{
    ui_label_t *label = (ui_label_t *)control;

    if (label->text) {
        free(label->text);
    }
    tf_free(label->tf);
    free(label);
}

ui_label_t *ui_dialog_add_label(ui_dialog_t *d, rect_t r, const char *text)
{
    ui_label_t *label = calloc(1, sizeof(ui_label_t));

    label->type = CONTROL_LABEL;
    label->d = d;
    label->r = r;
    label->draw = label_draw;
    label->free = label_free;
    label->text = strdup(text);
    label->tf = tf_new(&font_OpenSans_Regular_11X12, label->r.width - 2 * BORDER_WIDTH, TF_ELIDE);

    ui_dialog_add_control(d, (ui_control_t *)label);

    return label;
}


/* ui_list */

static void list_draw(ui_control_t *control)
{
    ui_list_t *list = (ui_list_t *)control;

    list->item_height = list->tf->font->height + BORDER_WIDTH * 2;
    list->rows = (list->r.height - 2 + list->item_height - 1) / list->item_height;

    list->tf->clip = list->r;
    list->tf->clip.x += list->d->cr.x + 1;
    list->tf->clip.y += list->d->cr.y + 1;
    list->tf->clip.width -= 2;
    list->tf->clip.height -= 2;

    rect_t r = list->r;
    r.x += list->d->cr.x;
    r.y += list->d->cr.y;

    fill_rectangle(fb, r, 0x3186);
    if (control == control->d->active) {
        draw_rectangle(fb, r, DRAW_STYLE_SOLID, 0xFFFF);
    }

    if (list->item_index > list->item_count) {
        list->item_index = list->item_count - 1;
    }

    if (list->item_index > list->first_index + list->rows - 1) {
        list->first_index = list->item_index - list->rows + 1;
    }
    if (list->item_index < list->first_index) {
        list->first_index = list->item_index;
    }
    if ( list->item_index < list->first_index + 1) {
        list->shift = 0;
    }
    if (list->item_index > list->first_index + list->rows - 2) {
        list->shift = list->item_height - (list->r.height - 2) % list->item_height;
        if (list->shift >= list->item_height) {
            list->shift = 0;
        }
    }

    for (unsigned int row = 0; row < list->rows; row++) {
        if (row >= list->item_count) {
            break;
        }
        ui_list_item_t *item = &list->items[list->first_index + row];

        rect_t r = list->r;
        r.x += list->d->cr.x + 1;
        r.y += list->d->cr.y - list->shift + row * list->item_height + 1;
        r.width = list->r.width - 2;
        r.height = list->item_height;

        point_t p = {
            .x = r.x + BORDER_WIDTH,
            .y = r.y + r.height/2 - list->tf->font->height/2 + 1,
        };

        if (list->first_index + row == list->item_index) {
            fill_rectangle(fb, r, list->selected ? 0x001F : 0x632C);
        }
        tf_draw_str(fb, list->tf, item->text, p);
    }

    control->dirty = true;
}

static void list_free(ui_control_t *control)
{
    ui_list_t *list = (ui_list_t *)control;

    while (list->item_count > 0) {
        ui_list_remove(list, -1);
    }
    tf_free(list->tf);
    free(list);
}

static void list_onselect(ui_control_t *control)
{
    ui_list_t *list = (ui_list_t *)control;

    rect_t r = {
        .x = list->d->cr.x + list->r.x,
        .y = list->d->cr.y + list->r.y,
        .width = list->r.width,
        .height = list->r.height,
    };

    list->selected = true;
    list->draw(control);
    display_update_rect(r);

    keypad_info_t keys;
    while (true) {
        if (keypad_queue_receive(list->d->keypad, &keys, 250 / portTICK_RATE_MS)) {
            bool dirty = false;
            if (keys.pressed & KEYPAD_UP) {
                int i;
                for (i = list->item_index - 1; i >= 0; i--) {
                    if (list->items[i].type == LIST_ITEM_TEXT) {
                        break;
                    }
                }
                if (i >= 0) {
                    list->item_index = i;
                    dirty = true;
                }
            }

            if (keys.pressed & KEYPAD_DOWN) {
                int i;
                for (i = list->item_index + 1; i < list->item_count; i++) {
                    if (list->items[i].type == LIST_ITEM_TEXT) {
                        break;
                    }
                }
                if (i < list->item_count) {
                    list->item_index = i;
                    dirty = true;
                }
            }

            if (keys.pressed & KEYPAD_A) {
                if (list->item_index >= 0 && list->item_index < list->item_count) {
                    ui_list_item_t *item = &list->items[list->item_index];
                    if (item->onselect) {
                        item->onselect(item);
                    }
                }
            }

            if (keys.pressed & KEYPAD_B) {
                break;
            }

            if (keys.pressed & KEYPAD_MENU) {
                ui_dialog_unwind();
                break;
            }

            if (dirty) {
                list->draw(control);
                display_update_rect(r);
            }
        }
        statusbar_update();
    }

    list->selected = false;
    list->draw(control);
    list->dirty = true;
}

ui_list_t *ui_dialog_add_list(ui_dialog_t *d, rect_t r)
{
    ui_list_t *list = calloc(1, sizeof(ui_list_t));

    list->type = CONTROL_LIST;
    list->d = d;
    list->r = r;
    list->draw = list_draw;
    list->onselect = list_onselect;
    list->free = list_free;
    list->tf = tf_new(&font_OpenSans_Regular_11X12, list->r.width - 2 * BORDER_WIDTH, TF_ELIDE);

    ui_dialog_add_control(d, (ui_control_t *)list);

    return list;
}

static struct ui_list_item_t *list_insert_new(ui_list_t *list, int index)
{
    if (index < 0) {
        index = list->item_count + index;
    }
    assert(index >= 0 && index <= list->item_count);

    list->items = realloc(list->items, sizeof(ui_list_item_t) * (list->item_count + 1));
    assert(list->items != NULL);

    if (index < list->item_count) {
        memmove(&list->items[index + 1], &list->items[index], sizeof(ui_list_item_t) * (list->item_count - index));
    }

    memset(&list->items[index], 0, sizeof(ui_list_item_t));
    list->item_count += 1;
    return &list->items[index];
}

void ui_list_insert_text(ui_list_t *list, int index, char *text, ui_list_item_onselect_t onselect)
{
    ui_list_item_t *item = list_insert_new(list, index);
    item->type = LIST_ITEM_TEXT;
    item->text = strdup(text);
}

void ui_list_append_text(ui_list_t *list, char *text, ui_list_item_onselect_t onselect)
{
    ui_list_insert_text(list, list->item_count, text, onselect);
}

void ui_list_remove(ui_list_t *list, int index)
{
    if (index < 0) {
        index = list->item_count + index;
    }
    assert(index >= 0 && index < list->item_count);

    struct ui_list_item_t *item = &list->items[index];
    switch (item->type) {
        case LIST_ITEM_TEXT:
            if (item->text) {
                free((void *)item->text);
            }
            break;

        default:
            break;
    }

    if (list->item_count == 1) {
        free(list->items);
        list->items = NULL;
    } else {
        if (index < list->item_count - 1) {
            memmove(&list->items[index], &list->items[index + 1], sizeof(ui_list_item_t) * (list->item_count - index - 1));
        }
        list->items = realloc(list->items, sizeof(ui_list_item_t) * list->item_count - 1);
        assert(list->items != NULL);
    }

    list->item_count -= 1;
}
