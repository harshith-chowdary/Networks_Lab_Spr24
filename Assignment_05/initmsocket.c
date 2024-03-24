#include <msocket.h>

void *thread_R_func(void *arg);
void *thread_S_func(void *arg);
void *thread_G_func(void *arg);
int dropMessage(float p);

pthread_t thread_R, thread_S, thread_G;

mtpSocket * MTP_Table;

void printTable() {
    printf("Printing MTP Table\n");
    for (int i = 0; i < MAX_SOCKETS; i++) {
        if (!MTP_Table[i].available) {
            printf("PID => %d\n", MTP_Table[i].pid);
            printf("Socket ID => %d\n", i);
            printf("UDP Socket ID => %d\n", MTP_Table[i].udp_sockid);
            printf("Destination IP => %s\n", MTP_Table[i].dest_ip);
            printf("Destination Port => %d\n", MTP_Table[i].dest_port);
            // printf("Send Buffer =>\n");
            // for (int j = 0; j < SEND_BUFFER_SIZE; j++) {
            //     if (MTP_Table[i].send_buffer[j].header.seq_number != -1) {
            //         printf("Seq Number => %d\n", MTP_Table[i].send_buffer[j].header.seq_number);
            //         printf("Data => %s\n", MTP_Table[i].send_buffer[j].data);
            //     }
            // }
            // printf("Receive Buffer =>\n");
            // for (int j = 0; j < RECV_BUFFER_SIZE; j++) {
            //     if (MTP_Table[i].receive_buffer[j].header.seq_number != -1) {
            //         printf("Seq Number => %d\n", MTP_Table[i].receive_buffer[j].header.seq_number);
            //         printf("Data => %s\n", MTP_Table[i].receive_buffer[j].data);
            //     }
            // }
        }
    }
    printf("\n");
}

// To find the max seq number between x and y (mod 16 + 1)
int max_window(int x, int y) {
    if (abs(x-y) < MAX_SEQ_NUMBER/2) {
        return (x >= y) ? x : y;
    } else {
        return (x >= y) ? y : x;
    }
}

// Semaphores
int sem1, sem2, mutex, mutex_swnd, mutex_sendbuf, mutex_recvbuf;

int total_sendto_calls = 0;

void sigint_handler(int signum) {
    if (signum == SIGINT) {
        shmctl(MTP_TABLE, IPC_RMID, 0);
        shmctl(SHARED_RESOURCE, IPC_RMID, 0);

        semctl(SEM1, 0, IPC_RMID, 0);
        semctl(SEM2, 0, IPC_RMID, 0);
        semctl(MUTEX, 0, IPC_RMID,  0);
        semctl(MUTEX_SWND, 0, IPC_RMID, 0);
        semctl(MUTEX_SENDBUF, 0, IPC_RMID, 0);
        semctl(MUTEX_RECVBUF, 0, IPC_RMID, 0);

        exit(EXIT_SUCCESS);
    }

    exit(EXIT_FAILURE);
}

void logs() {

    for(int socket_id = 0; socket_id < MAX_SOCKETS; socket_id++) {
        if (!MTP_Table[socket_id].available) {
            printf("\n=====================================================================\n");

            printf("PID => %d\n", MTP_Table[socket_id].pid);
            printf("Socket ID=> %d\n", socket_id);
            printf("UDP Socket ID=> %d\n", MTP_Table[socket_id].udp_sockid);

            printf("SEND BUFFER SEQ NUMS => ");
            for (int i = 0; i < SEND_BUFFER_SIZE; i++) {
                printf("%d ", MTP_Table[socket_id].send_buffer[i].header.seq_number);
            }
            printf("\n");

            printf("RECV WINDOW => ");
            for (int i = 0; i < RECEIVER_WINDOW_SIZE; i++) {
                printf("%d ", MTP_Table[socket_id].rwnd.window[i]);
            }
            printf("\n");

            printf("RECV BUFFER => ");
            for (int i = 0; i < RECV_BUFFER_SIZE; i++) {
                printf("%d ", MTP_Table[socket_id].receive_buffer[i].header.seq_number);
            }

            printf("\n");

            printf("\n=====================================================================\n");
        }
    }
}

// float DROP_PROBABILITY;

