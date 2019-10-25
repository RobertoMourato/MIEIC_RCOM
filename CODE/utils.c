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
#include "alarm.h"

void setLinkLayer(linkLayer *linkLayer,char port[]){
    strcpy(linkLayer->port,port);
    linkLayer->baudRate = BAUDRATE;
    linkLayer->sequenceNumber = 1; //TODO put smth here that i dont know 
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

char * makeDatePacket(char data[],int *dataPackLen,off_t filesize,linkLayer *linkLayer){

    *dataPackLen = 4 + strlen(data); // C+N+L1+L2
    char * dataPacket = (char *)malloc( *dataPackLen);

    dataPacket[0]= C1;
    dataPacket[1]= linkLayer->sequenceNumber % 255; //N – número de sequência (módulo 255)
    dataPacket[2]= (char) filesize/256; //L2 L1 – indica o número de octetos (K) do campo de dados (K = 256 * L2 + L1)
    dataPacket[3]= (char) filesize %256; //L2 L1 – indica o número de octetos (K) do campo de dados (K = 256 * L2 + L1)
    strcat((char*)dataPacket+5,data);//P1 ... PK – campo de dados do pacote (K octetos)
    
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


unsigned char BCC_make(char * buffer, int size)
{
    unsigned char BCC;
    BCC = buffer[0];
    for(int i = 1; i < size; i++)
    {
        BCC = BCC^buffer[i];
    }
    return BCC;
}

unsigned char * BCC_stuffing(unsigned char BCC)
{   
    //todo checka isto, porque nao sei o que queres guardar na string 
    unsigned char * BCC_stuffed = malloc(sizeof(char)); 
    if(BCC == ESC)
    {
        BCC_stuffed[0] = ESC;
        BCC_stuffed[1] = ESC_NEXT;
    }
    else if(BCC == FLAG)
    {
        BCC_stuffed[0] = ESC;
        BCC_stuffed[1] = FLAG_NEXT;
    }
    return BCC_stuffed;

}

unsigned char read_control_field(int fd)
{
    supervision_instance_data_t SU_state;
    SU_state.state = start;

    unsigned char part_of_frame;
    unsigned char control_field;


    while(flag && !(SU_state.state == stop))
    {
        read(fd,&part_of_frame,1);
        
        switch(SU_state.state)
        {
            case start:
                if(part_of_frame == FLAG)
                    SU_state.state = flag_rcv;
                break;

            case flag_rcv:
                if(part_of_frame == A_3)
                    SU_state.state = a_rcv;
                else if (part_of_frame == FLAG)
                    SU_state.state = flag_rcv;
                else
                {
                    SU_state.state = start;
                }
                break;
            case a_rcv:
                if(part_of_frame == SET || part_of_frame == DISC || part_of_frame == UA || part_of_frame == RR0 || part_of_frame == RR1 || part_of_frame == REJ0 || part_of_frame == REJ1)
                    {
                    control_field = part_of_frame;
                    SU_state.state = c_rcv;
                    }
                else
                    SU_state.state = start;
                break;
            case c_rcv:
                if(part_of_frame == (A_3 ^ control_field))
                    SU_state.state = bcc_ok;
                else
                {
                    SU_state.state = start;
                }
                break;
            case bcc_ok:
                if(part_of_frame == FLAG)
                    {
                        SU_state.state = stop;
                        return control_field;
                    }
                else
                {
                    SU_state.state = start;
                }
                break;
                
        }
    }
    printf("Error in reading control field");
    return NULL;
}