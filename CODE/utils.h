#ifndef UTILS_H_
#define UTILS_H_

#define TRANSMITTER 0
#define RECEIVER 1

#define FLAG 0x7E
#define A_3 0x03 // 0x03 comands sent by writer and reply by receiver //0x01 if reply sent by writer and comands by reader
#define A_1 0x01
#define SET 0x03
#define UA 0x07

#define MAX_SIZE 1

//port definitions data
typedef struct
{
    int fileDescriptor;
    int status;
} applicationLayer;

//layer definitions
typedef struct
{
    char port[20];
    int baudRate;
    unsigned int sequenceNumber;
    unsigned int timeout;
    unsigned int numTransmissions; /*Número de tentativas em caso de falha*/
    char frame[MAX_SIZE];          /*Trama*/
}linkLayer;

//state machine states
typedef enum {
    start,
    flag_rcv,
    a_rcv,
    c_rcv,
    bcc_ok,
    stop
} state_t;

//state machine store
typedef struct
{
    state_t state;
} instance_data_t;

/**
 * @brief set reception state machine 
 * @param mahchine get machine current state 
 * @param pack byte to check 
 */
void set_reception(instance_data_t *machine, unsigned char pack);
/**
 * @brief ua reception state machine 
 * @param mahchine get machine current state 
 * @param pack byte to check 
 */
void ua_reception(instance_data_t *machine, unsigned char pack);

#endif // UTILS_H_
