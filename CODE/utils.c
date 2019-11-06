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

extern int flag;

void setLinkLayer(linkLayer *linkLayer, char port[])
{
    strcpy(linkLayer->port, port);
    linkLayer->baudRate = BAUDRATE;
    linkLayer->sequenceNumber = 1; //TODO put smth here that i dont know
    linkLayer->timeout = TIMEOUT;
    linkLayer->numTransmissions = ATEMPTS;
}

unsigned char *makeControlPacket(int type, char path[], off_t filesize, int *controlPackLen)
{

    unsigned int L1 = ceil(log2(filesize) / 8);
    unsigned int L2 = strlen(path);
    *controlPackLen = 5 + L1 + L2; //5 = C + T1 + L1 + T2 + L2 + (char) path length is byte
    printf("control pack len  %d\n", 5 + L1 + L2);
    unsigned char *controlPacket = (unsigned char *)malloc(*controlPackLen);

    char c;
    switch (type)
    {
    case (START):
        c = C2;
        break;
    case (END):
        c = C3;
        break;
    }
    controlPacket[0] = c;  //C
    controlPacket[1] = T1; //T1
    controlPacket[2] = L1; //L1
    unsigned n = L1;
    for (unsigned int i = 0; i < L1; i++)
    { //V1
        n--;
        controlPacket[3 + i] = (filesize >> 8 * n) & 0xFF;
    }
    controlPacket[3 + L1] = T2;       //T2
    controlPacket[4 + L1] = (char)L2; //L2
    for (unsigned int i = 0; i < L2; i++)
    {
        controlPacket[5 + L1 + i] = path[i];
    }

    return controlPacket;
}

int sendDataPacket(int fd, linkLayer *linkLayer, const char *buffer, int length)
{

    unsigned int dataPackLen = 4 + length; // C+N+L1+L2

    unsigned char *dataPacket = (unsigned char *)malloc(dataPackLen);

    // build package header
    dataPacket[0] = C1;
    dataPacket[1] = linkLayer->sequenceNumber % 255; //N – número de sequência (módulo 255)
    dataPacket[2] = length / 256;                    //L2 L1 – indica o número de octetos (K) do campo de dados (K = 256 * L2 + L1)
    dataPacket[3] = length % 256;                    //L2 L1 – indica o número de octetos (K) do campo de dados (K = 256 * L2 + L1)

    // copy file chunk to package
    memcpy(&dataPacket[4], buffer, length);

    // write package
    if (llwrite(fd, dataPacket, dataPackLen) > 0)
    {
        printf("ERROR: Could not write to link layer while sending data package.\n"); //TODO mudar
        free(dataPacket);
        return 0;
    }
    free(dataPacket);
    //increment sequenceNumber
    linkLayer->sequenceNumber++;
    return 1;
}

void set_reception(supervision_instance_data_t *machine, unsigned char pack)
{
    switch (machine->state)
    {
    case (start):
        if (pack == FLAG)
            machine->state = flag_rcv;
        break;
    case (flag_rcv):
        if (pack == A_3)
        {
            machine->state = a_rcv;
            break;
        }
        if (pack != FLAG)
            machine->state = start;
        break;
    case (a_rcv):
        if (pack == SET)
        {
            machine->state = c_rcv;
            break;
        }
        if (pack == FLAG)
        {
            machine->state = flag_rcv;
            break;
        }
        machine->state = start;
        break;
    case (c_rcv):
        if (pack == (A_3 ^ SET))
        {
            machine->state = bcc_ok;
            break;
        }
        if (pack == FLAG)
        {
            machine->state = flag_rcv;
            break;
        }
        machine->state = start;
        break;
    case (bcc_ok):
        if (pack == FLAG)
        {
            machine->state = stop;
            break;
        }
        machine->state = start;
        break;
    case (stop):
        break;
    }
}

