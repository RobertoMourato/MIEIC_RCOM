#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include "utils.h"
#include "error.h"
#include "DataLayer.h"
#include "alarm.h"

//*GLOBALS
linkLayer layerPressets;
//Frames (S)(U)
unsigned char set[5] = {FLAG, A_3, SET, A_3 ^ SET, FLAG};
unsigned char ua[5] = {FLAG, A_3, UA, A_3 ^ UA, FLAG};
unsigned char disc[5] = {FLAG, A_3, DISC, A_3 ^ DISC, FLAG};
//unsigned char ack[5] = {FLAG, A_3, RR, A_3 ^ RR, FLAG};
//unsigned char nack[5] = {FLAG, A_3, REJ, A_3 ^ REJ, FLAG};

int num_frame = 0;

int portState;

struct termios oldtio;
//set Layer values
supervision_instance_data_t machineOpenTransmitter = {start};
supervision_instance_data_t machineOpenReceiver = {start};
supervision_instance_data_t machineCloseTransmitter = {start};
supervision_instance_data_t machineCloseReceiver = {start};

//TRANSMITTER | RECEIVER
int llopen(int port, int type)
{
    int fd, res;
    struct termios newtio;
    char buf[255];
    char portStr[20] = "/dev/ttyS"; //to append to the port number
    char portNr = port + '0';
    puts(&portNr);
    strcat(portStr, &portNr);
    //set layer pressets using flags defined
    setLinkLayer(&layerPressets, portStr);
    /*
    Open serial port device for reading and writing and not as controlling tty
    because we don't want to get killed if linenoise sends CTRL-C.
    */

    fd = open(portStr, O_RDWR | O_NOCTTY);
    if (fd < 0)
    {
        perror(portStr);
        exit(ERR_RDWR);
    }

    if (tcgetattr(fd, &oldtio) == -1)
    { /* save current port settings */
        perror("tcgetattr");
        exit(-1);
    }

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    /* set input mode (non-canonical, no echo,...) */
    newtio.c_lflag = 0;

    newtio.c_cc[VTIME] = 1; /* inter-character timer unused */
    newtio.c_cc[VMIN] = 0;  /* blocking read until 5 chars received */

    /* 
    VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a 
    leitura do(s) pr�ximo(s) caracter(es)
  */

    tcflush(fd, TCIOFLUSH);

    if (tcsetattr(fd, TCSANOW, &newtio) == -1)
    {
        perror("tcsetattr");
        exit(-1);
    }

    printf("New termios structure set\n");

    switch (type)
    {
    case TRANSMITTER:
        //Install alarm
        portState = TRANSMITTER;
        (void)signal(SIGALRM, alarm_handler);

        while (attempt < layerPressets.numTransmissions)
        {
            if (flag)
            {
                //SENDING SET
                printf("attempt %d...\n",attempt);
                printf("Sending SET...\n");
                print_buf("SET",set,5);
                res = write(fd, set, 5);
                if (res < 0)
                    exit(ERR_WR);

                alarm(layerPressets.timeout);
                flag = 0;

                printf("Receiving UA...\n");
                for (int i = 0; i < 5; i++)
                {
                    res = read(fd, &buf[i], 1);
                    ua_reception(&machineOpenTransmitter, buf[i]);
                    if (machineOpenTransmitter.state == stop)
                        printf("Succsefully passed UA\n");
                }
                if (machineOpenTransmitter.state == stop)
                {
                    turnoff_alarm();
                    //alarm(0);
                    printf("Passei corretamente!\n");
                    return 0;
                }
            }
        }
        return -1;
        break;
    case RECEIVER:
        //RECEIVE  SET - AND CHECK
        portState = RECEIVER;
        printf("Receiving SET...\n");
        while (machineOpenReceiver.state != stop)
        {
            for (int i = 0; i < 5; i++)
            {
                res = read(fd, &buf[i], 1);
                if (res < 0)
                    exit(ERR_RD);
                set_reception(&machineOpenReceiver, buf[i]);
                if (machineOpenReceiver.state == stop)
                {
                    printf("Succsefully passed SET\n");
                }
            }
        }

        //SEND UA
        printf("Sending UA...\n");
        print_buf("UA",ua,5);
        res = write(fd, ua, 5);
        if (res < 0)
            exit(ERR_WR);
        break;
    }

    //make structure
    return fd;
}

