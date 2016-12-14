#include "http.h"

#define h_addr h_addr_list[0]

void fail(char *msg)
{
    fprintf(stderr, "Error: %s\n", msg);
    exit(EXIT_FAILURE);
}

static char *
host_from_url(const char *host)
{
    char *addr = strdup(host);
    char *end;

    char *str = strstr(addr, "http://");
    if (str) {
        addr += strlen("http://");
        end = strchr(addr, '/');
        if (end) {
            *end = '\0';
        }
        return strdup(addr);
    }

    str = strstr(addr, "https://");
    if (str) {
        addr += strlen("https://");
        end = strchr(addr, '/');
        if (end) {
            *end = '\0';
        }
        return strdup(addr);
    }


    return strdup(addr);
}


static char *
path_from_url(const char *path)
{
    if (path == NULL) return (NULL);
    char *addr = strdup(path);
    char *p = addr;

    if (!p) {
        return (NULL);
    }

    char *str = strstr(addr, "http://");
    if (str) {
        str += 7;
        char *p = strchr(str, '/');
        if (p) {
            return strdup(p);
        } else {
            return strdup("/");
        }
    }

    str = strstr(addr, "https://");
    if (str) {
        str += 8;
        char *p = strchr(str, '/');
        if (p) {
            return strdup(p);
        } else {
            return strdup("/");
        }
    }

    return (p);
}


static BIO *
Connect_Ssl(const char *hostname, int port)
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

    SSL_CTX_set_timeout(ctx, 5);
    SSL_CTX_load_verify_locations(ctx, "/etc/ssl/certs", NULL);

    bio = BIO_new_ssl_connect(ctx);
    if (!bio) {
        fail("BIO_new_ssl_connect");
    }

    BIO_get_ssl(bio, &ssl);
    SSL_set_mode(ssl, SSL_MODE_AUTO_RETRY);
    BIO_set_conn_hostname(bio, bio_addr);

    if (BIO_do_connect(bio) <= 0) {
        fail("BIO_do_connect");
    }

    return (bio);

}


static int
Connect(const char *hostname, int port)
{
#if 0 
    int sock;
    int error;
    struct addrinfo hints, *res, *res0;
    struct servent *s;

    s = getservbyport(htons(port), NULL);
    if (!s) {
      fail("getservbyport");
    }

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    //hints.ai_family = AF_INET AF_INET6;
    hints.ai_socktype = SOCK_STREAM;

    error = getaddrinfo(hostname, s->s_name, &hints, &res0);
    if (error) {
        fail("invalid host");
    } 
    sock = -1; 
    for (res = res0; res; res = res->ai_next) {
        if (res->ai_family != AF_INET && res->ai_family != AF_INET6) continue;
        sock = socket(res->ai_family, res->ai_socktype,
                      res->ai_protocol);
        if (sock == -1) continue;

        if (connect(sock, res->ai_addr, res->ai_addrlen) == -1) {
            continue;
        } 

        break;
    };
    
    freeaddrinfo(res0);

    if (sock == -1) {
        return (0);
    } 

    return (sock);
#else
    int sock;
    struct sockaddr_in host_addr;
    struct hostent *host;

    host = gethostbyname(hostname);
    if (host == NULL)
        fail("invalid host");

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (!sock)
        return (0);

    host_addr.sin_family = AF_INET;
    host_addr.sin_port = htons(port);
    host_addr.sin_addr = *((struct in_addr *) host->h_addr);

    int status = connect(sock, (struct sockaddr *) & host_addr,
                              sizeof(struct sockaddr));
    if (status == 0) {
       return (sock);
    }

    return (0);
#endif
}

static ssize_t 
Write(url_t *url, char *bytes, size_t len)
{
    if (url->connection_ssl) {
        return BIO_write(url->bio, bytes, len);
    }

    return write(url->sock, bytes, len);
}


static ssize_t
Read(url_t *url, char *buf, size_t len)
{
    if (url->connection_ssl) {
        return BIO_read(url->bio, buf, len);
    } 

    return read(url->sock, buf, len);
}


const char *
url_header_get(url_t *url, const char *name)
{
    int i;

    for (i = 0; url->headers[i]; i++) {
        header_t *tmp = url->headers[i];
        if (!tmp) return NULL;
        if (!strcmp(tmp->name, name)) {
            return tmp->value;
        }
    }
    return NULL;
}

#define MAX_FILE_SIZE 2147483648 - 1

static void
_http_content_get(url_t *url)
{
    /* This is important for buffered writes on FreeBSD */
    char buf[BUFFER_SIZE];
    unsigned int length = 0;
    int total = 0;
    int bytes;
    int ratio;

    const char *have_length = url_header_get(url, "Content-Length");
    if (have_length) {
        length = atoi(have_length);
        url->len = length;
    }
    
    if (!have_length) {
        /* If no content lengt */
        length = MAX_FILE_SIZE;
    }

    ratio = length / 100;

    /* start the read by reading one byte */
    Read(url, buf, 1);

    if (length && !url->callback_data && url->fd == -1) {
        url->data = calloc(1, length);
    }

 
    do {
        bytes = Read(url, buf, BUFFER_SIZE);
        if (bytes <= 0 ) {
            break; 
        }

        while (bytes < BUFFER_SIZE) {
            fprintf(stderr, "Buffering...[read]\n");
	    if ((total + bytes) == length) break;
	    ssize_t count = Read(url, &buf[bytes], total - bytes);
	    if (bytes <= 0) {
                break;
            }
	    bytes += count;
        } 

        if (url->callback_data) {
            data_cb_t received;
            received.data = buf;
            received.size = bytes;
            url->callback_data(&received);
        } else {
            if (url->fd >= 0) {
                write(url->fd, buf, bytes);
            } else {
                unsigned char *pos = (unsigned char *) url->data + total;
                memcpy(pos, buf, bytes);
            }
        }

        total += bytes; 

	if (have_length && url->print_percent) {
            int percent = (total / ratio);
	    printf("%d of %d bytes (%d%%)\t\t\r", total, length, percent);
            fflush(stdout);
        }

    } while (total < length);

    if (url->callback_done) {
        url->callback_done(NULL);
    }

    printf("\n");
}


