#include <stdio.h>
#include <string.h>

#include "../util.h"

const char *
getdns(void)
{
    FILE *fp;
    char line[200] , *p;
    if((fp = fopen("/etc/resolv.conf" , "r")) == NULL)
    {
        printf("Failed opening /etc/resolv.conf file \n");
    }
    while(fgets(line , 200 , fp))
    {   

        if(line[0] == '#')
        {
            continue;
        }
      
        if(strncmp(line , "nameserver" , 10) == 0) // Finds the first line with "nameserver"
        {
            p = strtok(line , " "); // Grabs "nameserver"
            p = strtok(NULL , " "); // Grabs the IP

            p[strlen(p) - 1] = 0; // Removes new line character
            fclose(fp);

            return bprintf("%s", p); // Ship it

        }
    }
    fclose(fp);
    return bprintf("None");
     
}