#ifndef DATALAYER_H_
#define DATALAYER_H_

/**
 * @brief open serial port
 * @param port get port number in /dev/ttySX where X is the port received
 * @param type if it opens as a trasnmitter or a reciver 
 * @return return fd or -1 in error case 
 */
int llopen(int port, int type);

/**
 * @brief write to serial port
 * @param fd file descriptor port to write in
 * @param buffer character array to transmit
 * @param length array lentgh
 * @return return number of characters written or -1 in error case 
 */
int llwrite(int fd, char *buffer, int length);

/**
 * @brief write from serial port
 * @param fd file descriptor port to read from
 * @param buffer char array received
 * @return array length received -1 in error case
 */
int llread(int fd, unsigned char *buffer);

/**
 * @brief close serial port
 * @param fd file descriptor 
 * @return positive value case success negative if error 
 */
int llclose(int fd);

#endif //DATALAYER_H_