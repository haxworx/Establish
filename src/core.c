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

extern Ui_Main_Contents *ui;

char buffer[65535];
ssize_t total = 0;

void
populate_list(void)
{
    char *start = &buffer[0];
    int count = 0;

    while (start) {
        char *end = strchr(start, '=');
        if (!end) break;
        *end = '\0';
        char *name = strdup(start);
        start = end + 1; end++;
        while (end[0] != '\n') {
            end++;
        }
        *end = '\0';
        char *url = strdup(start);
        printf("url %s\n", url);
        printf("name %s\n\n", name);
        distributions[count] = malloc(sizeof(distro_t));
        distributions[count]->name = strdup(name);
        distributions[count]->url = strdup(url);
        count++;

        start = end + 1;
    }

    distributions[count] = NULL;

    /* CREATE the main window here... */
    ui = elm_window_create();
}

typedef struct _handler_t handler_t;
struct _handler_t {
    SHA256_CTX ctx;
    int fd;
    Ecore_Con_Url *h;
    Ecore_Event_Handler *add;
    Ecore_Event_Handler *complete;
};

static Eina_Bool
_list_data_cb(void *data, int type EINA_UNUSED, void *event_info)
{
    Ecore_Con_Event_Url_Data *url_data = event_info;
    char *buf = &buffer[total];

    int i;

    for (i = 0; i < url_data->size; i++) {
        buf[i] = url_data->data[i];
    }

    total += url_data->size;
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
    ecore_con_url_free(handle->h);
    
    populate_list();
}

Eina_Bool
get_distribution_list(void)
{
    handler_t *handler = malloc(sizeof(*handler));

    if (!ecore_con_url_pipeline_get()) {
        ecore_con_url_pipeline_set(EINA_TRUE);
    }

    handler->h = ecore_con_url_new("http://haxlab.org/list.txt");

    handler->add = ecore_event_handler_add(ECORE_CON_EVENT_URL_DATA, _list_data_cb, NULL);
    handler->complete = ecore_event_handler_add(ECORE_CON_EVENT_URL_COMPLETE, _list_complete_cb, handler);

    ecore_con_url_get(handler->h);
}


/* uses ecore_con as the engine...*/


static Eina_Bool
_download_data_cb(void *data, int type EINA_UNUSED, void *event_info)
{
    handler_t *h = data;
    Ecore_Con_Event_Url_Data *url_data = event_info;
    SHA256_Update(&h->ctx, url_data->data, url_data->size);
    int chunk = url_data->size;
    char *pos = url_data->data;

    while (chunk) {
        ssize_t count =  write(h->fd, pos, chunk);
        if (count <= 0) {
            break;
        }
         
        pos += count;
        chunk -= count;
    }

    return EINA_TRUE;
}

static Eina_Bool
_download_complete_cb(void *data, int type EINA_UNUSED, void *event_info)
{
    handler_t *h = data;

    Ecore_Con_Event_Url_Complete *url_complete = event_info;

    close(h->fd);

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

    ecore_con_url_free(h->h);
 
    elm_progressbar_pulse(ui->progressbar, EINA_FALSE);
    elm_object_disabled_set(ui->bt_ok, EINA_FALSE);

    ecore_event_handler_del(h->add);
    ecore_event_handler_del(h->complete);
    
    //free(h);

    return EINA_TRUE;
}

static Eina_Bool
_download_progress_cb(void *data EINA_UNUSED, int type EINA_UNUSED, void *event_info)
{
    Ecore_Con_Event_Url_Progress *url_progress = event_info;
    if (url_progress->down.now == 0 || url_progress->down.total == 0) {
        return EINA_TRUE;
    }

    elm_progressbar_value_set(ui->progressbar, (double) (url_progress->down.now / url_progress->down.total));
    
    return EINA_TRUE;
}

static handler_t *h;

