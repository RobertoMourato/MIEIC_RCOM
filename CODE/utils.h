#ifndef UTILS_H_
#define UTILS_H_

#define TRANSMITTER 0
#define RECEIVER 1

#define FLAG 0x7E
#define A_3 0x03 // 0x03 comands sent by writer and reply by receiver 
#define A_1 0x01 //0x01 if reply sent by writer and comands by receiver
#define SET 0x03
#define UA 0x07
#define DISC 0x0B
#define RR 0x03 //check what R on slides means
#define REJ 0x01  //check what R on slides means 

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
} supervision_state_t;

//state machine store
typedef struct
{
    supervision_state_t state;
} supervision_instance_data_t;

/**
 * @brief set reception state machine 
 * @param machine get machine current state 
 * @param pack byte to check 
 */
void set_reception(supervision_instance_data_t *machine, unsigned char pack);
/**
 * @brief ua reception state machine 
 * @param machine get machine current state 
 * @param pack byte to check 
 */
void ua_reception(supervision_instance_data_t *machine, unsigned char pack);
/**
 * @brief disc reception state machine 
 * @param machine get machine current state 
 * @param pack byte to check 
 */
void disc_reception(supervision_instance_data_t *machine, unsigned char pack);

#endif // UTILS_H_