static int
_http_headers_get(url_t *url)
{
    int i;

    for (i = 0; i < MAX_HEADERS; i++) {
        url->headers[i] = NULL;
    }

    int bytes = 0;
    int len = 0;
    char buf[4096] = { 0 };

    int idx = 0;

    while (1) {
        len = 0;
        while (buf[len - 1] != '\r' && buf[len] != '\n') {
            bytes = Read(url, &buf[len], 1);
            len += bytes;
        }

        buf[len] = '\0';

        if (strlen(buf) == 2) return (1);

        int count = sscanf(buf, "\nHTTP/1.1 %d", &url->status);
        if (count) continue;
        url->headers[idx] = calloc(1, sizeof(header_t));
        char *start = strchr(buf, '\n');
        if (start) {
            start++; 
        }
        char *end = strchr(start, ':');
        *end = '\0';
        url->headers[idx]->name = strdup(start);

        start = end + 1;
        while (start[0] == ' ') {
            start++;
        }

        end = strchr(start, '\r'); 
        *end = '\0'; 
        url->headers[idx]->value = strdup(start);

        ++idx;

        memset(buf, 0, len);
    }

    return (0);
}

void
url_user_agent_set(url_t *url, const char *string)
{
    if (url->user_agent) free(url->user_agent);
    url->user_agent = strdup(string);
}

url_t *
url_new(const char *addr)
{
    url_t *url = calloc(1, sizeof(url_t));

    url->fd = -1;

    if (!url->user_agent) {
        url->user_agent = strdup
	("Mozilla/5.0 (Linux; Android 4.0.4; Galaxy Nexus Build/IMM76B) AppleWebKit/535.19 (KHTML, like Gecko) Chrome/18.0.1025.133 Mobile Safari/535.19");
    } 

    if (!strncmp(addr, "https://", 8)) {
        url->connection_ssl = true;
    } 
 
    url->host = host_from_url(addr);
    url->path = path_from_url(addr);

    url->get = &get; 
    url->user_agent_set = &user_agent_set;
    url->finish = &finish;
    url->headers_get = &headers_get;
    url->header_get = &header_get;
    url->fd_write_set = &fd_write_set;
    url->callback_set = &callback_set;


    url->self = url;

    if (url->connection_ssl) {
        url->bio  = Connect_Ssl(url->host, 443);
    } else 
        url->sock = Connect(url->host, 80);

    if (url->bio || url->sock) {
        char query[4096];

        snprintf(query, sizeof(query), "GET /%s HTTP/1.1\r\n"
	    "User-Agent: %s\r\n"
    	    "Host: %s\r\n\r\n", url->path, url->user_agent, url->host);
       
        Write(url, query, strlen(query)); 
    }

    return url;
}

int 
get(void *self)
{
    url_t *this = (url_t *)self;
    return url_get(this);
}

void
user_agent_set(void *self, const char *string)
{
    url_t *this = (url_t *) self;
    url_user_agent_set(this, string);
}

const char *
header_get(void *self, const char *name)
{
    url_t *this = (url_t *) self;
    return url_header_get(this, name);
}

void 
finish(void *self)
{
    url_t *this = (url_t *) self;

    url_finish(this);
}

int 
fd_write_set(void *self, int fd)
{
    url_t *this = (url_t *) self;

    return url_fd_write_set(this, fd); 
}

void
callback_set(void *self, int type, callback func)
{
    url_t *this = (url_t *) self;
    
    url_callback_set(this, type, func);
}


int
url_get(url_t *url)
{
    if (url->headers[0] == NULL) {
        _http_headers_get(url);
    }

    _http_content_get(url);
     
    return url->status;
}

void 
url_headers_get(url_t *url)
{
    _http_headers_get(url);
}

void 
headers_get(void *self)
{
    url_t *this = self;
    url_headers_get(this);
}

void
url_finish(url_t *url)
{
    if (url->connection_ssl) {
        BIO_free_all(url->bio); 
    } else if (url->sock >= 0) {
        close(url->sock); 
    }

    if (url->fd >= 0) {
        close(url->fd);
    }

    if (url->host) free(url->host);
    if (url->path) free(url->path);
    int i;
    for (i = 0; i < MAX_HEADERS; i++) {
        header_t *tmp = url->headers[i];
        if (!tmp) { 
            break;
        }
        free(tmp->name);
        free(tmp->value);
        free(tmp);
    } 
    if (url->user_agent) free(url->user_agent);
    free(url->data);
}

int
url_fd_write_set(url_t *url, int fd)
{
    if (fd >= 0) {
        url->fd = fd;
    }
    return (0);
}

void
url_callback_set(url_t *url, int type, callback func)
{
    switch(type) {
    case CALLBACK_DATA:
        url->callback_data = func;  
    break;
    case CALLBACK_DONE:
        url->callback_done = func;
    };
}


