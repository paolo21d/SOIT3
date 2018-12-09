///Z niewiadomego powodu przy kazdym nastepnym uruchomieniu nalezy
///zmienic wartosc define BUFFKEY, EMPTYKEY, FULLKEY, MUTSYSKEY
///Zmienic na wartosc ktora wczesniej nie wystepowala
///czyli np cyfr setek zmianiac co uruchomienie i tak od 0 do 9
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/sem.h>

#define BUFSIZE 9
#define BUFFKEY   2500
#define EMPTYKEY  2501
#define FULLKEY   2502
#define MUTSYSKEY 2503
static struct sembuf buf;
static struct sembuf bb[3];

struct Queue {
    char buf[BUFSIZE];
    int head, tail, length;
};
void putToBuf(struct Queue *q, char val) {
    q->buf[q->tail] = val;
    q->tail = (q->tail+1)%BUFSIZE;
    q->length++;
    printf("Wsadzam %c, size: %d\n", val, q->length);
}
char getFromBuf(struct Queue *q){
    char ret = q->buf[q->head];
    q->head = (q->head+1)%BUFSIZE;
    q->length--;
    printf("Wyjmuje %d\n", ret);
    return ret;
}
///////////////////////
void upS(int semid, int semnum){
    buf.sem_num = semnum;
 	buf.sem_op = 1;
	buf.sem_flg = 0;
	if (semop(semid, &buf, 1) == -1){
 		perror("Podnoszenie semafora");
		exit(1);
	}
}
void downS(int semid, int semnum){
    buf.sem_num = semnum;
	buf.sem_op = -1;
	buf.sem_flg = 0;
 	if (semop(semid, &buf, 1) == -1){
		perror("Opuszczenie semafora");
		exit(1);
 	}
}
void downC1S(int semid){
    for(int i=0; i<3; i++){
        bb[i].sem_num = i;
        //bb[i].sem_op = -1;
        bb[i].sem_flg = 0;
    }
    bb[0].sem_op = -1;
    bb[1].sem_op = -4;
    bb[2].sem_op = -3;
    if(semop(semid, &bb, 3) == -1){
        perror("DownC1S");
        exit(1);
    }
}
void downC2S(int semid){
    for(int i=0; i<3; i++){
        bb[i].sem_num = i;
        //bb[i].sem_op = -1;
        bb[i].sem_flg = 0;
    }
    bb[0].sem_op = -5;
    bb[1].sem_op = -2;
    bb[2].sem_op = -1;
    if(semop(semid, &bb, 3) == -1){
        perror("DownC2S");
        exit(1);
    }
}
void downC3S(int semid){
    for(int i=0; i<3; i++){
        bb[i].sem_num = i;
        //bb[i].sem_op = -1;
        bb[i].sem_flg = 0;
    }
    bb[0].sem_op = -1;
    bb[1].sem_op = -1;
    bb[2].sem_op = -1;
    if(semop(semid, &bb, 3) == -1){
        perror("DownC3S");
        exit(1);
    }
}
void upC1S(int semid){
    for(int i=0; i<3; i++){
        bb[i].sem_num = i;
        bb[i].sem_flg = 0;
    }
    bb[0].sem_op = 1;
    bb[1].sem_op = 4;
    bb[2].sem_op = 3;
    if(semop(semid, &bb, 3) == -1){
        perror("UPC1S");
        exit(1);
    }
}
void upC2S(int semid){
    for(int i=0; i<3; i++){
        bb[i].sem_num = i;
        bb[i].sem_flg = 0;
    }
    bb[0].sem_op = 5;
    bb[1].sem_op = 2;
    bb[2].sem_op = 1;
    if(semop(semid, &bb, 3) == -1){
        perror("UPC2S");
        exit(1);
    }
}
void upC3S(int semid){
    for(int i=0; i<3; i++){
        bb[i].sem_num = i;
        bb[i].sem_flg = 0;
    }
    bb[0].sem_op = 1;
    bb[1].sem_op = 1;
    bb[2].sem_op = 1;
    if(semop(semid, &bb, 3) == -1){
        perror("UPC3S");
        exit(1);
    }
}
void downAll(int semid){
    for(int i=0; i<3; ++i){
        bb[i].sem_num = i;
        bb[i].sem_flg = 0;
        bb[i].sem_op = -1;
    }
    if(semop(semid, &bb, 3) == -1){
        perror("DownAll");
        exit(1);
    }
    //printf("PO DOWNALL\n");
}
void upAll(int semid){
    //printf("PRZED UPALL\n");
    for(int i=0; i<3; ++i){
        bb[i].sem_num = i;
        bb[i].sem_flg = 0;
        bb[i].sem_op = 1;
    }
    //printf("ZARAZ SEMOP\n");
    if(semop(semid, &bb, 3) == -1){
        perror("UpAll");
        exit(1);
    }
    //printf("PO UPALL\n");
}
void Producent(int nr){
    int buffid = shmget(BUFFKEY, 3*sizeof(struct Queue), 0600);
    struct Queue *buffer = (struct Queue*)shmat(buffid, NULL, 0);
    int semid = semget(MUTSYSKEY, 3, 0600);
    int emptyid = semget(EMPTYKEY, 3, 0600);
    int fullid = semget(FULLKEY, 3, 0600);
    ///
    printf("Producer %d\n", nr);
while(1){
    //printf("prod, przed down empty\n");
    downS(emptyid, nr);
    //printf("prod, przed down semid\n");
    //printf("Prod %d, semid val: %d %d %d\n", nr, semctl(semid, 0, GETVAL), semctl(semid, 1, GETVAL), semctl(semid, 2, GETVAL));
    downS(semid, nr);
        printf("\tPRODUCER %d\n", nr);
        putToBuf(&buffer[nr], nr);
    upS(semid, nr);
    upS(fullid, nr);
    sleep(1);
}
}
void Consumer(int nr){
    int buffid = shmget(BUFFKEY, 3*sizeof(struct Queue), 0600);
    struct Queue *buffer = (struct Queue*)shmat(buffid, NULL, 0);
    int semid = semget(MUTSYSKEY, 3, 0600);
    int emptyid = semget(EMPTYKEY, 3, 0600);
    int fullid = semget(FULLKEY, 3, 0600);
    ///
    printf("Consumer %d\n", nr);
    if(nr==0){ ///Consumer 0
    while(1){
        downC1S(fullid);
        downAll(semid);
            printf("\tCONSUMER %d\n", nr);
            for(int i=0; i<1; ++i) getFromBuf(&buffer[0]); //kolejka A
            for(int i=0; i<4; ++i) getFromBuf(&buffer[1]); //kolejka B
            for(int i=0; i<3; ++i) getFromBuf(&buffer[2]); //kolejka C
        upAll(semid);
        upC1S(emptyid);
        sleep(2);
    }
    } else if(nr==1){ ///Consumer 1
    while(1){
        downC2S(fullid);
        downAll(semid);
            printf("\tCONSUMER %d\n", nr);
            for(int i=0; i<5; ++i) getFromBuf(&buffer[0]); //kolejka A
            for(int i=0; i<2; ++i) getFromBuf(&buffer[1]); //kolejka B
            for(int i=0; i<1; ++i) getFromBuf(&buffer[2]); //kolejka C
        upAll(semid);
        upC2S(emptyid);
        sleep(2);
    }
    } else if(nr==2){ ///Consumer 2
    while(1){
        downC3S(fullid);
        downAll(semid);
            printf("\tCONSUMER %d\n", nr);
            for(int i=0; i<1; ++i) getFromBuf(&buffer[0]); //kolejka A
            for(int i=0; i<1; ++i) getFromBuf(&buffer[1]); //kolejka B
            for(int i=0; i<1; ++i) getFromBuf(&buffer[2]); //kolejka C
        //printf("Cons 2, semid val: %d %d %d\n", semctl(semid, 0, GETVAL), semctl(semid, 1, GETVAL), semctl(semid, 2, GETVAL));
        upAll(semid);
        //printf("Cons 2, semid val: %d %d %d\n", semctl(semid, 0, GETVAL), semctl(semid, 1, GETVAL), semctl(semid, 2, GETVAL));
        upC3S(emptyid);
        //printf("Cons 2, semid val: %d %d %d\n", semctl(semid, 0, GETVAL), semctl(semid, 1, GETVAL), semctl(semid, 2, GETVAL));
        sleep(2);
    }
    }
}
int main() {
    int buffid = shmget(BUFFKEY, 3*sizeof(struct Queue), IPC_CREAT|0600);
    struct Queue *buffer = (struct Queue*)shmat(buffid, NULL, 0);

    int semid = semget(MUTSYSKEY, 3, IPC_CREAT|IPC_EXCL|0600); // 0 - mutex
    if(semid==-1){
        semid = semget(MUTSYSKEY, 3, 0600);
        if(semid==-1) perror("BLAD SEMID");
    }
    semctl(semid, 0, SETVAL, (int)1);
    semctl(semid, 1, SETVAL, (int)1);
    semctl(semid, 2, SETVAL, (int)1);

    int emptyid = semget(EMPTYKEY, 3, IPC_CREAT|IPC_EXCL|0600); // semafor zliczajacy ile jest elem empty w kazdej kolejce
    if(emptyid==-1){
        emptyid = semget(EMPTYKEY, 3, 0600);
        if(emptyid==-1) perror("BLAD EMPTYID");
    }
    semctl(emptyid, 0, SETVAL, (int)BUFSIZE);
    semctl(emptyid, 1, SETVAL, (int)BUFSIZE);
    semctl(emptyid, 2, SETVAL, (int)BUFSIZE);

    int fullid = semget(FULLKEY, 3, IPC_CREAT|IPC_EXCL|0600); // semafor zliczajacy ile jest elem full w kazdej kolejce
    if(fullid==-1){
        fullid = semget(FULLKEY, 3, 0600);
        if(fullid==-1) perror("BLAD FULLID");
    }
    semctl(fullid, 0, SETVAL, (int)0);
    semctl(fullid, 1, SETVAL, (int)0);
    semctl(fullid, 2, SETVAL, (int)0);

    for(int i=0; i<3; i++){ //inicjacja poczatkowa kolejek
        buffer[i].length=0;
        buffer[i].head=0;
        buffer[i].tail=0;
    }

    pid_t pid[6];
    for(int i=0; i<3; ++i){ // odpalenie procesow producentow
        pid[i]=fork();
        if(pid[i]==0){ Producent(i); }
    }
    for(int i=3; i<6; ++i){ // odpalenie procesow konsumantow
        pid[i]=fork();
        if(pid[i]==0){ Consumer(i-3); }
    }

    sleep(20);
    for(int i=0; i<6; ++i) // zabicie wszytkich procesow
        kill(pid[i], SIGKILL);
    shmdt(buffer);
}

