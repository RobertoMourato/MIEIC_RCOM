/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>


#include "utils.h"
#include "error.h"

#define BAUDRATE B115200 	
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

volatile int STOP=FALSE;
unsigned char set[5]= {FLAG, A_3,SET,A_3^SET,FLAG};

instance_data_t machine = {start};

int main(int argc, char** argv)
{
    int fd,c, res;
    struct termios oldtio,newtio;
    char buf[255];
    char ua[255];

    int i, sum = 0, speed = 0;
    c=0;
    
    if ( (argc < 2) || 
  	     ((strcmp("/dev/ttyS0", argv[1])!=0) && 
  	      (strcmp("/dev/ttyS1", argv[1])!=0) )) {
      printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
      exit(1);
    }

	
  /*
    Open serial port device for reading and writing and not as controlling tty
    because we don't want to get killed if linenoise sends CTRL-C.
  */


    fd = open(argv[1], O_RDWR | O_NOCTTY );
    if (fd <0) {perror(argv[1]); exit(-1); }

    if ( tcgetattr(fd,&oldtio) == -1) { /* save current port settings */
      perror("tcgetattr");
      exit(-1);
    }

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    /* set input mode (non-canonical, no echo,...) */
    newtio.c_lflag = 0;

    newtio.c_cc[VTIME]    = 0;   /* inter-character timer unused */
    newtio.c_cc[VMIN]     = 1 ;   /* blocking read until 5 chars received */



  /* 
    VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a 
    leitura do(s) próximo(s) caracter(es)
  */

    tcflush(fd, TCIOFLUSH);

    if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }

    printf("New termios structure set\n");

    //SENDING SET 
    printf("Sending SET...\n");
    res = write(fd, set,5);
    if(res <0) exit(ERR_WR);
    
    printf("Receiving UA...\n");
    //RECEIVING UA 
    for( int i=0; i<5; i++){
        res = read(fd,&ua[i],1);    
        set_reception(&machine,ua[i]);   
        if(machine.state == stop)
            printf("Succsefully passed UA\n"); 
    }


    //PASS DATA
    /*testing*/
	
	printf("Type text to send: "); 
	
	gets(buf);
    buf[strlen(buf)] = '\0';

	printf("Passed: %s Size: %d\n", buf,strlen(buf)); 
	
	//get string char size
 	res = write(fd,buf,strlen(buf)+1);   

    //receive answer 

    for(int i=0;; i++){
        res = read(fd, &buf[i], 1);
        if(buf[i] == '\0') break;  
    }

	printf("Received:%s\n",buf);   

  /* 
    O ciclo FOR e as instruções seguintes devem ser alterados de modo a respeitar 
    o indicado no guião 
  */



   	sleep(1); 
    if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }




    close(fd);
    return 0;
}

