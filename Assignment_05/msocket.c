#include <msocket.h>

// Function to create an int4 value
int4 init_int4(int value) {
    int4 result;
    result.value = value % 16; // Ensure value is within the range 0 to 15
    return result;
}

// Function to add two int4 values
int4 add_int4(int4 a, int4 b) {
    int sum = (a.value + b.value) % 16; // Perform addition and modulo operation
    return init_int4(sum);
}

// Function to subtract two int4 values
int4 sub_int4(int4 a, int4 b) {
    int diff = (a.value - b.value + 16) % 16; // Perform subtraction and ensure positive result
    return init_int4(diff);
}

mtpSocket *get_shared_MTP_Table() {
    key_t sm_key = ftok(".", MTP_TABLE);
    int sm_id = shmget(sm_key, (MAX_SOCKETS)*sizeof(mtpSocket), 0777|IPC_CREAT);
    if (sm_id == -1) {
        perror("shmget");
        return NULL;
    }
    mtpSocket *mtpTable = (mtpSocket *)shmat(sm_id, 0, 0);
    return mtpTable;
}

SOCK_INFO *get_SOCK_INFO() {
    key_t sm_key = ftok(".", SHARED_RESOURCE);
    int sm_id_sock_info = shmget(sm_key, sizeof(int), 0777|IPC_CREAT);
    if (sm_id_sock_info == -1) {
        perror("shmget");
        return NULL;
    }
    SOCK_INFO *vars = (SOCK_INFO *)shmat(sm_id_sock_info, 0, 0);
    return vars;
}

void get_sem1(int *id) {
    key_t sm_key = ftok(".", SEM1);
    *id = semget(sm_key, 1, 0777|IPC_CREAT);
}

void get_sem2(int *id) {
    key_t sm_key = ftok(".", SEM2);
    *id = semget(sm_key, 1, 0777|IPC_CREAT);
}

void get_mutex(int *id) {
    key_t sm_key = ftok(".", MUTEX);
    *id = semget(sm_key, 1, 0777|IPC_CREAT);
}

void get_mutex_swnd(int *id) {
    key_t sm_key = ftok(".", MUTEX_SWND);
    *id = semget(sm_key, 1, 0777 | IPC_CREAT);
}

void get_mutex_sendbuf(int *id) {
    key_t sm_key = ftok(".", MUTEX_SENDBUF);
    *id = semget(sm_key, 1, 0777 | IPC_CREAT);
}

void get_mutex_recvbuf(int *id) {
    key_t sm_key = ftok(".", MUTEX_RECVBUF);
    *id = semget(sm_key, 1, 0777 | IPC_CREAT);
}

int m_socket(int domain, int type, int protocol) {

    if(type != SOCK_MTP) {
        return ERROR;
    }
    
    pop.sem_num = vop.sem_num = 0;
    pop.sem_flg = vop.sem_flg = 0;
    pop.sem_op = -1; 
    vop.sem_op = 1;

    mtpSocket *MTP_Table = get_shared_MTP_Table();
    SOCK_INFO *sock_info = get_SOCK_INFO();

    int sem1;
    get_sem1(&sem1);

    int sem2;
    get_sem2(&sem2);

    int mutex;
    get_mutex(&mutex);

    for (int i = 0; i < MAX_SOCKETS; i++) {
        if(MTP_Table[i].available == 1) {
            down(mutex);
            int mtp_id = i;
            MTP_Table[i].available = 0;
            MTP_Table[i].pid = getpid();
            
            sock_info->status = 0;
            sock_info->mtp_id = mtp_id;

            up(sem1);
            down(sem2);

            if(sock_info->error_no != 0) {
                errno = sock_info->error_no;
                MTP_Table[i].available = 1;
                up(mutex);

                shmdt(MTP_Table);
                shmdt(sock_info);

                return ERROR;
            }
            
            shmdt(MTP_Table);
            shmdt(sock_info);

            up(mutex);
            return mtp_id;
        }
    }

    errno = ENOBUFS; 
    up(mutex);   
 
    shmdt(MTP_Table);
    shmdt(sock_info);

    return ERROR;    
}