void ua_reception(supervision_instance_data_t *machine, unsigned char pack)
{
    switch (machine->state)
    {
    case (start):
        if (pack == FLAG)
            machine->state = flag_rcv;
        break;
    case (flag_rcv):
        if (pack == A_3)
        {
            machine->state = a_rcv;
            break;
        }
        if (pack != FLAG)
            machine->state = start;
        break;
    case (a_rcv):
        if (pack == UA)
        {
            machine->state = c_rcv;
            break;
        }
        if (pack == FLAG)
        {
            machine->state = flag_rcv;
            break;
        }
        machine->state = start;
        break;
    case (c_rcv):
        if (pack == (A_3 ^ UA))
        {
            machine->state = bcc_ok;
            break;
        }
        if (pack == FLAG)
        {
            machine->state = flag_rcv;
            break;
        }
        machine->state = start;
        break;
    case (bcc_ok):
        if (pack == FLAG)
        {
            machine->state = stop;
            break;
        }
        machine->state = start;
        break;
    case (stop):
        break;
    }
}

void disc_reception(supervision_instance_data_t *machine, unsigned char pack)
{
    switch (machine->state)
    {
    case (start):
        if (pack == FLAG)
            machine->state = flag_rcv;
        break;
    case (flag_rcv):
        if (pack == A_3)
        {
            machine->state = a_rcv;
            break;
        }
        if (pack != FLAG)
            machine->state = start;
        break;
    case (a_rcv):
        if (pack == DISC)
        {
            machine->state = c_rcv;
            break;
        }
        if (pack == FLAG)
        {
            machine->state = flag_rcv;
            break;
        }
        machine->state = start;
        break;
    case (c_rcv):
        if (pack == (A_3 ^ DISC))
        {
            machine->state = bcc_ok;
            break;
        }
        if (pack == FLAG)
        {
            machine->state = flag_rcv;
            break;
        }
        machine->state = start;
        break;
    case (bcc_ok):
        if (pack == FLAG)
        {
            machine->state = stop;
            break;
        }
        machine->state = start;
        break;
    case (stop):
        break;
    }
}

unsigned char BCC_make(unsigned char *buffer, int size)
{
    unsigned char BCC;
    BCC = buffer[0];
    for (int i = 1; i < size; i++)
    {
        BCC = BCC ^ buffer[i];
    }
    return BCC;
}

unsigned char *BCC_stuffing(unsigned char BCC)
{
    unsigned char *BCC_stuffed = malloc(sizeof(char));
    if (BCC == ESC)
    {
        BCC_stuffed = (unsigned char *)realloc(BCC_stuffed, 2);
        BCC_stuffed[0] = ESC;
        BCC_stuffed[1] = ESC_NEXT;
    }
    else if (BCC == FLAG)
    {
        BCC_stuffed = (unsigned char *)realloc(BCC_stuffed, 2);
        BCC_stuffed[0] = ESC;
        BCC_stuffed[1] = FLAG_NEXT;
    }
    else
    {
        BCC_stuffed[0] = BCC;
    }
    return BCC_stuffed;
}

unsigned char read_control_field(int fd)
{
    supervision_instance_data_t SU_state;
    SU_state.state = start;

    unsigned char part_of_frame;
    unsigned char control_field;

    while (!flag && !(SU_state.state == stop))
    {
        read(fd, &part_of_frame, 1);

        switch (SU_state.state)
        {
        case start:
            if (part_of_frame == FLAG)
                SU_state.state = flag_rcv;
            break;

        case flag_rcv:
            if (part_of_frame == A_3)
                SU_state.state = a_rcv;
            else if (part_of_frame == FLAG)
                SU_state.state = flag_rcv;
            else
            {
                SU_state.state = start;
            }
            break;
        case a_rcv:
            if (part_of_frame == SET || part_of_frame == DISC || part_of_frame == UA || part_of_frame == RR0 || part_of_frame == RR1 || part_of_frame == REJ0 || part_of_frame == REJ1)
            {
                control_field = part_of_frame;
                SU_state.state = c_rcv;
            }
            else
                SU_state.state = start;
            break;
        case c_rcv:
            if (part_of_frame == (A_3 ^ control_field))
                SU_state.state = bcc_ok;
            else
            {
                SU_state.state = start;
            }
            break;
        case bcc_ok:
            if (part_of_frame == FLAG)
            {
                SU_state.state = stop;
                return control_field;
            }
            else
            {
                SU_state.state = start;
            }
            break;
        case stop:
            break;
        }
    }
    printf("Error in reading control field");
    //cant return null in C
    return '0';
}

