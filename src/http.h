#ifndef __HTTP_H__
#define __HTTP_H__

#define _DEFAULT_SOURCE 1
#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>

#define h_addr h_addr_list[0]

#define CALLBACK_DONE 0x01
#define CALLBACK_DATA 0x02

#define BUFFER_SIZE 4096

typedef int (*callback)(void *data);

typedef int (*fn_get)(void *self);
typedef void (*fn_user_agent_set)(void *self, const char *string);
typedef const char *(*fn_header_get)(void *self, const char *string);
typedef void (*fn_finish)(void *self);
typedef void (*fn_headers)(void *self);
typedef int (*fn_fd_write_set)(void *self, int fd);
typedef void (*fn_callback_set)(void *self, int type, callback func);

typedef struct _data_cb_t data_cb_t;
struct _data_cb_t {
    void *data;
    int size;
};
 
#define MAX_HEADERS 128

typedef struct _header_t header_t;
struct _header_t {
    char *name;
    char *value;
};

typedef struct _url_t url_t;
struct _url_t {
    url_t *self;
    fn_get get;
    fn_user_agent_set user_agent_set;
    fn_headers headers_get;
    fn_header_get header_get;
    fn_finish finish;
    fn_fd_write_set fd_write_set;
    fn_callback_set callback_set;
    int sock;
    BIO *bio;
    char *host;
    char *path;
    int status;
    int len;
    int fd;
    bool connection_ssl;
    bool print_percent;
    char *user_agent;
    header_t *headers[MAX_HEADERS];
    void *data;
    callback callback_data;
    callback callback_done;
};

/* Public API */

url_t *url_new(const char *url);
int url_get(url_t *url);
int get(void *self);
void url_headers_get(url_t *url);
void headers_get(void *self);
const char *url_header_get(url_t *url, const char *name);
const char *header_get(void *self, const char *name);
void url_user_agent_set(url_t *url, const char *string);
void user_agent_set(void *self, const char *string);
void url_finish(url_t *url);
void finish(void *self);
int url_fd_write_set(url_t *url, int fd);
int fd_write_set(void *self, int fd);
void url_callback_set(url_t *url, int type, callback func);
void callback_set(void *self, int type, callback func);
void fail(char *msg);

#endif
