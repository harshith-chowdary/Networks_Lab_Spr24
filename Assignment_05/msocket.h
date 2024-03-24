#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <signal.h>

// Define constants
#define SOCK_MTP 69 // MTP socket type
#define T 5 // Timeout period
#define MAX_SOCKETS 25
#define MAX_MESSAGES 100

#define DROP_PROBABILITY 0.33 // Probability of dropping a packet

#define SEND_BUFFER_SIZE 10
#define RECV_BUFFER_SIZE 5
#define SENDER_WINDOW_SIZE 5
#define RECEIVER_WINDOW_SIZE 5

#define MAX_SEQ_NUMBER 16 // Maximum sequence number - 4 bits

#define MESSAGE_SIZE 1022 // 1 KB message size
#define IP_SIZE 16 // 15 characters for IP address and 1 for null terminator

#define ERROR -1 // Error return value
#define SUCCESS 0 // Success return value

#define MTP_TABLE 101
#define SHARED_RESOURCE 102
#define SEM1 103
#define SEM2 104
#define MUTEX 105
#define MUTEX_SWND 106
#define MUTEX_SENDBUF 107
#define MUTEX_RECVBUF 108

// sembuf
struct sembuf pop, vop;
#define down(s) semop(s, &pop, 1)   // wait(s)
#define up(s) semop(s, &vop, 1)     // signal(s)

// Define custom data type - int4 (unsigned 4-bit integer)
typedef struct {
    int value; // Internal integer value
} int4;

int4 init_int4(int value); // Function to create an int4 value
int4 add_int4(int4 a, int4 b); // Function to add two int4 values
int4 sub_int4(int4 a, int4 b); // Function to subtract two int4 values

typedef struct {
    int seq_number; // Sequence number of the message
} header;

// Structure for the message
typedef struct {
    header header; // Message header
    char data[MESSAGE_SIZE]; // Message data
} message;

// Structure for the sending window
typedef struct {
    int left_idx; // Left index of the window
    int right_idx; // Right index of the window

    int fresh_msg; // Index of the new entry
    int last_seq_no; // Last sequence number sent

    int last_ack_seqno; // Last acknowledged sequence number
    int last_sent; // Last sent sequence number
    time_t last_sent_time[SEND_BUFFER_SIZE]; // Last active time of the message

    int last_ack_rwnd_size; // Last acknowledged receiver window size
} send_window;

// Structure for the receiving window
typedef struct {
    bool full; // 1 if the buffer is full, 0 otherwise

    int last_inorder_received; // Last in-order received sequence number
    int last_consumed; // Last user-taken sequence number

    int window[5]; // Sequence numbers of the messages in the window
} receive_window;

// Define data structures
typedef struct {
    bool available; // 0 if the socket is available, 1 if it is in use
    pid_t pid; // Process ID for the process that created the MTP socket
    int udp_sockid; // Mapping from MTP socket ID to the corresponding UDP socket ID

    char dest_ip[IP_SIZE]; // IP address of the destination
    unsigned short int dest_port; // Port number of the destination

    message send_buffer[SEND_BUFFER_SIZE]; // Sender-side message buffer
    message receive_buffer[RECV_BUFFER_SIZE]; // Receiver-side message buffer

    send_window swnd; // Sending window
    receive_window rwnd; // Receiving window
    // Add any other fields as necessary
} mtpSocket;

typedef struct {
    int status;
    int mtp_id; // MTP socket ID
    struct sockaddr_in src_addr; // Source address

    int return_value; // Return value of the last function call
    int error_no; // Error number of the last function call
} SOCK_INFO;

// Function prototypes
int m_socket(int domain, int type, int protocol);
int m_bind(int socket_id, char *src_ip, unsigned short int src_port, char *dest_ip, unsigned short int dest_port);
int m_sendto(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen);
int m_recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen);
int m_close(int sockfd);

void *thread_R_func(void *arg);
void *thread_S_func(void *arg);
void *thread_G_func(void *arg);
int dropMessage(float p);

// Function to get the shared MTP table
mtpSocket *get_shared_MTP_Table(); // Function to get the shared MTP table
SOCK_INFO *get_SOCK_INFO(); // Function to get the shared SOCK_INFO
void get_sem1(int *sem1);   // Function to get the shared semaphore 1
void get_sem2(int *sem2);   // Function to get the shared semaphore 2
void get_mutex(int *mutex); // Function to get the shared mutex
void get_mutex_swnd(int *mutex_swnd);   // Function to get the shared mutex for the sending window
void get_mutex_sendbuf(int *mutex_sendbuf);  // Function to get the shared mutex for the sending buffer
void get_mutex_recvbuf(int *mutex_recvbuf);  // Function to get the shared mutex for the receiving buffer