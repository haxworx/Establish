#include "http.h"

int fd = -1;
SHA256_CTX ctx;

void 
usage(void)
{
    printf("./ayup <url> <file>\n");
    exit(EXIT_FAILURE);
}

int
data_done_cb(void *data)
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

int
main(int argc, char **argv)
{
    int i = 0, j = 0;
    int len = 0;

    if (argc != 3) usage();

    url_t *req = url_new(argv[1]);

    printf("(c) Copyright 2016. Al Poole <netstar@gmail.com>\n"
           "All rights reserved.\n");

    SHA256_Init(&ctx);

    fd = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        fail("open()");
    }
   
    req->print_percent = true;

    req->callback_set(req->self, CALLBACK_DATA, data_received_cb);
    req->callback_set(req->self, CALLBACK_DONE, data_done_cb);
 
    /* override default user-agent string */
    req->user_agent_set(req->self, 
        "Mozilla/5.0 (Windows NT 10.0; WOW64) AppleWebKit/537.36 "
        "(KHTML, like Gecko) Chrome/53.0.2785.143 Safari/537.36");

    printf("User-Agent: %s\n", req->user_agent);

    req->headers_get(req->self);

    const char *length = req->header_get(req->self, "Content-Length");
    if (length) {
        len = atoi(length);
    }

    for (i = 0; req->headers[i]; i++) {
        header_t *header = req->headers[i];
        printf("%s: %s\n", header->name, header->value);
    }
 
    int status = req->get(req->self);
    switch (status) {
        case 404:
        fail("404 not found!"); 
        break;
    };

    if (status != 200) {
        fail("status is not 200!");
    }

    unsigned char result[SHA256_DIGEST_LENGTH] = { 0 };
    SHA256_Final(result, &ctx);
   
    char sha256sum[2 * SHA256_DIGEST_LENGTH + 1] = { 0 };
    for (i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        snprintf(&sha256sum[j], sizeof(sha256sum), "%02x", (unsigned int) result[i]);
        j += 2;
    }   

    req->finish(req->self);

    printf("SHA256 = %s\n", sha256sum);
    printf("done!\n");

    return (EXIT_SUCCESS);
}