int m_close(int socket_id) {
    
    pop.sem_num = vop.sem_num = 0;
    pop.sem_flg = vop.sem_flg = 0;
    pop.sem_op = -1; 
    vop.sem_op = 1;

    mtpSocket *MTP_Table = get_shared_MTP_Table();
    SOCK_INFO *sock_info = get_SOCK_INFO();

    int sem1;
    get_sem1(&sem1);

    int sem2;
    get_sem2(&sem2);

    int mutex;
    get_mutex(&mutex);

    int mutex_swnd;
    get_mutex_swnd(&mutex_swnd);

    int mutex_sendbuf;
    get_mutex_sendbuf(&mutex_sendbuf);

    int mutex_recvbuf;
    get_mutex_recvbuf(&mutex_recvbuf);

    down(mutex);
    
    if(MTP_Table[socket_id].available == 0) {
        MTP_Table[socket_id].available = 1;
        MTP_Table[socket_id].udp_sockid = -1;

        down(mutex_swnd);
        down(mutex_sendbuf);

        for (int k = 0; k < SEND_BUFFER_SIZE; k++) {
            MTP_Table[socket_id].send_buffer[k].header.seq_number = -1;
            memset(MTP_Table[socket_id].send_buffer[k].data, 0, MESSAGE_SIZE);
        }

        up(mutex_sendbuf);

        MTP_Table[socket_id].swnd.left_idx = 0;
        MTP_Table[socket_id].swnd.right_idx = (RECV_BUFFER_SIZE - 1) % SEND_BUFFER_SIZE;
        MTP_Table[socket_id].swnd.fresh_msg = 0;
        MTP_Table[socket_id].swnd.last_seq_no = 0;
        MTP_Table[socket_id].swnd.last_sent = -1;
        MTP_Table[socket_id].swnd.last_ack_seqno = 0;

        for (int k = 0; k < SENDER_WINDOW_SIZE; k++) {
            MTP_Table[socket_id].swnd.last_sent_time[k] = 0;
        }

        up(mutex_swnd);

        down(mutex_recvbuf);
        
        MTP_Table[socket_id].rwnd.full = 0;
        MTP_Table[socket_id].rwnd.last_inorder_received = 0;
        MTP_Table[socket_id].rwnd.last_consumed = 0;

        for (int k = 0; k < RECV_BUFFER_SIZE; k++) {
            MTP_Table[socket_id].receive_buffer[k].header.seq_number = -1;
            memset(MTP_Table[socket_id].receive_buffer[k].data, 0, MESSAGE_SIZE);
        }

        for (int k = 0; k < RECEIVER_WINDOW_SIZE; k++) {
            MTP_Table[socket_id].rwnd.window[k] = k+1;
        }

        up(mutex_recvbuf);

        sock_info->mtp_id = socket_id;
        
        sock_info->status = 2;

        up(sem1);
        down(sem2);

        if(sock_info->error_no != 0) {
            errno = sock_info->error_no;
            MTP_Table[socket_id].available = 0;
            up(mutex);

            shmdt(MTP_Table);
            shmdt(sock_info);

            return ERROR;
        }

        int retval = sock_info->return_value;

        shmdt(MTP_Table);
        shmdt(sock_info);

        up(mutex);
        return retval;
    }

    up(mutex);
            
    shmdt(MTP_Table);
    shmdt(sock_info);

    return ERROR;
}

