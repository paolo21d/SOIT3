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
#include <time.h>

#define BUFSIZE 10
#define BUFFKEY   1600
#define EMPTYKEY  1601
#define FULLKEY   1602
#define MUTSYSKEY 1603
#define GROUPEMPTY 1604
#define MUTEXFORKEY 1605

#define ILOSC_KONSUMENTOW 5
#define ILOSC_PRODUCENTOW 2
#define ILOSC_KOLEJEK 5
static struct sembuf buf;
static struct sembuf bb[ILOSC_KOLEJEK];

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
 		perror("Podnoszenie semafora upS");
		exit(1);
	}
}
void downS(int semid, int semnum){
    buf.sem_num = semnum;
	buf.sem_op = -1;
	buf.sem_flg = 0;
 	if (semop(semid, &buf, 1) == -1){
		perror("Opuszczenie semafora downS");
		exit(1);
 	}
}
void downAll(int semid){
    for(int i=0; i<ILOSC_KOLEJEK; ++i){
        bb[i].sem_num=i;
        bb[i].sem_op = -1;
        bb[i].sem_flg = 0;
    }
    if(semop(semid, &bb, ILOSC_KOLEJEK) == -1){
        perror("Opuszczenie semafora downAll");
        exit(1);
    }
}
void upAll(int semid){
    for(int i=0; i<ILOSC_KOLEJEK; ++i){
        bb[i].sem_num=i;
        bb[i].sem_op = 1;
        bb[i].sem_flg = 0;
    }
    if(semop(semid, &bb, ILOSC_KOLEJEK) == -1){
        perror("Podnoszenie semafora upAll");
        exit(1);
    }
}
void losujNumeryKolejek(int *tab){
    for(int i=0; i<ILOSC_KOLEJEK; ++i)
        tab[i]=i;
    int nrZamiana, tmp;
    for(int i=0; i<ILOSC_KOLEJEK; ++i){
        nrZamiana = rand()%ILOSC_KOLEJEK;
        tmp=tab[i];
        tab[i]=tab[nrZamiana];
        tab[nrZamiana]=tmp;
    }
}


