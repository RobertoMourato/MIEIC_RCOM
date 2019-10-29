/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>

#include "utils.h"
#include "error.h"

#define BAUDRATE B115200
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

volatile int STOP = FALSE;
unsigned char ua[5] = {FLAG, A_3, UA, A_3 ^ UA, FLAG};

instance_data_t machine = {start};

int main(int argc, char **argv)
{
  int fd, c, res;
  struct termios oldtio, newtio;
  char buf[255];
  char set[255]; //SET

  if ((argc < 2) ||
      ((strcmp("/dev/ttyS0", argv[1]) != 0) &&
       (strcmp("/dev/ttyS1", argv[1]) != 0)))
  {
    printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
    exit(1);
  }

  /*
    Open serial port device for reading and writing and not as controlling tty
    because we don't want to get killed if linenoise sends CTRL-C.
  */

  fd = open(argv[1], O_RDWR | O_NOCTTY);
  if (fd < 0)
  {
    perror(argv[1]);
    exit(-1);
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

  newtio.c_cc[VTIME] = 0; /* inter-character timer unused */
  newtio.c_cc[VMIN] = 1;  /* blocking read until 5 chars received */

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

  //RECEIVE  SET - AND CHECK
  printf("Receiving SET...\n");
  while (machine.state != stop)
  {

    for (int i = 0; i < 5; i++)
    {
      //printf("%d\n",i);
      res = read(fd, &set[i], 1);
      if (res < 0)
        exit(ERR_RD);
      set_reception(&machine, set[i]);
      if (machine.state == stop)
        printf("Succsefully passed SET\n");
    }
  }

  //SEND UA
  printf("Sending UA...\n");
  res = write(fd, ua, 5);
  if (res < 0)
    exit(ERR_WR);

  printf("Receiving DATA...\n");
  //PASS DATA
  for (int i = 0;; i++)
  {
    res = read(fd, &buf[i], 1);
    if (buf[i] == '\0')
      break;
  }

  printf("message received - %s\n", buf);

  res = write(fd, buf, strlen(buf) + 1);

  printf("Ressending - %s\n", buf);

  /* 
    O ciclo WHILE deve ser alterado de modo a respeitar o indicado no gui�o 
  */

  sleep(1);
  tcsetattr(fd, TCSANOW, &oldtio);
  close(fd);
  return 0;
}
