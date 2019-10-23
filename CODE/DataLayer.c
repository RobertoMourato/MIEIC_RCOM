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
                printf("Sending SET...\n");
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
                    printf("Passei corretamente!\n");
                    break;
                }
            }
        }
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
        res = write(fd, ua, 5);
        if (res < 0)
            exit(ERR_WR);
        break;
    }

    //make structure
    return fd;
}
//*VERSAO ANTIGA - FUCNIONAL MAS BASICA
int llwrite(int fd, char *buffer, int length){
    
    int res; 
    //char buf[255] to write from!

    //TODO tuff bytes 
    //TODO add FH to buffer received
    
    //install alarm 
    (void)signal(SIGALRM, alarm_handler);

    while(attempt < layerPressets.numTransmissions){
        if(flag){
            res = write(fd,buffer,length);
            if( res < 0){
                printf("Error writting to port!\n");
                return -1;
            }
            alarm(layerPressets.timeout);
            //read the ACK or NACK choose what to do!
        }
    }
    //write to port 


    printf("ola");
    
    return 0; //if sucessful 
//*PROTIPO DO ANDY 
int llwrite(int fd, char *buffer, int length);
{
    unsigned char BCC_data;
    unsigned char * BCC_data_stuffed;
    unsigned char * data_frame;
    BCC_data = BCC_make(buffer, size);
    BCC_data_stuffed = BCC_stuffing(BCC);

    data_frame[0] = FLAG;
    data_frame[1] = A_3;
    
    if(num_frame == 0)
    {
        data_frame[2] = NS_0;
    }
    else
    {
        data_frame[2] = NS_1;
    }

    data_frame[3] = (A_3^data_frame[2]);

    int inside_temp = 0;
    unsigned char * temp;
    for(int i = 0; i < length; i++)
  {
    
    if(buffer[i] == FLAG )     //byte stuffing
    {
      buffer[i] = ESC;
      for(int num_save_temp = i+1; i < size; num_save_temp++ )
      {
        
        temp[inside_temp] = buffer[num_save_temp];
        inside_temp++;

      }

      buffer[i+1] = FLAG_NEXT;

      int new_pos = i+2;

      for(int num_move = 0; num_move < inside_temp)
      {
          buffer[new_ pos] = temp[num_move];
          new_pos++;
      }

      inside_temp = 0;

      
      
      i = i+2;
    }
    else if(buffer[i] == ESC)
    {
      buffer[i] = ESC;
      for(int num_save_temp = i+1; i < size; num_save_temp++ )
      {
        
        temp[inside_temp] = buffer[num_save_temp];
        inside_temp++;

      }

      buffer[i+1] = ESC_NEXT;

      int new_pos = i+2;

      for(int num_move = 0; num_move < inside_temp)
      {
          buffer[new_ pos] = temp[num_move];
          new_pos++;
      }
      
      inside_temp = 0;

      
      
      i = i+2;
    } 
    else
    {
        data_frame[i+4] = buffer[i];   
    }
    
  }
  if(strlen(BCC_stuffed) == 1)
  {
      data_frame[strlen(data_frame)+1] = BCC;
  }
  else if(strlen(BCC_stuffed)== 2)
  {
      data_frame[strlen(data_frame)+1] = BCC_stuffed[0];
      data_frame[strlen(data_frame)+1] = BCC_stuffed[1];
  }

  data_frame[strlen(data_frame)+1] = FLAG;


  write(fd, data_frame, sizeof(data_frame));
        
    
    
    
}

int llread(int fd, char *buffer);

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
