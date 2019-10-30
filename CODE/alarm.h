#ifndef ALARM_H_
#define ALARM_H_

#include "application.h"

unsigned int attempt = 0;
unsigned int flag = 1;

extern applicationLayer app;

void alarm_handler(/*int signo*/)
{
    printf("alarme # %d\n", attempt);
    flag = 1;
    attempt++;
    fcntl(app.fileDescriptor,F_SETFL,O_NONBLOCK); 

}

void turnoff_alarm(){
    alarm(0);
    flag = 1;
    attempt = 0;
    fcntl(app.fileDescriptor,F_SETFL,~O_NONBLOCK);
}

#endif //ALARM_H_