void Producent(int nr){
    int buffid = shmget(BUFFKEY, 5*sizeof(struct Queue), 0600);
    struct Queue *buffer = (struct Queue*)shmat(buffid, NULL, 0);
    int mutex = semget(MUTSYSKEY, 5, 0600);
    int emptyid = semget(EMPTYKEY, 5, 0600);
    int fullid = semget(FULLKEY, 5, 0600);
    int groupEmptyId = semget(GROUPEMPTY, 1, 0600);
    int numeryKolejek[5];
    int wsadzamDo;
    ///
    printf("Producer %d\n", nr);
while(1){
    losujNumeryKolejek(numeryKolejek); //wylosowac ciag kolejek do ktorych probowac wejsc
    downS(groupEmptyId, 0); // czekanie az chociaz jedna kolejka bedzie NIEpelna
    //printf("Producent %d wylosowal kolejki\n", nr);
            //moze tutaj dodac jeszcze jeden semafor blokujacy na wejsciu do tego for-a ??? ///chyba dodalem
    downAll(mutex); //blokada wszystkich kolejek
    for(int i=0; i<5; ++i){ //szukanie pierwszej nie pelnej
        /*buf.sem_num = numeryKolejek[wsadzamDo];
        buf.sem_op = 0;
        buf.sem_flg = 0;
        if( semop(emptyid, &buf, 1) != -1){
            break;
        }*/
        if(buffer[numeryKolejek[i]].length<BUFSIZE){
            wsadzamDo = numeryKolejek[i];
            //printf("Prod %d, wsadzam do %d\n", nr, wsadzamDo);
            break;
        }
    }
    upAll(mutex);
    downS(emptyid, wsadzamDo);
    //upAll(semid);
    //tutaj ewentualnie up na tym sem sprzed for
    downS(mutex, wsadzamDo);
        printf("\tPRODUCER %d\n", nr);
        putToBuf(&buffer[wsadzamDo], nr);
    upS(mutex, wsadzamDo);
    upS(fullid, wsadzamDo);
    sleep(1);
}
}
void Consumer(int nr){ //konument wyjmuje zawsze z tej samej kolejki o indexie: nr
    int buffid = shmget(BUFFKEY, 5*sizeof(struct Queue), 0600);
    struct Queue *buffer = (struct Queue*)shmat(buffid, NULL, 0);
    int semid = semget(MUTSYSKEY, 5, 0600);
    int emptyid = semget(EMPTYKEY, 5, 0600);
    int fullid = semget(FULLKEY, 5, 0600);
    int groupEmptyId = semget(GROUPEMPTY, 1, 0600);
    ///
    printf("Consumer %d\n", nr);
    while(1){
        downS(fullid, nr);
        downS(semid, nr);
            printf("\tCONSUMER %d\n", nr);
            getFromBuf(&buffer[nr]);
        upS(semid, nr);
        upS(emptyid, nr);
        upS(groupEmptyId, 0); // usunieto 1 elem z jakiejs kolejki = wzrosla liczba pustych elementow
        sleep(2);
    }
}
int main() {
    srand(time(NULL));
    int buffid = shmget(BUFFKEY, 5*sizeof(struct Queue), IPC_CREAT|0600);
    struct Queue *buffer = (struct Queue*)shmat(buffid, NULL, 0);

    int semid = semget(MUTSYSKEY, 5, IPC_CREAT|IPC_EXCL|0600); //mutex - blokuje dostep do danej kolejki
    if(semid==-1){
        semid = semget(MUTSYSKEY, 5, 0600);
        if(semid==-1) perror("BLAD SEMID");
    }
    for(int i=0; i<5; ++i)
        semctl(semid, i, SETVAL, (int)1);

    int emptyid = semget(EMPTYKEY, 5, IPC_CREAT|IPC_EXCL|0600); //ile jest elemtnow empty w danej kolejce
    if(emptyid==-1){
        emptyid = semget(EMPTYKEY, 5, 0600);
        if(emptyid==-1) perror("BLAD EMPTYID");
    }
    for(int i=0; i<5; ++i)
        semctl(emptyid, i, SETVAL, (int)BUFSIZE);

    int fullid = semget(FULLKEY, 5, IPC_CREAT|IPC_EXCL|0600); //ile jest elemnetow full w danej kolejce
    if(fullid==-1){
        fullid = semget(FULLKEY, 5, 0600);
        if(fullid==-1) perror("BLAD FULLID");
    }
    for(int i=0; i<5; ++i)
        semctl(fullid, i, SETVAL, (int)0);

    int groupEmptyId = semget(GROUPEMPTY, 1, IPC_CREAT|IPC_EXCL|0600); //ile jest w ogolnosci we wszystkich kolejkach elem empty
    if(groupEmptyId==-1){
        groupEmptyId = semget(GROUPEMPTY, 1, 0600);
        if(groupEmptyId==-1)    perror("BLAD GROUPEMPTYID");
    }
    semctl(groupEmptyId, 0, SETVAL, (int)(5*BUFSIZE));

    for(int i=0; i<5; i++){
        buffer[i].length=0;
        buffer[i].head=0;
        buffer[i].tail=0;
    }

    pid_t pid[ILOSC_KONSUMENTOW+ILOSC_PRODUCENTOW];
    for(int i=0; i<ILOSC_PRODUCENTOW; ++i){
        pid[i]=fork();
        if(pid[i]==0){ Producent(i); }
    }
    for(int i=ILOSC_PRODUCENTOW; i<ILOSC_KONSUMENTOW+ILOSC_PRODUCENTOW; ++i){ //5 Konsumentow
        pid[i]=fork();
        if(pid[i]==0){ Consumer(i-ILOSC_PRODUCENTOW); }
    }

    sleep(20);
    for(int i=0; i<ILOSC_KONSUMENTOW+ILOSC_PRODUCENTOW; ++i)
        kill(pid[i], SIGKILL);
    shmdt(buffer);
}