void
ecore_www_file_save(const char *remote_url, const char *local_uri)
{
    if (!ecore_con_url_pipeline_get()) {
        ecore_con_url_pipeline_set(EINA_TRUE);
    }

    h = malloc(sizeof(handler_t));

    SHA256_Init(&h->ctx);

    h->h = ecore_con_url_new(remote_url);
    if (!h->h) {
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

    ecore_con_url_get(h->h); 
    
    elm_progressbar_pulse(ui->progressbar, EINA_TRUE);
    elm_object_disabled_set(ui->bt_ok, EINA_TRUE);
}

/* This engine is a threaded fallback engine.
 *  It works quite well and I wish to leave it here as an example.
*/
void 
Error(char *fmt, ...)
{
    char buf[1024];
    va_list(ap);

    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    fprintf(stderr, "Error: %s\n", buf);

    exit(EXIT_FAILURE);
}

char *
from_url_host(char *host)
{
    char *addr = strdup(host);
    char *end; 

    char *str = strstr(addr, "http://");
    if (str) {
        addr += strlen("http://");
        end = strchr(addr, '/');
        *end = '\0';
        return addr;
    }

    str = strstr(addr, "https://");
    if (str) {
        addr += strlen("https://");
        end = strchr(addr, '/');
        *end = '\0';
        return addr;
    }

    return NULL;
}

char *
from_url_path(char *path)
{
    if (path == NULL) return NULL;

    char *addr = strdup(path);
    char *p = addr;

    if (!p) {
        return NULL;
    }
 
    char *str = strstr(addr, "http://");
    if (str) {
        str += 7;
        char *p = strchr(str, '/');
        if (p) {
            return p;
        }
    }

    str = strstr(addr, "https://");
    if (str) {
        str += 8;
        char *p = strchr(str, '/');
        if (p) {
            return p; 
        }
    }

    return p;
}

BIO *
connect_ssl(const char *hostname, int port)
{
    SSL_load_error_strings();
    ERR_load_BIO_strings();
    OpenSSL_add_all_algorithms();

    BIO *bio;
    char bio_addr[8192];

    snprintf(bio_addr, sizeof(bio_addr), "%s:%d", hostname, port);

    SSL_library_init();
 
    SSL_CTX *ctx = SSL_CTX_new(SSLv23_client_method());
    SSL *ssl = NULL;


    SSL_CTX_load_verify_locations(ctx, "/etc/ssl/certs", NULL);
 
    bio = BIO_new_ssl_connect(ctx);
    if (!bio) {
        Error("BIO_new_ssl_connect");
    }

    BIO_get_ssl(bio, &ssl);
    SSL_set_mode(ssl, SSL_MODE_AUTO_RETRY);
    BIO_set_conn_hostname(bio, bio_addr);

    if (BIO_do_connect(bio) <= 0) {
        Error("BIO_do_connect");
    }

    return bio;
}

typedef struct header_t header_t;
struct header_t {
    char location[1024];
    char content_type[1024];
    int content_length;
    char date[1024];
    int status;
};

ssize_t 
check_one_http_header(int sock, BIO *bio, header_t * headers)
{
    int bytes = -1;
    int len = 0;
    char buf[8192] = { 0 };
    while (1) {
        while (buf[len - 1] != '\r' && buf[len] != '\n') {
            if (!bio)
                bytes = read(sock, &buf[len], 1);
            else
                bytes = BIO_read(bio, &buf[len], 1);

            len += bytes;
        }

        buf[len] = '\0';
        len = 0;

        sscanf(buf, "\nHTTP/1.1 %d", &headers->status);
        sscanf(buf, "\nContent-Type: %s\r", headers->content_type);
        sscanf(buf, "\nLocation: %s\r", headers->location);
        sscanf(buf, "\nContent-Length: %d\r",
               &headers->content_length);


        if (headers->content_length && strlen(buf) == 2) {
            return 1;                                  // found!!
        }

        memset(buf, 0, 8192);
    }
    return 0;                                          // not found
}


int 
check_http_headers(int sock, BIO *bio, const char *addr, const char *file)
{
    char out[8192] = { 0 };
    header_t headers;

    memset(&headers, 0, sizeof(header_t));

    snprintf(out, sizeof(out), "GET %s HTTP/1.1\r\nHost: %s\r\n\r\n", file, addr);

    ssize_t len = 0;

    if (!bio) {
        len = write(sock, out, strlen(out));
    } else {
        len = BIO_write(bio, out, strlen(out));
    }

    len = 0;

    do {
        len = check_one_http_header(sock, bio, &headers);
    } while (!len);

    if (!headers.content_length)
        Error("BAD BAD HTTP HEADERS!");

    return headers.content_length;
}


int 
connect_tcp(const char *hostname, int port)
{
    int sock;
    struct hostent *host;
    struct sockaddr_in host_addr;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        Error("socket");
    }

    host = gethostbyname(hostname);
    if (!host) {
        Error("gethostbyname");
    }

    host_addr.sin_family = AF_INET;
    host_addr.sin_port = htons(port);
    host_addr.sin_addr = *((struct in_addr *) host->h_addr);
    memset(&host_addr.sin_zero, 0, 8);

    int status = connect(sock, (struct sockaddr *) &host_addr,
                    sizeof(struct sockaddr));

    if (status == 0) {
        return sock;
    }

    return 0;
}

