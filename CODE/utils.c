#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"
#include "error.h"
#include "DataLayer.h"

void setLinkLayer(linkLayer *linkLayer,char port[]){
    strcpy(linkLayer->port,port);
    linkLayer->baudRate = BAUDRATE;
    linkLayer->sequenceNumber = SEQNUM; //TODO put smth here that i dont know 
    linkLayer->timeout = TIMEOUT;
    linkLayer->numTransmissions = ATEMPTS;
}

char * makeControlPacket(int type, char path[],off_t filesize,int *controlPackLen){
    
    unsigned int L1 = sizeof(filesize);
    unsigned int L2 = strlen(path);

    *controlPackLen = 5 + L1 + L2; //5 = C + T1 + L1 + T2 + L2 + (char) path length is byte 
    char * controlPacket = (char *)malloc(*controlPackLen);

    char c; 
    switch(type){
        case(START):
            c= C2;
        break;
        case(END):
            c= C3;
        break;
    }
    controlPacket[0] = c;  //C
    controlPacket[1] = T1;  //T1
    controlPacket[2] = L1;  //L1
     *(off_t*)(controlPacket + 3 )= filesize; //V1 
    controlPacket[3+L1] = T2;  //T2
    controlPacket[4+L1] = (char) L2; //L2
    strcat((char*)controlPacket+5+L1,path); //V2

    return controlPacket;
}

char * makeDatePacket(char data[],int *dataPackLen){

    *dataPackLen = 6 + strlen(data); // C+N+L2+L1+P1+P2
    char * dataPacket = (char *)malloc( *dataPackLen);

    dataPacket[0]= C1;
    dataPacket[1]= 0x01; //N – número de sequência (módulo 255)
    dataPacket[2]= 0x01; //L2 L1 – indica o número de octetos (K) do campo de dados (K = 256 * L2 + L1)
    dataPacket[3]= 0x01; //L2 L1 – indica o número de octetos (K) do campo de dados (K = 256 * L2 + L1)
    dataPacket[4]= 0x01;//P1 ... PK – campo de dados do pacote (K octetos)
    strcat((char*)dataPacket+5,data);
    dataPacket[5+strlen(data)] =0x01;//P1 ... PK – campo de dados do pacote (K octetos)

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

void disc_reception(supervision_instance_data_t *machine, unsigned char pack){
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