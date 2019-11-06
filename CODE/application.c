#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

#include "utils.h"
#include "error.h"
#include "DataLayer.h"
#include "application.h"

//GLOBALS
applicationLayer app;
extern linkLayer layerPressets;

unsigned int numMsg = 0;

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
                {
                    printf("Error Openning!");
                    exit(ERR_LLOPEN);
                }
                //set port as oppenned
                open_flag = TRUE;
            }
            else
                printf("Connection has already been oppened...");
            break;
        case 1:
            switch (app.status)
            {
                //vars TRANSMITTER
                int res;
                char path[255];
                struct stat metadata;
                FILE *f;
                unsigned char *controlPack;

                int eof;

            case (TRANSMITTER):

                //UI
                printf("============================\n");
                printf("Enter file path to read: ");

                //aplication level, has to read the file, split the file, make the frames and set packet header
                scanf("%s", path);
                printf(" path size: %ld\n", strlen(path));

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

                if (res < 0)
                {
                    printf("Error reading file!\n");
                }
                else
                    printf("File read has %d Bytes\n ", res);

                int controlPackSize = 0;

                controlPack = makeControlPacket(START, path, metadata.st_size, &controlPackSize);

                //START WRITTING STUFF

                printf("Writing Start Control pack...\n");
                if (llwrite(app.fileDescriptor, controlPack, controlPackSize) != 0)
                {
                    printf("Error writting start control packet\n");
                    return -1;
                }

                //while splitting the file
                char *filePack = malloc(MAX_SIZE);
                // read file chunks
                unsigned int readBytes = 0, writtenBytes = 0;
                while ((readBytes = fread(filePack, sizeof(char), MAX_SIZE, f)) > 0)
                {
                    printf("readbytes %d\n",readBytes);
                    // send those chunks inside data packages
                    if (!sendDataPacket(app.fileDescriptor, &layerPressets, filePack, readBytes))
                    {
                        free(filePack);
                        return 0;
                    }

                    // reset file buffer
                    filePack = memset(filePack, 0, MAX_SIZE);

                    // increment no. of written bytes
                    writtenBytes += readBytes;

                }

                free(filePack);

                 //close file
                res = fclose(f);
                if (res != 0)
                {
                    printf("Error closing file!\n");
                    return -1;
                }
                else
                    printf("File closed...\n");

                //send PH Data start
                controlPackSize = 0;
                controlPack = makeControlPacket(END, path, metadata.st_size, &controlPackSize);
                //printf("size %d\n", controlPackSize);
                if (llwrite(app.fileDescriptor, controlPack, controlPackSize) == -1)
                {
                    printf("Error writting start control packet");
                    return -1;
                }
                //if any of this fails, process will end
                printf("ive sent %d messages\n", numMsg);
                break;
            case (RECEIVER):

                eof = FALSE;

                int sizeOfStartTransmition = 0;
                int sizeOfMessage = 0;
                off_t sizeOfAllMessages = 0;
                off_t aux = 0;

                unsigned char *startTransmition = (unsigned char *)malloc(0);
                unsigned char *fileName = (unsigned char *)malloc(0);

                sizeOfStartTransmition = llread(app.fileDescriptor, startTransmition);
                printf("Read message with %d\n", sizeOfStartTransmition);

                setThingsFromStart(&sizeOfAllMessages, fileName, startTransmition);
                printf("sizeOfAllMessages: %ld\n", sizeOfAllMessages);
                printf("fileName: %s\n", fileName);

                //printf("Read message with %d\n", sizeOfStartTransmition);
                //print_buf("message with header",startTransmition,sizeOfStartTransmition);

                unsigned char *allMessages = (unsigned char *)malloc(sizeOfAllMessages);

                while (!eof)
                {
                    unsigned char *message = (unsigned char *)malloc(0);
                    sizeOfMessage = llread(app.fileDescriptor, message);
                    printf("Read message with %d\n", sizeOfMessage);
                    //print_buf("message with header",message,sizeOfMessage);

                    if (sizeOfMessage == 0)
                    {
                        continue; 
                    }

                    if (endReached(message, sizeOfMessage, startTransmition, sizeOfStartTransmition))
                    {
                        printf("End Reached!\n");
                        free(startTransmition);                        
                        break;
                    }

                    message = headerRemoval(message, sizeOfMessage);
                    sizeOfMessage = sizeOfMessage - 4;

                    //print_buf("New message after header removal",message,sizeOfMessage);
                    /*for(int i =0; i<sizeWithNoHeader; i++){
                        allMessages[i+aux]= message[i];
                    }*/
                    memcpy(allMessages + aux, message, sizeOfMessage);
                    aux += sizeOfMessage;
                    printf("aux %ld\n", aux);
                    numMsg++;
                    free(message);
                }
                
                printf("fileName: %s\n", fileName);
                //printf("allMessages: %s\n",allMessages);
                //print_buf("data",allMessages,sizeOfAllMessages);

                FILE *file = fopen((char *)fileName, "wb+");

                free(fileName);

                if (file == NULL)
                {
                    printf("Error oppenning file to write!\n");
                    return -1;
                }
                fwrite((void *)allMessages, 1, (off_t)sizeOfAllMessages, file);

                free(allMessages);
                printf("sizeOfAllMessages: %ld\n", sizeOfAllMessages);
                printf("New file created\n");
                res = fclose(file);
                if (res != 0)
                {
                    printf("Error closing file!\n");
                    return -1;
                }
                printf("ive read %d messages\n", numMsg);
            }
            break;
        case 2:
            if (llclose(app.fileDescriptor) > 0)
            {
                printf("Port sucssefully closed!");
                running_flag = FALSE;
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
