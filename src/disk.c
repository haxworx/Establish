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

void _clear_storage(void)
{
    int i;

    for (i = 0; i < MAX_DISKS; i++) {
       if (storage[i]) {
          free(storage[i]);
          storage[i] = NULL;
       } 
    }
}

int system_get_disks(void)
{
    int disk_count = 0;

    _clear_storage();

#if defined(__OpenBSD__)
    static const mib[] = { CTL_HW, HW_DISKNAMES };
    static const unsigned int miblen = 2;
    char buf[128];
    char *drives;
    size_t len;

    sysctl(mib, miblen, NULL, &len, NULL, 0);

    if (!len) return;

    drives = calloc(1, len + 1);
    if (!drives) {
        return 0;
    }

    sysctl(mib, miblen, drives, &len, NULL, 0);

    char *s = drives;
    while (s) {
        char *end = strchr(s, ':');
        if (!end) { 
            break;
        }        
      
        *end = '\0';
 
        /* Do not expose common drives */       
        if (!strcmp(s, "sd0") || !strcmp(s, "hd0") ||
            !strncmp(s, "cd", 2)) {
            goto skip;
        }

        if (s[0] == ',') {
            s++;
        }

        snprintf(buf, sizeof(buf), "/dev/%sc", s);
        printf("buffer: %s\n\n", buf);
        storage[disk_count++] = strdup(buf);
skip:
        end++;
        s = strchr(end, ',');
        if (!s) {
            break;
        }
    }

    free(drives);

#else 
    /* This is problematic...eeze is broken atm. */
    eeze_init();
    Eina_List *devices = eeze_udev_find_by_type(EEZE_UDEV_TYPE_DRIVE_REMOVABLE, NULL);
    Eina_List *l;
    char *data;

    EINA_LIST_FOREACH(devices, l, data) {
        char buf[PATH_MAX];
      
	snprintf(buf, sizeof(buf), "%s/block", data);

	DIR *dir = opendir(buf);
	struct dirent *dh;
	while (dh = readdir(dir)) {
            if (dh->d_name[0] == '.') continue;
	    char p[PATH_MAX];
            snprintf(p, sizeof(p), "%s/%s", buf, dh->d_name);
            struct stat fstats;
	    stat(p, &fstats);
	    if (S_ISDIR(fstats.st_mode)) {
                char devpath[PATH_MAX];
                snprintf(devpath, sizeof(devpath), "/dev/%s", dh->d_name);
	        storage[disk_count++] = strdup(devpath);
		break;
	    }
	}
	closedir(dir); 
    }

    eina_list_free(devices);

    eeze_shutdown();
#endif   
    storage[disk_count] = NULL;
    if (disk_count) {
        if (ui) {
            update_combobox_storage();
        }
    }
 
    return disk_count;
}

