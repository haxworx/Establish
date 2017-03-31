#ifndef __CORE_H__
#define __CORE_H__

#define _DEFAULT_SOURCE 1

#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <Eina.h>
#include <Ecore.h>
#include <Ecore_Con.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>

/* Change this to host your own list of images on 
 * another server */
#define REMOTE_LIST_URL "http://haxlab.org/list.txt"

Eina_Bool remote_distributions_get(void);

/* ecore implementation */
void ecore_www_file_save(const char *, const char *);

/* fallback implementation */
char *www_file_save(Ecore_Thread *, const char *, const char *);

#endif
