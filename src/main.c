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

#include "ui.h"
#include "core.h"
#include "disk.h"

Ui_Main_Contents *ui = NULL;

Eina_Bool
system_check_changes(void *data)
{
    (void) data;
    int count = system_get_disks();
    if (count) {
        ecore_timer_del(timer);
        timer = NULL;
    }

    return ECORE_CALLBACK_RENEW;
}


EAPI_MAIN int
elm_main(int argc, char **argv)
{
    ecore_init(); 
    ecore_con_init();
    ecore_con_url_init();

    elm_init(argc, argv);

    if (!system_get_disks()) {
        /* keep checking until disk found */ 
        timer = ecore_timer_add(3.0, system_check_changes, NULL);
    }

    get_distribution_list();

    ecore_main_loop_begin();
    
    ecore_con_url_shutdown();
    ecore_con_shutdown();   
    ecore_shutdown();
    elm_shutdown();

    return (0);
}

ELM_MAIN()

