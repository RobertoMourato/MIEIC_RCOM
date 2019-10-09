#ifndef UTILS_H_
#define UTILS_H_

#define FLAG 0x7E
#define A_3 0x03 // 0x03 comands sent by writer and reply by receiver //0x01 if reply sent by writer and comands by reader
#define A_1 0x01
#define SET 0x03
#define UA 0x07
//TODO make define to be used as array index

typedef enum
{
    start,
    flag_rcv,
    a_rcv,
    c_rcv,
    bcc_ok,
    stop
} state_t;

typedef struct
{
    state_t state;
} instance_data_t;

int set_reception(instance_data_t *machine, unsigned char pack)
{
    switch (machine->state)
    {
    case (start):
        if (pack == FLAG)
            machine->state = flag_rcv;
        break;
    case (flag_rcv):
        if (pack == A_1)
        {
            machine->state = a_rcv;
            break;
        }
        if (pack != FLAG)
            machine->state = start;
        break;
    case (a_rcv):
        if (pack == 1)
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
        if (pack == A_3 ^ SET)
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
        if (pack == flag_rcv)
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

#endif // UTILS_H_