int main(int argc, char *argv[]) {

    // if(argc < 2) {
    //     printf("Usage => %s <drop_probability>\n", argv[0]);
    //     exit(EXIT_FAILURE);
    // }

    // DROP_PROBABILITY = atof(argv[1]);

    signal(SIGINT, sigint_handler);

    srand(time(NULL));

    MTP_Table = get_shared_MTP_Table();

    for(int i = 0; i < MAX_SOCKETS; i++) {
        MTP_Table[i].available = 1;
        MTP_Table[i].udp_sockid = -1;

        // for (int j = 0; j < SEND_BUFFER_SIZE; j++) {
        //     MTP_Table[i].send_buffer[j].header.seq_number = -1;
        //     memset(MTP_Table[i].send_buffer[j].data, 0, MESSAGE_SIZE);
        // }

        // for (int j = 0; j < RECV_BUFFER_SIZE; j++) {
        //     MTP_Table[i].receive_buffer[j].header.seq_number = -1;
        //     memset(MTP_Table[i].receive_buffer[j].data, 0, MESSAGE_SIZE);
        // }
    }

    SOCK_INFO *sock_info = get_SOCK_INFO();

    sock_info->status = -1;
    sock_info->mtp_id = -1;
    sock_info->return_value = 0;
    memset(&(sock_info->src_addr), 0, sizeof(sock_info->src_addr));

    // int sem1;
    get_sem1(&sem1);
    semctl(sem1, 0, SETVAL, 0);

    // int sem2;
    get_sem2(&sem2);
    semctl(sem2, 0, SETVAL, 0);

    // int mutex;
    get_mutex(&mutex);
    semctl(mutex, 0, SETVAL, 1);

    // int mutex_swnd;
    get_mutex_swnd(&mutex_swnd);
    semctl(mutex_swnd, 0, SETVAL, 1);

    // int mutex_sendbuf;
    get_mutex_sendbuf(&mutex_sendbuf);
    semctl(mutex_sendbuf, 0, SETVAL, 1);

    // int mutex_recvbuf;
    get_mutex_recvbuf(&mutex_recvbuf);
    semctl(mutex_recvbuf, 0, SETVAL, 1);

    pop.sem_num = vop.sem_num = 0;
    pop.sem_flg = vop.sem_flg = 0;
    pop.sem_op = -1; 
    vop.sem_op = 1;

    // Initialize threads
    if (pthread_create(&thread_R, NULL, thread_R_func, NULL) != 0) {
        perror("pthread_create for thread_R");
        exit(EXIT_FAILURE);
    }
    if (pthread_create(&thread_S, NULL, thread_S_func, NULL) != 0) {
        perror("pthread_create for thread_S");
        exit(EXIT_FAILURE);
    }
    if (pthread_create(&thread_G, NULL, thread_G_func, NULL) != 0) {
        perror("pthread_create for thread_G");
        exit(EXIT_FAILURE);
    }

    int socket_id;
    while (1) {
        sock_info->error_no = 0;
        down(sem1);

        if(sock_info->status == 0) {
            printf("SOCKET Call\n");
            socket_id = socket(AF_INET, SOCK_DGRAM, 0);
            MTP_Table[sock_info->mtp_id].udp_sockid = socket_id;

            sock_info->return_value = socket_id;
            sock_info->error_no = errno;
        }

        else if(sock_info->status == 1) {
            printf("BIND Call\n");
            socket_id = MTP_Table[sock_info->mtp_id].udp_sockid;

            if(setsockopt(socket_id, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0) {
                perror("setsockopt(SO_REUSEADDR) failed");
            }

            sock_info->return_value = bind(socket_id, (const struct sockaddr *)&(sock_info->src_addr), sizeof(sock_info->src_addr));
            sock_info->error_no = errno;
        }

        else if(sock_info->status == 2) {
            printf("CLOSE Call\n");
            socket_id = MTP_Table[sock_info->mtp_id].udp_sockid;

            // sock_info->return_value = close(socket_id);
            sock_info->return_value = 0; // Dummy return value without actually closing the socket
            sock_info->error_no = errno;
        }
        sock_info->status = -1;

        // printTable();
        up(sem2);
    }

    // Join thread_S
    if (pthread_join(thread_S, NULL) != 0) {
        perror("Failed to join S thread");
        exit(EXIT_FAILURE);
    }

    // Join thread_R
    if (pthread_join(thread_R, NULL) != 0) {
        perror("Failed to join R thread");
        exit(EXIT_FAILURE);
    }

    // Join thread_G
    if (pthread_join(thread_G, NULL) != 0) {
        perror("Failed to join G thread");
        exit(EXIT_FAILURE);
    }

    shmdt(MTP_Table);
    shmdt(sock_info);
}

void *thread_R_func(void *arg) {
    mtpSocket * MTP_Table = get_shared_MTP_Table();

    down(mutex_recvbuf);

    for (int i = 0; i < MAX_SOCKETS; i++) {
        
        MTP_Table[i].rwnd.full = 0;
        MTP_Table[i].rwnd.last_inorder_received = 0;
        MTP_Table[i].rwnd.last_consumed = 0;

        for (int k = 0; k < RECEIVER_WINDOW_SIZE; k++) {
            MTP_Table[i].rwnd.window[k] = k+1;
        }

        for (int k = 0; k < RECV_BUFFER_SIZE; k++) {
            MTP_Table[i].receive_buffer[k].header.seq_number = -1;
            memset(MTP_Table[i].receive_buffer[k].data, 0, MESSAGE_SIZE);
        }
    }

    up(mutex_recvbuf);

    fd_set readfds;
    struct timeval timeout;
    int max_fd;

    while (1) {
        timeout.tv_sec = 2;
        timeout.tv_usec = 0;

        FD_ZERO(&readfds);
        max_fd = -1;

        for (int i = 0; i < MAX_SOCKETS; i++) {
            if (!MTP_Table[i].available) {
                FD_SET(MTP_Table[i].udp_sockid, &readfds);
                if (MTP_Table[i].udp_sockid > max_fd) {
                    max_fd = MTP_Table[i].udp_sockid;
                }
            }
        }

        int ret = select(max_fd + 1, &readfds, NULL, NULL, &timeout);
        if(ret < 0){
            perror("select");
            sleep(2);
            continue;
        }
        
        // logs
        // logs();

        for (int i = 0; i < MAX_SOCKETS; i++) {

            if(!MTP_Table[i].available) {

                if (FD_ISSET(MTP_Table[i].udp_sockid, &readfds)){
                    char buffer[MESSAGE_SIZE + 2];

                    struct sockaddr_in dest_address;
                    dest_address.sin_family = AF_INET;
                    dest_address.sin_port = htons(MTP_Table[i].dest_port);
                    dest_address.sin_addr.s_addr = inet_addr(MTP_Table[i].dest_ip);

                    int addr_len = sizeof(dest_address);

                    int len = recvfrom(MTP_Table[i].udp_sockid, buffer, MESSAGE_SIZE + 2, 0, (struct sockaddr *) &dest_address, &addr_len);
                    printf("SOCKET %d => RECV => %s\n", i, buffer);

                    if(len < 0){
                        perror("recvfrom");
                        continue;
                    }

                    if(dropMessage(DROP_PROBABILITY)) {
                        printf("DROPPED Message => %s\n", buffer);
                        continue;
                    }

                    buffer[len] = '\0';

                    // Received message is ACK
                    if (buffer[0] == 'A') {
                        down(mutex_swnd);

                        int curr_ack_seqno = (int)(buffer[1]-'a');
                        int curr_rwnd_size = (int)(buffer[2]-'a');

                        send_window curr_swnd = MTP_Table[i].swnd;
                        int last_ack_seqno = curr_swnd.last_ack_seqno;

                        printf("SOCKET %d => curr_ack_seqno => %d, last_ack_seqno => %d\n", i,  curr_ack_seqno, last_ack_seqno);

                        bool dup_ack = 0;
                        if (max_window(last_ack_seqno, curr_ack_seqno) == last_ack_seqno && curr_swnd.last_ack_rwnd_size == curr_rwnd_size) {
                            // Received ACK is Duplicate
                            printf("SOCKET %d => DUPLICATE ACK\n", i);
                            up(mutex_swnd);
                            continue;
                        }

                        printf("SOCKET %d => ACK RECEIVED\n", i);

                        if (max_window(last_ack_seqno, curr_ack_seqno) == last_ack_seqno && curr_swnd.last_ack_rwnd_size != curr_rwnd_size) {
                            dup_ack = 1;
                        }

                        down(mutex_sendbuf);

                        int k = MTP_Table[i].swnd.left_idx;

                        if(!dup_ack) {
                            while (MTP_Table[i].send_buffer[k].header.seq_number != curr_ack_seqno) {
                                MTP_Table[i].send_buffer[k].header.seq_number = -1;
                                k = (k + 1) % SEND_BUFFER_SIZE;
                            }

                            MTP_Table[i].send_buffer[k].header.seq_number = -1;
                            k = (k+1)%SEND_BUFFER_SIZE;
                        }
                        
                        printf("SOCKET %d => PREV snwd => [%d, %d]\n",i, MTP_Table[i].swnd.left_idx, MTP_Table[i].swnd.right_idx);
                        
                        MTP_Table[i].swnd.left_idx = k;
                        MTP_Table[i].swnd.right_idx = (k + curr_rwnd_size - 1) % SEND_BUFFER_SIZE;
                        MTP_Table[i].swnd.last_ack_seqno = curr_ack_seqno;

                        printf("SOCKET %d => UPD snwd => [%d, %d]\n", i, MTP_Table[i].swnd.left_idx, MTP_Table[i].swnd.right_idx);

                        up(mutex_sendbuf);

                        up(mutex_swnd);
                    }

                    else {
                        down(mutex_recvbuf);
                        int seq_no = (int)(buffer[1]-'a');

                        for (int k = 0; k < RECEIVER_WINDOW_SIZE; k++) {
                            if (MTP_Table[i].rwnd.window[k] == seq_no) {
                                for (int j = 0; j < RECV_BUFFER_SIZE; j++) {
                                    if (MTP_Table[i].receive_buffer[j].header.seq_number == -1) {
                                        MTP_Table[i].receive_buffer[j].header.seq_number = seq_no;
                                        strncpy(MTP_Table[i].receive_buffer[j].data, buffer + 2, MESSAGE_SIZE);

                                        break;
                                    }
                                }
                                break;
                            }
                        }

                        int seq_numbers[MAX_SEQ_NUMBER + 1];
                        int curr_rwnd_size = 0;

                        memset(seq_numbers, 0, sizeof(seq_numbers));
                        
                        for (int k = 0; k < RECV_BUFFER_SIZE; k++) {
                            if (MTP_Table[i].receive_buffer[k].header.seq_number == -1) {
                                curr_rwnd_size++;
                            }
                            else {
                                seq_numbers[MTP_Table[i].receive_buffer[k].header.seq_number] = 1;
                            }
                        }

                        int curr = (MTP_Table[i].rwnd.last_inorder_received) % MAX_SEQ_NUMBER + 1;

                        while(1) {
                            if(seq_numbers[curr] != 1) {
                                break;
                            }

                            MTP_Table[i].rwnd.last_inorder_received = curr;

                            curr = (curr)%MAX_SEQ_NUMBER + 1;
                        }

                        // Update receive window
                        if (curr_rwnd_size) {

                            int curr = (MTP_Table[i].rwnd.last_inorder_received) % MAX_SEQ_NUMBER + 1;

                            memset(MTP_Table[i].rwnd.window, -1, sizeof(MTP_Table[i].rwnd.window));
                            int curr_rwnd_idx = 0;
                            for (int j = 1; j <= curr_rwnd_size; j++) {
                                while (seq_numbers[curr] != 0) {
                                    curr = (curr) % MAX_SEQ_NUMBER + 1;
                                }

                                MTP_Table[i].rwnd.window[curr_rwnd_idx++] = curr;
                                curr = (curr)%MAX_SEQ_NUMBER + 1;
                            }

                            while (curr_rwnd_idx < RECEIVER_WINDOW_SIZE) {
                                MTP_Table[i].rwnd.window[curr_rwnd_idx++] = -1;
                            }

                            MTP_Table[i].rwnd.full = 0;

                            // Send the ACK
                            char data_ACK[3] = {'A', (char)(MTP_Table[i].rwnd.last_inorder_received+'a'), (char)(curr_rwnd_size+'a')};
                            int udp_id = MTP_Table[i].udp_sockid;

                            struct sockaddr_in dest_addr;
                            dest_addr.sin_family = AF_INET;
                            dest_addr.sin_port = htons(MTP_Table[i].dest_port);
                            dest_addr.sin_addr.s_addr = inet_addr(MTP_Table[i].dest_ip);

                            int bytes = sendto(udp_id, data_ACK, 3, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));

                            if (curr_rwnd_size == 0) {
                                MTP_Table[i].rwnd.full = 1;
                            }

                            up(mutex_recvbuf);
                        }
                    }
                }
                
                else {

                    if (!MTP_Table[i].rwnd.full) continue;

                    down(mutex_recvbuf);

                    int seq_numbers[MAX_SEQ_NUMBER+1];
                    memset(seq_numbers, 0, sizeof(seq_numbers));

                    int rwnd_size = 0;
                    for (int k = 0; k < RECV_BUFFER_SIZE; k++) {
                        if (MTP_Table[i].receive_buffer[k].header.seq_number == -1) {
                            rwnd_size++;
                        }
                        else {
                            seq_numbers[MTP_Table[i].receive_buffer[k].header.seq_number] = 1;
                        }
                    }

                    if (rwnd_size) {
                        int curr_rwnd_idx = 0;

                        int curr = (MTP_Table[i].rwnd.last_inorder_received) % MAX_SEQ_NUMBER + 1;

                        memset(MTP_Table[i].rwnd.window, -1, sizeof(MTP_Table[i].rwnd.window));
                        for (int j = 1; j <= rwnd_size; j++) {
                            while (seq_numbers[curr] != 0) {
                                curr = (curr) % MAX_SEQ_NUMBER + 1;
                            }

                            MTP_Table[i].rwnd.window[curr_rwnd_idx++] = curr;
                            curr = (curr)%MAX_SEQ_NUMBER + 1;
                        }

                        while (curr_rwnd_idx < RECEIVER_WINDOW_SIZE) {
                            MTP_Table[i].rwnd.window[curr_rwnd_idx++] = -1;
                        }
                    
                        // Send the ACK
                        char data_ACK[3] = {'A', (char)(MTP_Table[i].rwnd.last_inorder_received+'a'), (char)(rwnd_size+'a')};
                        int udp_id = MTP_Table[i].udp_sockid;

                        printf("SOCKET %d => Sending ACK, UPD rwnd size => %s\n",i+1, data_ACK);

                        struct sockaddr_in dest_addr;
                        dest_addr.sin_family = AF_INET;
                        dest_addr.sin_port = htons(MTP_Table[i].dest_port);
                        dest_addr.sin_addr.s_addr = inet_addr(MTP_Table[i].dest_ip);

                        int bytes = sendto(udp_id, data_ACK, 3, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
                    }

                    up(mutex_recvbuf);
                }
            }
        }
    }

    pthread_exit(NULL);
}

void *thread_S_func(void * arg){
    mtpSocket * MTP_Table = get_shared_MTP_Table();
    SOCK_INFO * sock_info = get_SOCK_INFO();

    down(mutex_swnd);
    down(mutex_sendbuf);

    for (int i = 0; i < MAX_SOCKETS; i++) {
        for (int k = 0; k < SEND_BUFFER_SIZE; k++) {
            MTP_Table[i].send_buffer[k].header.seq_number = -1;
            memset(MTP_Table[i].send_buffer[k].data, 0, MESSAGE_SIZE);
        }
        MTP_Table[i].swnd.left_idx = 0;
        MTP_Table[i].swnd.right_idx = (RECV_BUFFER_SIZE - 1) % SEND_BUFFER_SIZE;
        MTP_Table[i].swnd.fresh_msg = 0;
        MTP_Table[i].swnd.last_seq_no = 0;
        MTP_Table[i].swnd.last_sent = -1;
        MTP_Table[i].swnd.last_ack_seqno = 0;

        for (int k = 0; k < SENDER_WINDOW_SIZE; k++) {
            MTP_Table[i].swnd.last_sent_time[k] = 0;
        }
    }

    up(mutex_sendbuf);
    up(mutex_swnd);

    while(1){
        sleep(T/2);

        for(int i = 0; i < MAX_SOCKETS; i++){
            if(!MTP_Table[i].available){
                down(mutex_swnd);
                down(mutex_sendbuf);

                send_window curr_swnd = MTP_Table[i].swnd;
                
                int left = curr_swnd.left_idx;
                int right = curr_swnd.right_idx;

                bool timeout = 0;
                if ((right + 1) % SEND_BUFFER_SIZE == left) {
                    up(mutex_sendbuf);
                    up(mutex_swnd);
                    continue;
                }

                // logs
                // logs();

                time_t curr_time = time(NULL);
                while (left != (right + 1) % SEND_BUFFER_SIZE) {
                    if ((curr_time - curr_swnd.last_sent_time[left] > T) && (curr_swnd.last_sent_time[left] > 0) && (MTP_Table[i].send_buffer[left].header.seq_number > 0)) {
                        timeout = 1;
                        break;
                    }
                    left = (left + 1) % SEND_BUFFER_SIZE;
                }

                if (timeout) {
                    left = curr_swnd.left_idx;
                    right = curr_swnd.right_idx;

                    while (left != (right + 1) % SEND_BUFFER_SIZE) {
                        if(MTP_Table[i].send_buffer[left].header.seq_number < 0) {
                            break;
                        }

                        total_sendto_calls++;

                        char buffer[MESSAGE_SIZE + 2];
                        buffer[0] = 'D';
                        buffer[1] = (char)(MTP_Table[i].send_buffer[left].header.seq_number + 'a');
                        strncpy(buffer + 2, MTP_Table[i].send_buffer[left].data, MESSAGE_SIZE);

                        struct sockaddr_in dest_addr;
                        dest_addr.sin_family = AF_INET;
                        dest_addr.sin_port = htons(MTP_Table[i].dest_port);
                        dest_addr.sin_addr.s_addr = inet_addr(MTP_Table[i].dest_ip);

                        int bytes = sendto(MTP_Table[i].udp_sockid, buffer, MESSAGE_SIZE + 2, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));

                        MTP_Table[i].swnd.last_sent_time[left] = time(NULL);
                        printf("SOCKET %d => SEND \"%s\" as TIMEOUT of SEND @ %ld\n", i, buffer, MTP_Table[i].swnd.last_sent_time[left]);

                        MTP_Table[i].swnd.last_sent = left;

                        left = (left + 1) % SEND_BUFFER_SIZE;
                    }
                }
                else {
                    left = (curr_swnd.last_sent + 1)%SEND_BUFFER_SIZE;
                    right = curr_swnd.right_idx;

                    while (left != (right + 1) % SEND_BUFFER_SIZE) {
                        if(MTP_Table[i].send_buffer[left].header.seq_number < 0) {
                            break;
                        }

                        total_sendto_calls++;

                        char buffer[MESSAGE_SIZE + 2];
                        buffer[0] = 'D';
                        buffer[1] = (char)(MTP_Table[i].send_buffer[left].header.seq_number + 'a');

                        strncpy(buffer + 2, MTP_Table[i].send_buffer[left].data, MESSAGE_SIZE);

                        struct sockaddr_in dest_addr;
                        dest_addr.sin_family = AF_INET;
                        dest_addr.sin_port = htons(MTP_Table[i].dest_port);
                        dest_addr.sin_addr.s_addr = inet_addr(MTP_Table[i].dest_ip);

                        int bytes = sendto(MTP_Table[i].udp_sockid, buffer, MESSAGE_SIZE + 2, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));

                        MTP_Table[i].swnd.last_sent_time[left] = time(NULL);
                        printf("SOCKET %d => SEND \"%s\" as No ACK for msg SENT @ %ld\n", i, buffer, MTP_Table[i].swnd.last_sent_time[left]);

                        MTP_Table[i].swnd.last_sent = left;

                        left = (left + 1) % SEND_BUFFER_SIZE;
                    }
                }

                up(mutex_sendbuf);
                up(mutex_swnd);
            }
        }
    }

    pthread_exit(NULL);
}