#define CHUNK 512

char *
www_file_save(Ecore_Thread *thread, const char *remote_url, const char *local_url)
{
    BIO *bio = NULL;
    int is_ssl = 0;

    char *infile = (char *) remote_url;

    if (!strncmp("https://", infile, 8)) {
        is_ssl = 1;
    } 

    char *outfile = (char *) local_url;

    const char *address = from_url_host(infile);
    const char *path = from_url_path(infile);

    int in_fd, out_fd, sock;
    
    if (is_ssl) {
        bio = connect_ssl(address, 443);
    } else {
       sock = in_fd = connect_tcp(address, 80);
    }
    
    int length = check_http_headers(sock, bio, address, path);
    printf("len is %d\n\n", length);

    out_fd = open(outfile, O_WRONLY | O_CREAT, 0666);
    if (out_fd < 0) {
        Error("open: %s", outfile);
    }

    char buf[CHUNK];
    memset(buf, 0, sizeof(buf));

    ssize_t chunk = 0;
    ssize_t bytes = 0;
    int total = 0; 

    double percent = length / 10000;

    if (bio) {
        BIO_read(bio, buf, 1);
    } else {
        read(in_fd, buf, 1);
    }

    unsigned char result[SHA256_DIGEST_LENGTH] = { 0 };
    SHA256_CTX ctx;

    SHA256_Init(&ctx);

    do {
        if (bio) {
            bytes = BIO_read(bio, buf, CHUNK); 
        } else {
            bytes = read(in_fd, buf, CHUNK);
        }

        chunk = bytes;
        
        SHA256_Update(&ctx, buf, bytes);

        while (chunk) {
            ssize_t count =  write(out_fd, buf, chunk);

            if (count <= 0) {
                break;
            }
           
            chunk -= count;
            total += count;
        }

        int current = total / percent;
        int *tmp = malloc(sizeof(double));
        *tmp = (int) current; 
        ecore_thread_feedback(thread, tmp);

        if (ecore_thread_check(thread)) {
	    return NULL;
        }

        memset(buf, 0, bytes);

    } while (total < length);

    SHA256_Final(result, &ctx);

    int i;

    char sha256[2 * SHA256_DIGEST_LENGTH + 1] = { 0 };

    int j = 0;
    for (i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        snprintf(&sha256[j], sizeof(sha256), "%02x", (unsigned int) result[i]);
        j += 2;
    } 
    
    return strdup(sha256);
}