int m_bind(int socket_id, char *src_ip, unsigned short int src_port, char *dest_ip, unsigned short int dest_port){
    
    pop.sem_num = vop.sem_num = 0;
    pop.sem_flg = vop.sem_flg = 0;
    pop.sem_op = -1; 
    vop.sem_op = 1;

    mtpSocket *MTP_Table = get_shared_MTP_Table();
    SOCK_INFO *sock_info = get_SOCK_INFO();

    int sem1;
    get_sem1(&sem1);

    int sem2;
    get_sem2(&sem2);

    int mutex;
    get_mutex(&mutex);

    down(mutex);

    if(MTP_Table[socket_id].available == 0) {
        sock_info->status = 1;
        sock_info->mtp_id = socket_id;

        strcpy(MTP_Table[socket_id].dest_ip, dest_ip);
        MTP_Table[socket_id].dest_port = dest_port;

        sock_info->src_addr.sin_addr.s_addr = inet_addr(src_ip);
        sock_info->src_addr.sin_port = htons(src_port);
        sock_info->src_addr.sin_family = AF_INET;

        up(sem1);
        down(sem2);

        if(sock_info->error_no!=0) {
            errno = sock_info->error_no;
            MTP_Table[socket_id].dest_ip[0] = '\0';
            MTP_Table[socket_id].dest_port = 0;
            up(mutex);

            shmdt(MTP_Table);
            shmdt(sock_info);

            return ERROR;
        }

        int retval = sock_info->return_value;

        shmdt(MTP_Table);
        shmdt(sock_info);

        up(mutex);
        return retval;
    }

    up(mutex);
            
    shmdt(MTP_Table);
    shmdt(sock_info);
    
    return ERROR;
}

int m_sendto(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen) {

    pop.sem_num = vop.sem_num = 0;
    pop.sem_flg = vop.sem_flg = 0;
    pop.sem_op = -1; 
    vop.sem_op = 1;

    mtpSocket* MTP_Table = get_shared_MTP_Table();
    
    int mutex_swnd;
    get_mutex_swnd(&mutex_swnd);

    int mutex_sendbuf;
    get_mutex_sendbuf(&mutex_sendbuf);

    down(mutex_swnd);
    down(mutex_sendbuf);

    if(MTP_Table[sockfd].send_buffer[MTP_Table[sockfd].swnd.fresh_msg].header.seq_number != -1) {
        errno = ENOBUFS;

        up(mutex_sendbuf);
        up(mutex_swnd);

        shmdt(MTP_Table);

        return ERROR;
    }

    if (!(!MTP_Table[sockfd].available 
        && MTP_Table[sockfd].dest_port == ntohs(((struct sockaddr_in *)dest_addr)->sin_port) 
        && strcmp(MTP_Table[sockfd].dest_ip, inet_ntoa(((struct sockaddr_in *)dest_addr)->sin_addr))) == 0){

        shmdt(MTP_Table);
        errno = ENOTCONN;
        return ERROR;
    }

    int idx = MTP_Table[sockfd].swnd.fresh_msg;
    strncpy(MTP_Table[sockfd].send_buffer[idx].data, buf, MESSAGE_SIZE);

    MTP_Table[sockfd].send_buffer[idx].header.seq_number = (MTP_Table[sockfd].swnd.last_seq_no)%MAX_SEQ_NUMBER + 1;
    MTP_Table[sockfd].swnd.last_seq_no = MTP_Table[sockfd].send_buffer[idx].header.seq_number;
    MTP_Table[sockfd].swnd.fresh_msg = (MTP_Table[sockfd].swnd.fresh_msg+1)%SEND_BUFFER_SIZE;

    shmdt(MTP_Table);

    up(mutex_sendbuf);
    up(mutex_swnd);

    return 0;
}

int m_recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen) {

    pop.sem_num = vop.sem_num = 0;
    pop.sem_flg = vop.sem_flg = 0;
    pop.sem_op = -1; 
    vop.sem_op = 1;

    mtpSocket* MTP_Table = get_shared_MTP_Table();

    int mutex_recvbuf;
    get_mutex_recvbuf(&mutex_recvbuf);

    down(mutex_recvbuf);

    int min_seqno = (MTP_Table[sockfd].rwnd.last_consumed)%MAX_SEQ_NUMBER+1;

    for(int i=0; i<RECV_BUFFER_SIZE; i++) {
        if(MTP_Table[sockfd].receive_buffer[i].header.seq_number == min_seqno) {
            
            strncpy(buf, MTP_Table[sockfd].receive_buffer[i].data, MESSAGE_SIZE);
            MTP_Table[sockfd].receive_buffer[i].header.seq_number = -1;
            
            MTP_Table[sockfd].rwnd.last_consumed = min_seqno;

            shmdt(MTP_Table);
            up(mutex_recvbuf);
            return MESSAGE_SIZE;
        }
    }

    shmdt(MTP_Table);
    up(mutex_recvbuf);
    errno = ENOMSG;
    return ERROR;
}