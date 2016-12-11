/*

  Copyright (c) 2016, Al Poole <netstar@gmail.com>
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:
      * Redistributions of source code must retain the above copyright
        notice, this list of conditions and the following disclaimer.
      * Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.
      * Neither the name of the <organization> nor the
        names of its contributors may be used to endorse or promote products
        derived from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/


#include "ui.h"
#include "core.h"
#include "disk.h"

extern Ui_Main_Contents *ui;

#define WIN_WIDTH 360
#define WIN_HEIGHT 360 

static char *
gl_text_get(void *data, Evas_Object *obj EINA_UNUSED, const char *part EINA_UNUSED)
{
    char buf[128];
    int i = (int)(uintptr_t)data;
    snprintf(buf, sizeof(buf), "%s", distributions[i]->name);
    return strdup(buf);
}

static char *
gl_text_dest_get(void *data, Evas_Object *obj EINA_UNUSED, const char *part EINA_UNUSED)
{
    char buf[128];
    int i = (int)(uintptr_t)data;
    snprintf(buf, sizeof(buf), "%s", storage[i]);

    return strdup(buf);
}


static Eina_Bool
gl_filter_get(void *data, Evas_Object *obj EINA_UNUSED, void *key)
{
    if (strlen((char *) key)) return EINA_TRUE;

    return EINA_TRUE;
}

void
update_combobox_storage(void)
{
    int i;
    Elm_Genlist_Item_Class *itc;
    itc = elm_genlist_item_class_new();
    itc->item_style = "default";
    itc->func.text_get = gl_text_dest_get;
    
    elm_genlist_clear(ui->combobox_dest);
     
    for (i = 0; storage[i] != NULL; i++) {
        elm_genlist_item_append(ui->combobox_dest, itc, (void *) (uintptr_t) i,
                NULL, ELM_GENLIST_ITEM_NONE, NULL, (void *)(uintptr_t) i);
        Elm_Object_Item *item = elm_genlist_first_item_get(ui->combobox_dest);
	elm_genlist_item_show(item, ELM_GENLIST_ITEM_SCROLLTO_TOP);
    } 

    if (i) {
        elm_object_part_text_set(ui->combobox_dest, "guide", "Disk found!");
        elm_genlist_realized_items_update(ui->combobox_dest);
    }
}


void about_popup(Evas_Object *win)
{
    elm_policy_set(ELM_POLICY_QUIT, ELM_POLICY_QUIT_LAST_WINDOW_CLOSED);
    elm_win_autodel_set(win, EINA_TRUE);

    Evas_Object *content = elm_label_add(win);
    elm_object_text_set(content, "<align=center>(c) Copyright 2016. Al Poole.<br><br> http://haxlab.org"
		    		 "<br><br>netstar@gmail.com</align>");

    Evas_Object *popup = elm_popup_add(win);

    elm_object_content_set(popup, content);

    elm_object_part_text_set(popup, "title,text", "About");
   
    evas_object_show(popup);

    evas_object_smart_callback_add(popup, "unblock,clicked", NULL, NULL);

    evas_object_show(win);
}


void error_popup(Evas_Object *win)
{
    elm_policy_set(ELM_POLICY_QUIT, ELM_POLICY_QUIT_LAST_WINDOW_CLOSED);
    elm_win_autodel_set(win, EINA_TRUE);

    Evas_Object *content = elm_label_add(win);
    elm_object_text_set(content, 
                        "<align=center>You don't have valid permissions to write to that location.</align>");

    Evas_Object *popup = elm_popup_add(win);

    elm_object_content_set(popup, content);

    elm_object_part_text_set(popup, "title,text", "Error");
   
    evas_object_show(popup);

    evas_object_smart_callback_add(popup, "block,clicked", NULL, NULL);

    evas_object_show(win);
}

static void
_combobox_source_item_pressed_cb(void *data EINA_UNUSED, Evas_Object *obj,
                      void *event_info)
{
    char buf[256];
    const char *txt = elm_object_item_text_get(event_info);
    int i = (int)(uintptr_t) elm_object_item_data_get(event_info);

    elm_genlist_realized_items_update(ui->combobox_source);

    snprintf(buf, sizeof(buf), "%s", distributions[i]->url);

    if (remote_url) free(remote_url);
    remote_url = strdup(buf);

    elm_object_text_set(obj, txt);
    elm_combobox_hover_end(obj);
}


static void
_combobox_storage_item_pressed_cb(void *data EINA_UNUSED, Evas_Object *obj,
                      void *event_info)
{
    int i = (int)(uintptr_t) elm_object_item_data_get(event_info);
    const char *txt = elm_object_item_text_get(event_info);

    if (local_url) free(local_url);
    local_url = strdup(txt);
    elm_object_text_set(obj, txt);
    elm_combobox_hover_end(obj);
}

static void
thread_do(void *data, Ecore_Thread *thread)
{
    int count = 0;
    ui->sha256sum = www_file_save(thread, remote_url, local_url);

    if (ecore_thread_check(thread)) {
       return;
    }
}

static void
thread_end(void *data, Ecore_Thread *thread)
{
    if (ui->sha256sum) {
        elm_object_text_set(ui->sha256_label, ui->sha256sum);
        free(ui->sha256sum);
    } else
        error_popup(ui->win);

    elm_object_disabled_set(ui->bt_ok, EINA_FALSE);
    thread = NULL; 
}

static void
thread_feedback(void *data, Ecore_Thread *thread, void *msg)
{
    int *c = msg;

    elm_progressbar_value_set(ui->progressbar, (double) *c / 10000);

    free(c);
}

#define thread_cancel thread_end

static void
_win_delete_cb(void *data, Evas_Object *obj, void *event_info)
{
    evas_object_del(obj);
    if (timer) {
        ecore_timer_del(timer);
    }
    ecore_main_loop_quit();
    elm_exit();
}

static void
_bt_about_clicked_cb(void *data, Evas_Object *obj, void *event)
{
    about_popup(ui->win);
}

static void
_bt_cancel_clicked_cb(void *data, Evas_Object *obj, void *event)
{
    evas_object_del(obj);

    if (timer) {
        ecore_timer_del(timer);
    }
   
    while((ecore_thread_wait(thread, 0.1)) != EINA_TRUE);
    thread = NULL;
    elm_exit();
}

static void
_bt_clicked_cb(void *data EINA_UNUSED, Evas_Object *obj, void *event EINA_UNUSED)
{
   if (!remote_url) return;
   const char *txt = elm_object_text_get(ui->combobox_dest); 
   if (strlen(txt) == 0) return;

   if (txt)
       local_url = strdup(txt);

   printf("remote: %s and local: %s\n\n", remote_url, local_url);

#if ! defined(__FreeBSD__)
   /* The ecore_con engine (better) */
   ecore_www_file_save(remote_url, local_url);
