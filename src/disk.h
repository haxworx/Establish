#ifndef __DISK_H__
#define __DISK_H__
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/sysctl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#if defined(__linux__)
#include <Eina.h>
#include <Eeze.h>
#include <Eeze_Disk.h>
#endif

#define MAX_DISKS 10
char *storage[MAX_DISKS];

int system_get_disks(void);

#endif
