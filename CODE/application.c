#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>

#include "utils.h"
#include "error.h"
#include "DataLayer.h"
#include "application.h"

//GLOBALS
applicationLayer app;

int main(void)
{
    interface();
    return 0;
}

int interface()
{
    char buf[255];
    int openType = -1;
    int option = -1;
    int portNr = -1;

    printf("\n============================\n");
    printf("Select option:\n");
    printf("0 - llopen()\n");
    printf("1 - llwrite/read()\n");
    printf("2 - llclose()\n");
    printf("============================\n\n");
    printf("Type: ");
    scanf("%s",buf);
    option = atoi(buf);
    //smth like a loop, inteface polling 
    switch (option)
    {
    case 0:
        printf("============================\n");
        printf("Port number? ");
        scanf("%s",buf);
        portNr = atoi(buf);
        if(portNr != 0 && portNr!= 1)
        {
            printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
            exit(ERR_PRT);
        }
        printf("\n0 - Transmiter/ 1 - Receiver: ");
        scanf("%s",buf);
        openType= atoi(buf);
        if (openType != 0 && openType != 1)
        {
            printf("0 - TRANSMITTER/ 1 - RECEIVER!");
            exit(ERR_ARGV2);
        }
        printf("============================\n\n");
        if(openType == 0)
            app.status = TRANSMITTER;
        else  app.status = RECEIVER;
        app.fileDescriptor = llopen(portNr,openType);
        break;
    case 1:
        switch(app.status){
            case(TRANSMITTER):
            //llwrite(); 
            break;
            case (RECEIVER):
            //llread();
            break;
        }
        break;
    case 2:

        //llopen

        break;
    }

    return 0;
}
//return -1 error or port case succeeded