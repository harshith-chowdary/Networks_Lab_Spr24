#include <msocket.h>

int main() {
    key_t sm_key = ftok(".", MTP_TABLE);
    int sm_id = shmget(sm_key, (MAX_SOCKETS)*sizeof(mtpSocket), 0777|IPC_CREAT);
    mtpSocket *mtpTable = (mtpSocket *)shmat(sm_id, 0, 0);

    shmctl(sm_id, IPC_RMID, NULL);

    sm_key = ftok(".", SHARED_RESOURCE);
    int sm_id_sock_info = shmget(sm_key, sizeof(int), 0777|IPC_CREAT);
    SOCK_INFO *vars = (SOCK_INFO *)shmat(sm_id_sock_info, 0, 0);

    shmctl(sm_id_sock_info, IPC_RMID, NULL);

    sm_key = ftok(".", SEM1);
    int sem1 = semget(sm_key, 1, 0777|IPC_CREAT);
    semctl(sem1, 0, IPC_RMID, 0);

    sm_key = ftok(".", SEM2);
    int sem2 = semget(sm_key, 1, 0777|IPC_CREAT);
    semctl(sem2, 0, IPC_RMID, 0);

    sm_key = ftok(".", MUTEX);
    int mutex = semget(sm_key, 1, 0777|IPC_CREAT);
    semctl(mutex, 0, IPC_RMID, 0);

    return 0;
}