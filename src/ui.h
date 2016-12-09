#ifndef __UI_H__
#define __UI_H__

#define ELM_INTERFACE_ATSPI_ACCESSIBLE_PROTECTED
#include <Elementary.h>
#include <Ecore.h>
#include <Ecore_Con.h>

static char *remote_url = NULL;
static char *local_url = NULL;

Ecore_Thread *thread;
Ecore_Timer *timer;

typedef struct _Ui_Main_Contents Ui_Main_Contents;
struct _Ui_Main_Contents { 
    Evas_Object *win;
    Evas_Object *icon;
    Evas_Object *box;
    Evas_Object *combobox_source;
    Evas_Object *combobox_dest;
    Evas_Object *sha256_label;
    Evas_Object *progressbar;
    Evas_Object *table;
    Evas_Object *bt_ok;
    Evas_Object *bt_cancel;
    Evas_Object *bt_about;
    Evas_Object *ee_effect;
    Evas  *canvas;
    char *sha256sum;
};

typedef struct distro_t distro_t;
struct distro_t {
    char *name;
    char *url;
};

distro_t *distributions[128];

Ui_Main_Contents *elm_window_create(void);
void update_combobox_storage(void);
void error_popup(Evas_Object *win);

#endif
