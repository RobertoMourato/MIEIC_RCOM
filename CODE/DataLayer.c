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

linkLayer linkLayerSettings;

//*GLOBALS
unsigned char set[5] = {FLAG, A_3, SET, A_3 ^ SET, FLAG};
unsigned char ua[5] = {FLAG, A_3, UA, A_3 ^ UA, FLAG};
struct termios oldtio;
//set Layer values 
instance_data_t machineOpen = {start};

//TRANSMITTER | RECEIVER
int llopen(int port, int type)
{
    int fd,res;
    struct termios newtio;
    char buf[255];
    char portStr[20] = "/dev/ttyS"; //to append to the port number
    char portNr=  port +'0';
    puts(&portNr);
    strcat(portStr,&portNr);

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
        (void)signal(SIGALRM, alarm_handler);

        while (attempt < 4)
        {
            if (flag)
            {
                //SENDING SET
                printf("Sending SET...\n");
                res = write(fd, set, 5);
                if (res < 0)
                    exit(ERR_WR);

                alarm(3);
                flag = 0;

                printf("Receiving UA...\n");
                for (int i = 0; i < 5; i++)
                {
                    res = read(fd, &buf[i], 1);
                    ua_reception(&machineOpen, buf[i]);
                    if (machineOpen.state == stop)
                        printf("Succsefully passed UA\n");
                }
                if (machineOpen.state == stop)
                {
                    printf("Passei corretamente!\n");
                    break;
                }
            }
        }
        break;
    case RECEIVER:
        //RECEIVE  SET - AND CHECK
        printf("Receiving SET...\n");
        while (machineOpen.state != stop)
        {
            for (int i = 0; i < 5; i++)
            {
                res = read(fd, &buf[i], 1);
                if (res < 0)
                    exit(ERR_RD);
                set_reception(&machineOpen, buf[i]);
                if (machineOpen.state == stop)
                    printf("Succsefully passed SET\n");
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

int llwrite(int fd, char *buffer, int length);

int llread(int fd, char *buffer);

int llclose(int fd)
{
    //SEND RCV DISC SEND UA ACK
    sleep(1);
    if (tcsetattr(fd, TCSANOW, &oldtio) == -1)
    {
        perror("tcsetattr");
        exit(-1);
    }

    close(fd);
    return 0;
}
