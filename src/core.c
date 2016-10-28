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


#include "core.h"
#include "ui.h"
#include "http.h"

extern Ui_Main_Contents *ui;

void
parse_distribution_list(char *data)
{
    char *start = data;
    int count = 0;

    while (start) {
        char *end = strchr(start, '=');
        if (!end) {
            break;
        }

        *end = '\0';

        distributions[count] = malloc(sizeof(distro_t));
        distributions[count]->name = strdup(start);

        start = end + 1; end++;

        while (end[0] != '\n') {
            end++;
        }

        *end = '\0';

        distributions[count++]->url = strdup(start);

        start = end + 1;
    }

    distributions[count] = NULL;

    /* CREATE the main window here... */
    ui = elm_window_create();
}

typedef struct _handler_t handler_t;
struct _handler_t {
    SHA256_CTX ctx;
    char buffer[65535];
    int total;
    int fd;
    Ecore_Con_Url *conn;
    Ecore_Event_Handler *add;
    Ecore_Event_Handler *complete;
};

static Eina_Bool
_list_data_cb(void *data, int type EINA_UNUSED, void *event_info)
{
    Ecore_Con_Event_Url_Data *url_data = event_info;
    handler_t *handle = data;
    char *buf = &handle->buffer[handle->total];

    int i;

    for (i = 0; i < url_data->size; i++) {
        buf[i] = url_data->data[i];
    }

    handle->total += url_data->size;

    return (EINA_TRUE);
}

static Eina_Bool
_list_complete_cb(void *data, int type EINA_UNUSED, void *event_info)
{
    handler_t *handle = data;
    Ecore_Con_Event_Url_Complete *url_complete = event_info;

    if (url_complete->status != 200) {
        fprintf(stderr, "Error: retrieving remote list of images\n\n");
        exit(1 << 7); 
    }

    ecore_event_handler_del(handle->add);
    ecore_event_handler_del(handle->complete);
    ecore_con_url_free(handle->conn);
    
    parse_distribution_list(handle->buffer);

    return (EINA_TRUE);
}

Eina_Bool
get_distribution_list(void)
{
    handler_t *handler = calloc(1, sizeof(*handler));

    if (!ecore_con_url_pipeline_get()) {
        ecore_con_url_pipeline_set(EINA_TRUE);
    }

    handler->conn = ecore_con_url_new("http://haxlab.org/list.txt");

    handler->add = ecore_event_handler_add(ECORE_CON_EVENT_URL_DATA, _list_data_cb, handler);
    handler->complete = ecore_event_handler_add(ECORE_CON_EVENT_URL_COMPLETE, _list_complete_cb, handler);

    ecore_con_url_get(handler->conn);

    return (EINA_TRUE);
}


/* uses ecore_con as the engine...*/


static Eina_Bool
_download_data_cb(void *data, int type EINA_UNUSED, void *event_info)
{
    handler_t *h = data;
    Ecore_Con_Event_Url_Data *url_data = event_info;
    SHA256_Update(&h->ctx, url_data->data, url_data->size);
    int chunk = url_data->size;
    unsigned char *pos = url_data->data;

    while (chunk) {
        ssize_t count =  write(h->fd, pos, chunk);
        if (count <= 0) {
            break;
        }
         
        pos += count;
        chunk -= count;
    }

    return (EINA_TRUE);
}

static Eina_Bool
_download_complete_cb(void *data, int type EINA_UNUSED, void *event_info)
{
    handler_t *h = data;

    Ecore_Con_Event_Url_Complete *url_complete = event_info;

    close(h->fd);

    sync();
    sync();

    unsigned char result[SHA256_DIGEST_LENGTH] = { 0 };
    SHA256_Final(result, &h->ctx);

    int i;

    char sha256[2 * SHA256_DIGEST_LENGTH + 1] = { 0 };

    int j = 0;
    for (i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        snprintf(&sha256[j], sizeof(sha256), "%02x", (unsigned int) result[i]);
        j += 2;
    } 

    elm_object_text_set(ui->sha256_label, sha256);

    ecore_con_url_free(h->conn);
 
    elm_progressbar_pulse(ui->progressbar, EINA_FALSE);
    elm_object_disabled_set(ui->bt_ok, EINA_FALSE);

    ecore_event_handler_del(h->add);
    ecore_event_handler_del(h->complete);
    
    //free(h);

    return (EINA_TRUE);
}

static Eina_Bool
_download_progress_cb(void *data EINA_UNUSED, int type EINA_UNUSED, void *event_info)
{
    Ecore_Con_Event_Url_Progress *url_progress = event_info;
    if (url_progress->down.now == 0 || url_progress->down.total == 0) {
        return (EINA_TRUE);
    }

    elm_progressbar_value_set(ui->progressbar, (double) (url_progress->down.now / url_progress->down.total));
    
    return (EINA_TRUE);
}

void
ecore_www_file_save(const char *remote_url, const char *local_uri)
{
    if (!ecore_con_url_pipeline_get()) {
        ecore_con_url_pipeline_set(EINA_TRUE);
    }

    handler_t *h = calloc(1, sizeof(handler_t));

    SHA256_Init(&h->ctx);

    h->conn = ecore_con_url_new(remote_url);
    if (!h->conn) {
        return;
    }

    h->fd = open(local_uri,  O_WRONLY | O_CREAT, 0644);
    if (h->fd == -1) {
	error_popup(ui->win);
        return;
    }

    ecore_event_handler_add(ECORE_CON_EVENT_URL_PROGRESS, _download_progress_cb, NULL);
    h->add = ecore_event_handler_add(ECORE_CON_EVENT_URL_DATA, _download_data_cb, h);
    h->complete = ecore_event_handler_add(ECORE_CON_EVENT_URL_COMPLETE, _download_complete_cb, h);

    ecore_con_url_get(h->conn); 
    
    elm_progressbar_pulse(ui->progressbar, EINA_TRUE);
    elm_object_disabled_set(ui->bt_ok, EINA_TRUE);
}

int fd;
SHA256_CTX ctx;

int data_done_cb(void *data)
{
    return close(fd);
}

int
data_received_cb(void *data)
{
    data_cb_t *received = data;
    if (!received) return 0;
    SHA256_Update(&ctx, received->data, received->size);
    return write(fd, received->data, received->size);
}

char *
www_file_save(Ecore_Thread *thread, const char *remote_url, const char *local_uri)
{
    url_t *req = url_new(remote_url);

    SHA256_Init(&ctx);
    fd = -1;

    fd = open(local_uri, O_WRONLY | O_CREAT | O_TRUNC, 0644);

    req->callback_set(req->self, CALLBACK_DATA, data_received_cb);
    req->callback_set(req->self, CALLBACK_DONE, data_done_cb);

    int status = req->get(req->self);

    if (status != 200) {
        fail("status is not 200!");
    }

    unsigned char result[SHA256_DIGEST_LENGTH] = { 0 };
    SHA256_Final(result, &ctx);

    char sha256sum[2 * SHA256_DIGEST_LENGTH + 1] = { 0 };

    int i, j = 0;

    for (i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        snprintf(&sha256sum[j], sizeof(sha256sum), "%02x", (unsigned int) result[i]);
        j += 2;
    }

    req->finish(req->self);

    return strdup(sha256sum);
}



