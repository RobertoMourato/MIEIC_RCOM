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
{   //run console UI
    interface();
    return 0;
}

int interface()
{
    char buf[255]; //to read from console 
    int openType = -1; //sender / receiver
    int option = -1; //interface selector
    int portNr = -1; //port number

    int open_flag = FALSE; //flag to know if port is oppended
    int running_flag = TRUE; //flag to make program stop

    //smth like a loop, inteface polling
    while (running_flag)
    {
        //draw UI
        printf("\n============================\n");
        printf("Select option:\n");
        printf("0 - llopen()\n");
        printf("1 - llwrite/read()\n");
        printf("2 - llclose()\n");
        printf("============================\n");
        printf("Type: ");
        //get UI option
        scanf("%s", buf);
        option = atoi(buf);

        switch (option)
        {
        case 0:
            if (!open_flag)
            {   
                //enter port number
                printf("============================\n");
                printf("Port number? ");
                scanf("%s", buf);
                portNr = atoi(buf);
                if (portNr != 0 && portNr != 1)
                {
                    printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
                    exit(ERR_PRT);
                }
                //choose if transmotter or receiver 
                printf("\n0 - Transmiter/ 1 - Receiver: ");
                scanf("%s", buf);
                openType = atoi(buf);
                if (openType != 0 && openType != 1)
                {
                    printf("0 - TRANSMITTER/ 1 - RECEIVER!");
                    exit(ERR_ARGV2);
                }
                printf("============================\n\n");
                //save app status
                if (openType == 0)
                    app.status = TRANSMITTER;
                else
                    app.status = RECEIVER;
                //do llopen and store on descripter
                app.fileDescriptor = llopen(portNr, openType);
                if(app.fileDescriptor < 0 )
                    exit(ERR_LLOPEN);
                //set port as oppenned
                open_flag = TRUE;
            }
            else
                printf("Connection has already been oppened...");
            break;
        case 1:
            switch (app.status)
            {
                char path[255];

            case (TRANSMITTER):
                printf("Enter file path to read: ");
                scanf("%s", path);


                //llwrite();
                break;
            case (RECEIVER):
                //llread();
                break;
            }
            break;
        case 2:
            if(llclose(app.fileDescriptor) > 0){
                printf("Port sucssefully closed!");
                running_flag = FALSE; //i dont know if it is suppose the program to end ask later
            }else printf("Port didnt close!");
            break;
        }
    }

    printf("\nProgram sucssefully ended.\n");
    return 0;
}
//return -1 error or port case succeeded