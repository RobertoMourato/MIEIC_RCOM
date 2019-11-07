#ifndef UTILS_H_
#define UTILS_H_

#define TRANSMITTER 0
#define RECEIVER 1
#define START 0
#define END 1

//BIT FLAGS
#define FLAG 0x7E
#define ESC 0x7D
#define FLAG_NEXT 0x5E
#define ESC_NEXT 0x5D
#define NS_0 0x00
#define NS_1 0x40
#define A_3 0x03 // 0x03 comands sent by writer and reply by receiver 
#define A_1 0x01 //0x01 if reply sent by writer and comands by receiver
#define SET 0x03
#define UA 0x07
#define DISC 0x0B
#define RR0 0x05 
#define REJ0 0x01  
#define RR1 0x85  
#define REJ1 0x81 

#define C1 0x01
#define C2 0x02
#define C3 0x03
#define T1 0x00
#define T2 0x01

//PORT SETTINGS
#define BAUDRATE B115200
#define MODEMDEVICE "/dev/ttyS1"
#define SEQNUM 0
#define TIMEOUT 3
#define ATEMPTS 3
#define MAX_SIZE 1024

//ALARM 
#define VTIMEVAL 0   /* inter-character timer unused */
#define VMINVAL  1   /* blocking read until 5 chars received */

#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

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
 * @brief set linklayer data 
 * @param linkLayer linklayer to make
 */
void setLinkLayer(linkLayer *linklayer,char port[]);

/**
 * @brief make control packet 
 * @param type  0 - start 1 - end 
 * @param path filename path 
 * @param size size of the file 
 * @param controlPackLen length of the control pack made
 * @return char * with control packet
 */
unsigned char * makeControlPacket(int type, char path[],off_t size,int *controlPackLen);

/**
 * @brief make control packet 
 * @param fd fileDescriptor
 * @param linkLayer linkpressets so that i can acsses message seq number count
 * @param buffer file pack to write
 * @param length size read
 * @return postive value if success
 */
int sendDataPacket(int fd, linkLayer *linkLayer,const char* buffer, int length);

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

/**
 * @brief generate BCC packet 
 * @param buffer //todo
 * @param size //todo
 * @return //todo
 */
unsigned char BCC_make(unsigned char * buffer, int size);

/**
 * @brief stuffing bcc
 * @param BCC //todo
 * @return //todo
 */
unsigned char * BCC_stuffing(unsigned char BCC);

/**
 * @brief stuffing bcc
 * @param fd fileDescriptor
 * @return //todo
 */
unsigned char read_control_field(int fd);

//roberto
void setThingsFromStart(off_t *sizeOfAllMessages, unsigned char * fileName, unsigned char *startTransmition);
int endReached(unsigned char * message, int sizeOfMessage, unsigned char * startTransmition, int sizeOfStartTransmition);
unsigned char *headerRemoval(unsigned char *message, int sizeOfMessage);
int checkBcc2(unsigned char *buffer, int sizeBuffer);
void sendControlMessage(int fd, unsigned char controlField);

/**
 * @brief debug function to print bytes in hexadecimal 
 * @param title title of the bytes to print 
 * @param buf buffer with the bytes to print 
 * @param buf_len length of the buffer 
 */
void print_buf(const char *title, unsigned char *buf, size_t buf_len);

#endif // UTILS_H_
