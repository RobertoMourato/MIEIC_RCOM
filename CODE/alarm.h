#ifndef ALARM_H_
#define AALARM_H_

int attempt = 1;
int flag = 1;

void alarm_handler(int signo)
{
    printf("alarme # %d\n", attempt);
    flag = 1;
    attempt++;
}

#endif //ALARM_H_