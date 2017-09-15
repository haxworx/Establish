#ifndef __DISK_H__
#define __DISK_H__
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/sysctl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#if defined(__linux__)
#include <Eina.h>
#include <Eeze.h>
#include <Eeze_Disk.h>
#endif

#if defined(__FreeBSD__) || defined(_DragonFly__)
#include <sys/param.h>
#include <sys/ucred.h>
#include <sys/mount.h>
#endif

#define MAX_DISKS 10
char *storage[MAX_DISKS];

int system_disks_get(void);

#endif
