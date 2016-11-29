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


#include "disk.h"
#include "ui.h"

extern Ui_Main_Contents *ui;

static void
_clear_storage(void)
{
    int i;

    for (i = 0; i < MAX_DISKS; i++) {
       if (storage[i]) {
          free(storage[i]);
          storage[i] = NULL;
       } 
    }
}

static int 
_string_cmp(const void *a, const void *b)
{
    const char *s1 = *(const char **) a;
    const char *s2 = *(const char **) b;

    return strcmp(s1, s2);
}

int 
system_get_disks(void)
{
    int disk_count = 0;
    char buf[256];
    bool is_first = true;
    char *drives;
    size_t len;

    _clear_storage();

#if defined(__OpenBSD__) || defined(__NetBSD__)
    static const mib[] = { CTL_HW, HW_DISKNAMES };
    static const unsigned int miblen = 2;

    if ((sysctl(mib, miblen, NULL, &len, NULL, 0)) < 0) {
        return (0);
    }

    if (!len) return (0);

    drives = calloc(1, len + 1);
    if (!drives) {
        return (0);
    }

    if ((sysctl(mib, miblen, drives, &len, NULL, 0)) < 0) {
        return (0); 
    }

    char *s = drives;
    while (s) {
        char *end = strchr(s, ':');
        if (!end) { 
            break;
        }        
      
        *end = '\0';
 
	if (is_first || !strncmp(s, "cd", 2)) {
            is_first = false;
	    goto skip;
        }

        if (s[0] == ',') {
            s++;
        }

        snprintf(buf, sizeof(buf), "/dev/%sc", s);
        storage[disk_count++] = strdup(buf);
skip:
        end++;
        s = strchr(end, ',');
        if (!s) {
            break;
        }
    }

    free(drives);

#elif defined(__FreeBSD__) || defined(__DragonFly__)

    if ((sysctlbyname("kern.disks", NULL, &len, NULL, 0)) < 0) {
        return (0);
    }

    drives = calloc(1, len + 1);
    if (!drives) {
        return (0);
    }

    if ((sysctlbyname("kern.disks", drives, &len, NULL, 0)) < 0) {
        return (0);
    }
    
    char *s = drives;
    while (s) {
        char *end = strchr(s, ' ');
	if (end)  
	*end = '\0';

        if (is_first || !strncmp(s, "cd", 2) || !strncmp(s, "vn", 2) || !strncmp(s, "md", 2)) {
            is_first = false;
	    goto skip;
	}

	snprintf(buf, sizeof(buf), "/dev/%s", s);
	storage[disk_count++] = strdup(buf);
skip:
	if (!end) {
            break;	
	}
	end++;
	s = end;
    }

    free(drives);

#elif defined(__linux__) // This is gross!
    Eina_List *devices = NULL, *parents = NULL, *blacklist = NULL;
    Eina_List *l;
    char *data;
    const char *path;

    eeze_init();

    devices = eeze_udev_find_by_type(EEZE_UDEV_TYPE_DRIVE_MOUNTABLE, NULL); 
    if (!devices) {
        devices = eeze_udev_find_by_type(EEZE_UDEV_TYPE_DRIVE_REMOVABLE, NULL);
    }

    EINA_LIST_FOREACH(devices, l, data) {
	path = eeze_udev_syspath_get_parent((char *) data); 
	if (path) {
            if (!eina_list_data_find(parents, path)) {
                parents = eina_list_append(parents, path);	
	    }
	}
    }

    eina_list_free(devices);

    /* This list should contain mounted file systems to avoid */

    devices = eeze_udev_find_by_type(EEZE_UDEV_TYPE_DRIVE_INTERNAL, NULL);
    EINA_LIST_FOREACH(devices, l, data) {
        path = eeze_udev_syspath_get_devpath((char *) data);
	if (path) {
            blacklist = eina_list_append(blacklist, path); 
	}
    }

    parents = eina_list_sort(parents, -1, _string_cmp);

    EINA_LIST_FOREACH(parents, l, data) {
        path = eeze_udev_syspath_get_devpath(data);
	if (!path) continue;
	if (!eina_list_data_find(blacklist, path)) {
	    storage[disk_count++] = strdup(path);
	    printf("Adding: %s\n", path);
	}
    }

    eina_list_free(devices);
    eina_list_free(parents);
    eina_list_free(blacklist);

    eeze_shutdown();
#else
#error "Unsupported Operating System"
#endif   
    storage[disk_count] = NULL;

    if (disk_count) {
        qsort(storage, disk_count, sizeof(char *), _string_cmp);
        if (ui) {
            update_combobox_storage();
        }
    }

    return (disk_count);
}