//TODO Roberto

unsigned char *startFileName(unsigned char *start)
{
    int aux = (int)start[8];
    int i = 0;
    unsigned char *fileName = (unsigned char *)malloc(aux + 1);

    while (i < aux)
    {
        fileName[i] = start[i + 9];
        i++;
    }
    fileName[aux] = '\0';
    return fileName;
}

void setThingsFromStart(off_t *sizeOfAllMessages, unsigned char *fileName, unsigned char *startTransmition)
{
    int aux = (int)startTransmition[2];
    int namesize = 0;
    unsigned char *auxSize = malloc(aux);
    unsigned char *auxName;
    //Get filesize
    if (startTransmition[1] == T1)
    {
        int j = aux - 1;
        for (int i = 0; i < aux; i++)
        {
            auxSize[i] = startTransmition[3 + j];
            j--;
        }
    }

    //Get filename
    if (startTransmition[4 + aux - 1] == T2)
    {
        namesize = (int)startTransmition[5 + aux - 1];
        auxName = malloc(namesize);
        for (int i = 0; i < namesize; i++)
        {
            auxName[i] = startTransmition[6 + aux - 1 + i];
        }
    }

    //set variables
    memcpy(sizeOfAllMessages, (off_t *)auxSize, aux);
    fileName = realloc(fileName, namesize);
    memcpy(fileName, auxName, namesize);
}

int endReached(unsigned char *message, int sizeOfMessage, unsigned char *startTransmition, int sizeOfStartTransmition)
{
    if (sizeOfMessage != sizeOfStartTransmition)
    {
        return FALSE;
    }
    else
    {
        if (message[0] == C3)
        {
            for (size_t i = 1; i < (size_t)sizeOfMessage; i++)
            {
                if (message[i] != startTransmition[i])
                {
                    return FALSE;
                }
            }
        }
        else
            return FALSE;
    }
    return TRUE;
}

unsigned char *headerRemoval(unsigned char *message, int sizeOfMessage)
{

    unsigned char *aux = (unsigned char *)malloc(sizeOfMessage - 4);

    for (size_t i = 4; i < (size_t)sizeOfMessage; i++)
    {
        aux[i - 4] = message[i];
    }

    return aux;
}

int checkBcc2(unsigned char *buffer, int sizeBuffer)
{
    unsigned char Bcc2 = buffer[0];
    for (int i = 1; i < sizeBuffer - 1; i++)
    {
        Bcc2 ^= buffer[i];
    }
    if (Bcc2 == buffer[sizeBuffer - 1])
    {
        return TRUE;
    }
    else
        return FALSE;
}

void sendControlMessage(int fd, unsigned char controlField)
{
    unsigned char message[5];
    message[0] = FLAG;
    message[1] = A_3;
    message[2] = controlField;
    message[3] = message[1] ^ message[2];
    message[4] = FLAG;
    write(fd, message, sizeof(message));
}

void print_buf(const char *title, unsigned char *buf, size_t buf_len)
{
    size_t i = 0;
    fprintf(stdout, "%s\n", title);
    for (i = 0; i < buf_len; ++i)
        fprintf(stdout, "%02X%s", buf[i], (i + 1) % 16 == 0 ? "\r\n" : " ");
    printf("\n");
}