#else
   /* XXX: The fallback engine!  
    *
    * FreeBSD has a wee issue for now use this 
    * Also this should work everywhere else */

   elm_object_disabled_set(ui->bt_ok, EINA_TRUE);

   thread = ecore_thread_feedback_run(thread_do, thread_feedback, thread_end, thread_cancel,
    		   NULL, EINA_FALSE);
#endif
   return; 
}


static void
_win_resize_cb(void *data, Evas *e, Evas_Object *obj,void *event_info)
{
    int w, h;
    evas_object_geometry_get(obj, NULL, NULL, &w, &h);
    evas_object_resize(ui->ee_effect ,w,h);
}

Ui_Main_Contents *elm_window_create(void)
{
    int i;
    char path[PATH_MAX];

    ui = malloc(sizeof(Ui_Main_Contents));

    elm_policy_set(ELM_POLICY_QUIT, ELM_POLICY_QUIT_LAST_WINDOW_CLOSED);
    ui->win = elm_win_util_standard_add("Establish", "Establish");


    ui->icon = evas_object_image_add(evas_object_evas_get(ui->win));
    evas_object_image_file_set(ui->icon, "images/icon.ico", NULL);
    elm_win_icon_object_set(ui->win, ui->icon);
    evas_object_show(ui->icon);

    Evas_Object *popup = elm_popup_add(ui->win);
    elm_popup_orient_set(popup, ELM_POPUP_ORIENT_CENTER);
    elm_popup_scrollable_set(popup, EINA_TRUE);
    evas_object_size_hint_weight_set(popup, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_show(popup);
   
    ui->table = elm_table_add(popup);
    ui->box = elm_box_add(popup);

    const char *message = "<font_size=18 font_weidth=semibold>Establish</font>";
    ui->label = elm_label_add(popup);
    elm_object_text_set(ui->label, message);
    evas_object_show(ui->label);
    elm_box_pack_end(ui->box, ui->label);

    ui->combobox_source = elm_combobox_add(popup);
    evas_object_size_hint_weight_set(ui->combobox_source, EVAS_HINT_EXPAND, 0);
    evas_object_size_hint_align_set(ui->combobox_source, EVAS_HINT_FILL, 0);
    elm_object_part_text_set(ui->combobox_source, "guide", "Select an operating system...");
    elm_box_pack_end(ui->box, ui->combobox_source);
    evas_object_show(ui->combobox_source);

    Elm_Genlist_Item_Class *itc = elm_genlist_item_class_new();
    itc->item_style = "default";
    itc->func.text_get = gl_text_get;
    
    for (i = 0; distributions[i] != NULL; i++)
        elm_genlist_item_append(ui->combobox_source, itc, (void *) (uintptr_t) i,
                NULL, ELM_GENLIST_ITEM_NONE, NULL, (void *)(uintptr_t) i);
    evas_object_smart_callback_add(ui->combobox_source, "item,pressed",
                                     _combobox_source_item_pressed_cb, NULL);

    ui->combobox_dest = elm_combobox_add(popup);
    evas_object_size_hint_weight_set(ui->combobox_dest, EVAS_HINT_EXPAND, 0);
    evas_object_size_hint_align_set(ui->combobox_dest, EVAS_HINT_FILL, 0);
    elm_object_part_text_set(ui->combobox_dest, "guide", "Enter or select destination...");
    elm_box_pack_end(ui->box, ui->combobox_dest);
    evas_object_show(ui->combobox_dest);
    Elm_Genlist_Item_Class *itc2 = elm_genlist_item_class_new();
    itc2->item_style = "default";
    itc2->func.text_get = gl_text_dest_get;

    for (i = 0; storage[i] != NULL; i++)
        elm_genlist_item_append(ui->combobox_dest, itc2, (void *) (uintptr_t) i,
                NULL, ELM_GENLIST_ITEM_NONE, NULL, (void *)(uintptr_t) i);

    evas_object_smart_callback_add(ui->combobox_dest, "item,pressed",
                                  _combobox_storage_item_pressed_cb, NULL);

    ui->progressbar= elm_progressbar_add(popup);
    evas_object_size_hint_align_set(ui->progressbar, EVAS_HINT_FILL, 0.5);
    evas_object_size_hint_weight_set(ui->progressbar, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    //elm_progressbar_pulse_set(ui->progressbar, EINA_TRUE);
    elm_progressbar_span_size_set(ui->progressbar, 1.0);
    elm_progressbar_unit_format_set(ui->progressbar, "%1.2f%%");
    elm_box_pack_end(ui->box, ui->progressbar);
    evas_object_show(ui->progressbar);

    ui->sha256_label = elm_entry_add(popup);
    elm_entry_single_line_set(ui->sha256_label, EINA_TRUE);
    elm_entry_scrollable_set(ui->sha256_label, EINA_TRUE);
    evas_object_size_hint_weight_set(ui->sha256_label, EVAS_HINT_EXPAND, 0.5);
    evas_object_size_hint_align_set(ui->sha256_label, EVAS_HINT_FILL, EVAS_HINT_FILL);
    evas_object_size_hint_padding_set(ui->sha256_label, 5, 5, 0, 0);
    elm_box_pack_end(ui->box, ui->sha256_label);
    evas_object_show(ui->sha256_label);

    Evas_Object *separator = elm_separator_add(popup);
    elm_separator_horizontal_set(separator, EINA_TRUE);
    evas_object_show(separator);
    elm_box_pack_end(ui->box, separator); 
    
    ui->bt_ok = elm_button_add(popup);
    elm_object_text_set(ui->bt_ok, "Start");
    elm_table_pack(ui->table, ui->bt_ok, 0, 0, 1, 1);
    evas_object_smart_callback_add(ui->bt_ok, "clicked", _bt_clicked_cb, NULL);
    evas_object_show(ui->bt_ok);

    ui->bt_cancel = elm_button_add(popup);
    elm_object_text_set(ui->bt_cancel, "Exit");
    elm_table_pack(ui->table, ui->bt_cancel, 1, 0, 1, 1);
    evas_object_show(ui->bt_cancel);
    evas_object_smart_callback_add(ui->bt_cancel, "clicked", _bt_cancel_clicked_cb, NULL);

    ui->bt_about = elm_button_add(popup);
    elm_object_text_set(ui->bt_about, "About");
    elm_table_pack(ui->table, ui->bt_about, 2, 0, 1, 1);
    evas_object_show(ui->bt_about);
    evas_object_smart_callback_add(ui->bt_about, "clicked", _bt_about_clicked_cb, NULL);
   
    elm_box_pack_end(ui->box, ui->table);

    // This is for FUN!!! 
    ui->canvas = evas_object_evas_get(ui->win);
    ui->ee_effect = edje_object_add(ui->canvas);
    snprintf(path, sizeof(path), "objects/test.edj");
    edje_object_file_set(ui->ee_effect, path, "layout");
    evas_object_color_set(ui->ee_effect, 255, 255, 255, 48);
    evas_object_resize(ui->ee_effect,WIN_WIDTH, WIN_HEIGHT);
    evas_object_show(ui->ee_effect);
    evas_object_pass_events_set(ui->ee_effect, EINA_TRUE);


    Evas_Object *table = elm_table_add(popup);
    elm_table_pack(table, ui->box, 0, 0, 1, 1);
    elm_table_pack(table, ui->table, 0, 1, 1, 1);
    elm_object_content_set(popup, table);
    evas_object_size_hint_weight_set(table, EVAS_HINT_EXPAND, 0.5);
    evas_object_size_hint_align_set(table, EVAS_HINT_FILL, EVAS_HINT_FILL);
    evas_object_show(table);
    
    evas_object_resize(ui->win, WIN_WIDTH, WIN_HEIGHT);
    evas_object_show(ui->win);

    evas_object_resize(popup, 600, 600);
    evas_object_show(popup);
   
    evas_object_size_hint_weight_set(ui->table, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_size_hint_align_set(ui->table, EVAS_HINT_FILL, EVAS_HINT_FILL);
    evas_object_show(ui->table);

    evas_object_size_hint_weight_set(ui->box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_size_hint_align_set(ui->box, EVAS_HINT_FILL, EVAS_HINT_FILL);
    evas_object_show(ui->box);

    evas_object_event_callback_add(ui->win, EVAS_CALLBACK_RESIZE, _win_resize_cb, NULL);
    evas_object_smart_callback_add(ui->win, "delete,request", _win_delete_cb, NULL);
    
    return ui;
}

