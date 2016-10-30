#include "http.h"

struct distro_t {
    char *name;
    char *url;
};

struct distro_t *distributions[128];

int
parse_distribution_list(char *data)
{
    char *start = data;
    int count = 0;

    while (start) {
        char *end = strchr(start, '=');
        if (!end) {
            break;
        }

        *end = '\0';

        distributions[count] = malloc(sizeof(struct distro_t));
        distributions[count]->name = strdup(start);

        start = end + 1; end++;

        while (end[0] != '\n') {
            ++end;
        }
 
        *end = '\0';
        distributions[count++]->url = strdup(start);

        start = end + 1;
    }
  
    distributions[count] = NULL;

    return (count);
}


int
main(int argc, char **argv)
{
    int i = 0;

    url_t *req = url_new("http://haxlab.org/list.txt");

    int status = req->get(req->self);
    if (status != 200) {
        fail("status is not 200!");
    }

    int total = parse_distribution_list(req->data);
    if (total) {
        for (i = 0; distributions[i]; i++) {
            printf("(%02d) %s\n", i,distributions[i]->name);
        }

        char buf[1024];
        unsigned int choice = 1000; 

        while (choice >= total) {
            printf("\nselect : "); fflush(stdout);
            fgets(buf, sizeof(buf), stdin); 
            choice = atoi(buf);
        }

        const char *selected_url = distributions[choice]->url;
        printf("write to: "); fflush(stdout);
        fgets(buf, sizeof(buf), stdin);        
        buf[strlen(buf) -1] = '\0';
      
        const char *destination = buf;

        char command[1024];
        snprintf(command, sizeof(command), "./ayup %s %s", selected_url, destination);
        printf("executing: %s\n", command); 
        system(command);
    }

    req->finish(req->self);

    return (EXIT_SUCCESS);
}