void *thread_G_func(void *arg) {
    mtpSocket * MTP_Table = get_shared_MTP_Table();

    // Implement garbage collector thread logic here
    while (1) {
        sleep(10); // Check every minute

        printf("\n\nGarbage Collector => \n");
        printTable();
        printf("*****************************************\n");

        printf("\n\nTotal sendto calls => %d\n\n", total_sendto_calls);

        for(int i = 0; i < MAX_SOCKETS; i++) {
            down(mutex);

            if(MTP_Table[i].available == 0){

                // Use waitpid to check the status of the specified process
                int status;
                int pid = MTP_Table[i].pid;

                int result = kill(pid, 0);
                
                if (result == 0) {
                    printf("Process with PID %d exists.\n", pid);
                }
                else {
                    // Process has exited
                    printf("Process with PID %d has exited.\n", pid);

                    printf("Clearing the socket with ID %d\n", i);
                    MTP_Table[i].available = 1;
                    MTP_Table[i].pid = -1;
                    MTP_Table[i].udp_sockid = -1;
                    
                    // Close the UDP socket
                    m_close(i);
                }
            }

            up(mutex);
        }
    }

    shmdt(MTP_Table);

    pthread_exit(NULL);
}

int dropMessage(float p) {
    srand(time(NULL));
    float r = (float)rand() / (float)RAND_MAX;

    if(r < p) {
        return 1;
    }

    return 0;
}