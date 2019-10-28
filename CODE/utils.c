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

unsigned char *makeDatePacket(char data[], int *dataPackLen, off_t filesize, linkLayer *linkLayer)
{

    *dataPackLen = 4 + strlen(data); // C+N+L1+L2
    unsigned char *dataPacket = (unsigned char *)malloc(*dataPackLen);

    dataPacket[0] = C1;
    dataPacket[1] = linkLayer->sequenceNumber % 255; //N – número de sequência (módulo 255)
    dataPacket[2] = (int)filesize / 256;            //L2 L1 – indica o número de octetos (K) do campo de dados (K = 256 * L2 + L1)
    dataPacket[3] = (int)filesize % 256;            //L2 L1 – indica o número de octetos (K) do campo de dados (K = 256 * L2 + L1)
    for (int i = 0; i < MAX_SIZE; i++)
    {
        dataPacket[4+i]=data[i];
    }

    //increment sequenceNumber
    linkLayer->sequenceNumber++;

    return dataPacket;
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
        printf("start\n");
        if (pack == FLAG)
            machine->state = flag_rcv;
        break;
    case (flag_rcv):
        printf("flag\n");
        if (pack == A_3)
        {
            machine->state = a_rcv;
            break;
        }
        if (pack != FLAG)
            machine->state = start;
        break;
    case (a_rcv):
        printf("a\n");
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
        printf("c\n");
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
        printf("bcc\n");
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
    //todo checka isto, porque nao sei o que queres guardar na string
    unsigned char *BCC_stuffed = malloc(sizeof(char));
    if (BCC == ESC)
    {
        BCC_stuffed = (unsigned char*) realloc(BCC_stuffed, 2);
        BCC_stuffed[0] = ESC;
        BCC_stuffed[1] = ESC_NEXT;
    }
    else if (BCC == FLAG)
    {   
        BCC_stuffed = (unsigned char*) realloc(BCC_stuffed, 2);
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
        print_buf(" ",&part_of_frame,1);

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

//Roberto

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
    int aux = (int)startTransmition[8];
    int i = 0;
    unsigned char *fileNameAux = (unsigned char *)malloc(aux + 1);

    while (i < aux)
    {
        fileNameAux[i] = startTransmition[i + 9];
        i++;
    }
    fileNameAux[aux] = '\0';

    *fileName = *fileNameAux;
    *sizeOfAllMessages = (off_t)(startTransmition[3] << 24) | (startTransmition[4] << 16) | (startTransmition[5] << 8) | (startTransmition[6]);
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
            for (size_t i = 1; i <= (size_t)sizeOfMessage; i++)
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

unsigned char *headerRemoval(unsigned char *message, int sizeOfMessage, int *sizeWithNoHeader)
{

    unsigned char *aux = (unsigned char *)malloc(sizeOfMessage - 4);

    for (size_t i = 0; (i + 4) < (size_t)sizeOfMessage; i++)
    {
        aux[i] = message[i + 4];
    }

    *sizeWithNoHeader = sizeOfMessage - 4;
    return aux;
}

int checkBCC2(unsigned char *message, int sizeMessage)
{
    int i = 1;
    unsigned char BCC2 = message[0];
    for (; i < sizeMessage - 1; i++)
    {
        BCC2 ^= message[i];
    }
    if (BCC2 == message[sizeMessage - 1])
    {
        return TRUE;
    }
    else
        return FALSE;
}

void sendControlMessage(int fd, unsigned char C)
{
    unsigned char message[5];
    message[0] = FLAG;
    message[1] = A_3;
    message[2] = C;
    message[3] = message[1] ^ message[2];
    message[4] = FLAG;
    print_buf("Sent control message: ", message, sizeof(message));
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
