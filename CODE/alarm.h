#ifndef ALARM_H_
#define ALARM_H_

unsigned int attempt = 0;
unsigned int flag = 1;

void alarm_handler(/*int signo*/)
{
    printf("alarme # %d\n", attempt);
    flag = 1;
    attempt++;
}

void turnoff_alarm(){
    alarm(0);
    flag = 1;
    attempt = 0;
}

#endif //ALARM_H_