//PROTIPO DO ANDY
int llwrite(int fd, unsigned char *buffer, int length)
{
    print_buf("Received: ",buffer,length);
    int res;
    unsigned char *buf[255];
    unsigned char BCC_data;
    int data_frame_size = length + 6;
    unsigned char *BCC_data_stuffed = (unsigned char *)malloc((sizeof(unsigned char)) * 2);
    unsigned char *data_frame = (unsigned char *)malloc((sizeof(unsigned char)) * data_frame_size);
    BCC_data = BCC_make(buffer, length);
    BCC_data_stuffed = BCC_stuffing(BCC_data);

    data_frame[0] = FLAG;
    data_frame[1] = A_3;

    if (num_frame == 0)
    {
        printf("R(0)\n");
        data_frame[2] = NS_0;
    }
    else
    {
        printf("R(1)\n");
        data_frame[2] = NS_1;
    }

    data_frame[3] = (A_3 ^ data_frame[2]);

    int data_frame_inside_counter = 4;

    printf("Stuffing Bytes...\n");
    for (int i = 0; i < length; i++)
    {

        if (buffer[i] == ESC) //byte stuffing
        {
            data_frame = (unsigned char *)realloc(data_frame, (sizeof(unsigned char *)) * (data_frame_size++));
            data_frame[data_frame_inside_counter] = ESC;
            data_frame[data_frame_inside_counter + 1] = ESC_NEXT;

            data_frame_inside_counter++;
            data_frame_inside_counter++;
        }
        else if (buffer[i] == FLAG)
        {
            data_frame = (unsigned char *)realloc(data_frame, (sizeof(unsigned char *)) * (data_frame_size++));
            data_frame[data_frame_inside_counter] = ESC;
            data_frame[data_frame_inside_counter + 1] = FLAG_NEXT;

            data_frame_inside_counter++;
            data_frame_inside_counter++;
        }
        else
        {
            data_frame[data_frame_inside_counter] = buffer[i];
        }
    }
    if (strlen((char *)BCC_data_stuffed) == 1)
    {
        data_frame[data_frame_size - 1] = BCC_data;
    }
    else if (strlen((char *)BCC_data_stuffed) == 2)
    {
        data_frame = (unsigned char *)realloc(data_frame, (sizeof(unsigned char)) * (data_frame_size++));
        data_frame[data_frame_size - 2] = BCC_data_stuffed[0];
        data_frame[data_frame_size - 1] = BCC_data_stuffed[1];
    }

    data_frame[data_frame_size] = FLAG;

    printf("data frame size: %ld\n", sizeof(data_frame));
    print_buf("to pass: ",data_frame,sizeof(data_frame));

    (void)signal(SIGALRM, alarm_handler);

    //install alarm
   while(attempt < layerPressets.numTransmissions){
        if(flag){                                                                   // this flag is not from this function I think
            printf("Writing data_frame");
            res = write(fd, data_frame, sizeof(data_frame));
            if( res < 0){
                printf("Error writting to port!\n");
                return -1;
            }
            printf("going to read control field sent from read");
            unsigned char control_field = read_control_field(fd);
            alarm(layerPressets.timeout);
            flag = 0;
            if(data_frame[2]== NS_0 && control_field == RR1 || data_frame[2] == NS_1 && control_field == RR0 )
            {   
                printf("control field from write and read compatable");
                num_frame = num_frame ^ 1;
                alarm(0);
                return 0;
            }            
            //read the ACK or NACK choose what to do!
            
        }
    }

        //return 0 when succsefull
    }
    printf("ola\n");

    return -1;
}

