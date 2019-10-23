#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "utils.h"
#include "error.h"
#include "DataLayer.h"
#include "application.h"

//GLOBALS
applicationLayer app;

int main(void)
{ //run console UI
    interface();
    return 0;
}

int interface()
{
    char buf[255];     //to read from console
    int openType = -1; //sender / receiver
    int option = -1;   //interface selector
    int portNr = -1;   //port number

    int open_flag = FALSE;   //flag to know if port is oppended
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
                if (app.fileDescriptor < 0)
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
                //vars
                int res;
                char path[255];
                struct stat metadata;
                FILE *f;
                unsigned char *fileData;

                char *controlPack;

            case (TRANSMITTER):

                //UI
                printf("============================\n");
                printf("Enter file path to read: ");

                //aplication level, has to read the file, split the file, make the frames and set packet header
                scanf("%s", path);
                printf(" size: %ld\n", strlen(path));

                //open file
                f = fopen(path, "rb");
                if (f == NULL)
                {
                    printf("Error opening image!\n");
                    return -1;
                }
                //get img size value
                stat(path, &metadata);
                printf("This file has %ld bytes \n", metadata.st_size);
                //alloc memory to store the image
                fileData = (unsigned char *)malloc(metadata.st_size);
                res = fread(fileData, sizeof(unsigned char), metadata.st_size, f);
                if (res < 0)
                {
                    printf("Error reading file!\n");
                }
                else
                    printf("File read has %d coiso\n ", res);

                //close file
                res = fclose(f);
                if (res != 0)
                {
                    printf("Error closing file!\n");
                    return -1;
                }

                printf("res: %d\n", ((int)metadata.st_size) / MAX_SIZE);
                //send PH Data start
                int controlPackSize = 0;
                controlPack = makeControlPacket(START, path, metadata.st_size, &controlPackSize);
                printf("size %d\n", controlPackSize);
                if (llwrite(app.fileDescriptor, controlPack, controlPackSize) == -1)
                {
                    printf("Error writting start control packet");
                    return -1;
                }

                //while splitting the file
                for (int i = 0; i < ((int)metadata.st_size) / MAX_SIZE + 1; i++)
                {
                    //split data to send
                    char *tmpPack;
                    tmpPack = (char *)malloc(MAX_SIZE);
                    for (int j = 0; j <= MAX_SIZE; j++)
                    {
                        tmpPack[j] = fileData[i * MAX_SIZE + j];
                    }
                    int dataPackSize = 0;
                    char *dataPack = makeDatePacket(tmpPack, &dataPackSize);
                    if (llwrite(app.fileDescriptor, dataPack, dataPackSize) == -1)
                    { //wait for the return value
                        printf("Error writting mid control packet");
                        return -1;
                    }
                }
                //send PH Data start
                controlPackSize = 0;
                controlPack = makeControlPacket(END, path, metadata.st_size, &controlPackSize);
                printf("size %d\n", controlPackSize);
                if (llwrite(app.fileDescriptor, controlPack, controlPackSize) == -1)
                {
                    printf("Error writting start control packet");
                    return -1;
                }
                //if any of this fails, process will end

                break;
            case (RECEIVER):
                //remove nd read flags created on the case before and store the date on a file
                //TODO penso que e suposto fazer leitura bit a bit mas eu estou super casado ja nao dao mais...
                /*
                int eof = FALSE, res, len;
                char *filename, buffer;

                while (!eof)
                {
                    //keep reading until gets...
                    len = llread(app.fileDescriptor, buffer);
                    if (len < 0)
                    {
                        printf("Error reading file");
                        return -1;
                    }
                    switch ((buffer + 0))
                    {
                    case CSTART:
                        if((buffer+1) == T1){
                            filename = (char *)malloc(());
                        }
                        if((buffer+1) == T2){
                            filename = (char *)malloc(());
                        }
                        break;
                    case CDATA:
                    //nao sei manipular bem estes
                        break;
                    case CEND:
                        eof = FALSE;
                        break;
                    }
                }
                */
                break;
            }
            break;
        case 2:
            if (llclose(app.fileDescriptor) > 0)
            {
                printf("Port sucssefully closed!");
                running_flag = FALSE; //i dont know if it is suppose the program to end ask later
            }
            else
                printf("Port didnt close!");
            break;
        }
    }

    printf("\nProgram sucssefully ended.\n");
    return 0;
}
//return -1 error or port case succeeded