int llread(int fd, unsigned char *buffer)
{
    int *sizeBuffer = 0;
    unsigned char packectReaded;
    unsigned char packet;
    int auxTrama = 0;
    int sendData = FALSE;
    int state = 0;

    while (state != 6)
    {
        printf("Reading...\n");
        read(fd, &packet, 1);
        switch (state)
        {
        case 0:
            if (packet == FLAG)
                state = 1;
            break;
        case 1:
            if (packet == A_3)
                state = 2;
            else
            {
                if (packet == FLAG)
                    state = 1;
                else
                    state = 0;
            }
            break;
        case 2:
            if (packet == NS_0)
            {
                auxTrama = 0;
                packectReaded = packet;
                state = 3;
            }
            else if (packet == NS_1)
            {
                auxTrama = 1;
                packectReaded = packet;
                state = 3;
            }
            else
            {
                if (packet == FLAG)
                    state = 1;
                else
                    state = 0;
            }
            break;
        case 3:
            if (packet == (A_3 ^ packectReaded))
                state = 4;
            else
                state = 0;
            break;
        case 4:
            if (packet == FLAG)
            {
                if (checkBCC2(buffer, *sizeBuffer))
                {
                    if (auxTrama == 0)
                        sendControlMessage(fd, RR1);
                    else
                        sendControlMessage(fd, RR0);

                    state = 6;
                    sendData = TRUE;
                    printf("Enviou RR, T: %d\n", auxTrama);
                }
                else
                {
                    if (auxTrama == 0)
                        sendControlMessage(fd, REJ1);
                    else
                        sendControlMessage(fd, REJ0);
                    state = 6;
                    sendData = FALSE;
                    printf("Enviou REJ, T: %d\n", auxTrama);
                }
            }
            else if (packet == ESC)
            {
                state = 5;
            }
            else
            {
                buffer = (unsigned char *)realloc(buffer, ++(*sizeBuffer));
                buffer[*sizeBuffer - 1] = packet;
            }
            break;
        case 5:
            //printf("5state\n");
            if (packet == FLAG_NEXT)
            {
                buffer = (unsigned char *)realloc(buffer, ++(*sizeBuffer));
                buffer[*sizeBuffer - 1] = FLAG;
            }
            else
            {
                if (packet == ESC_NEXT)
                {
                    buffer = (unsigned char *)realloc(buffer, ++(*sizeBuffer));
                    buffer[*sizeBuffer - 1] = ESC;
                }
                else
                {
                    perror("Non valid character after escape character");
                    exit(-1);
                }
            }
            state = 4;
            break;
        }
    }

    printf("Message size: %d\n", *sizeBuffer);
    //message tem BCC2 no fim
    buffer = (unsigned char *)realloc(buffer, *sizeBuffer - 1);

    *sizeBuffer = *sizeBuffer - 1;
    if (sendData)
    {
        if (auxTrama == num_frame)
        {
            num_frame ^= 1;
        }
        else
            *sizeBuffer = 0;
    }
    else
        *sizeBuffer = 0;
    return *sizeBuffer;
}

int llclose(int fd)
{
    int res;
    char buf[255];
    int flag = TRUE;

    //flush
    tcflush(fd, TCIOFLUSH);

    switch (portState)
    {
    case (TRANSMITTER):

        (void)signal(SIGALRM, alarm_handler);

        while (attempt < layerPressets.numTransmissions)
        {
            if (flag)
            {
                //Send DISC
                printf("Sending DISC...\n");
                res = write(fd, disc, 5);
                if (res < 0)
                    exit(ERR_WR);

                alarm(layerPressets.timeout);
                flag = 0;

                //WAIT FOR DISC ACK
                printf("Receiving RECEIVER DISC...\n");
                for (int i = 0; i < 5; i++)
                {
                    res = read(fd, &buf[i], 1);
                    disc_reception(&machineCloseTransmitter, buf[i]);
                    if (machineCloseTransmitter.state == stop)
                    {
                        turnoff_alarm();
                        printf("Succsefully passed DISC\n");
                        //SEND UA
                        break;
                    }
                }
            }
            printf("Sending UA...\n");
            res = write(fd, disc, 5);
            if (res < 0)
                exit(ERR_WR);
        }
        break;

    case (RECEIVER):

        //READ DISC
        printf("Reading DISC...\n");
        for (int i = 0; i < 5; i++)
        {
            res = read(fd, &buf[i], 1);
            disc_reception(&machineCloseReceiver, buf[i]);
            if (machineCloseReceiver.state == stop)
                printf("Succsefully passed DISC\n");
        }
        //SEND DISC
        res = write(fd, disc, 5);
        if (res < 0)
            exit(ERR_WR);

        //reset machine
        machineCloseReceiver.state = start;

        //READ UA
        printf("Reading UA...\n");
        for (int i = 0; i < 5; i++)
        {
            res = read(fd, &buf[i], 1);
            ua_reception(&machineCloseReceiver, buf[i]);
            if (machineCloseReceiver.state == stop)
                printf("Succsefully passed UA\n");
        }

        break;
    }

    sleep(1);
    //disconnect port
    if (tcsetattr(fd, TCSANOW, &oldtio) == -1)
    {
        perror("tcsetattr");
        return -1;
    }

    close(fd);
    return 1;
}
