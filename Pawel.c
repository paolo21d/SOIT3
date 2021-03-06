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
#define MUTEXPRODKEY 1606

#define ILOSC_KONSUMENTOW 5
#define ILOSC_PRODUCENTOW 3
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
    //printf("Wsadzam %c, size: %d\n", val, q->length);
}
char getFromBuf(struct Queue *q){
    char ret = q->buf[q->head];
    q->head = (q->head+1)%BUFSIZE;
    q->length--;
    //printf("Wyjmuje %d\n", ret);
    return ret;
}
void printfQueue(struct Queue *q, int nr){
    printf("Queue %d size %d: ", nr, q->length);
    int index = q->head;
    for(int i=0; i<q->length; ++i, ++index)
        printf("%d ", q->buf[index%BUFSIZE]);

    printf("\n");
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
    for(int i=0; i<ILOSC_PRODUCENTOW; ++i){
        bb[i].sem_num=i;
        bb[i].sem_op = -1;
        bb[i].sem_flg = 0;
    }
    if(semop(semid, bb, ILOSC_PRODUCENTOW) == -1){
        perror("Opuszczenie semafora downAll");
        exit(1);
    }
}
void upAll(int semid){
    for(int i=0; i<ILOSC_PRODUCENTOW; ++i){
        bb[i].sem_num=i;
        bb[i].sem_op = 1;
        bb[i].sem_flg = 0;
    }
    if(semop(semid, bb, ILOSC_PRODUCENTOW) == -1){
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
    srand(time(NULL)+nr*123);
    int buffid = shmget(BUFFKEY, ILOSC_KOLEJEK*sizeof(struct Queue), 0600);
    struct Queue *buffer = (struct Queue*)shmat(buffid, NULL, 0);
    int mutex = semget(MUTSYSKEY, ILOSC_KOLEJEK, 0600);
    int emptyid = semget(EMPTYKEY, ILOSC_KOLEJEK, 0600);
    int fullid = semget(FULLKEY, ILOSC_KOLEJEK, 0600);
    int groupEmptyId = semget(GROUPEMPTY, 1, 0600);
    int mutexProd = semget(MUTEXPRODKEY, 1, 0600);
    int numeryKolejek[ILOSC_KOLEJEK];
    int wsadzamDo;
    ///
    printf("*********************Producer %d\n", nr);
while(1){
    losujNumeryKolejek(numeryKolejek); //wylosowac ciag kolejek do ktorych probowac wejsc
    downS(groupEmptyId, 0); // czekanie az chociaz jedna kolejka bedzie NIEpelna
    downS(mutexProd, 0); // zablokowanie wszystkich producentow
    for(int i=0; i<ILOSC_KOLEJEK; ++i){ //szukanie pierwszej nie pelnej kolejki
        if(semctl(emptyid, numeryKolejek[i], GETVAL) != 0){
            wsadzamDo = numeryKolejek[i];
            break;
        }
    }
    downS(emptyid, wsadzamDo);
    upS(mutexProd, 0); //odblokowanie producentow
    downS(mutex, wsadzamDo);
        printf("PRODUCER %d do %d\n", nr, wsadzamDo);
        putToBuf(&buffer[wsadzamDo], nr);
        printfQueue(&buffer[wsadzamDo], wsadzamDo);
    upS(mutex, wsadzamDo);
    upS(fullid, wsadzamDo);
    sleep(1);
}
}
void Consumer(int nr){ //konument wyjmuje zawsze z tej samej kolejki o indexie: nr
    int buffid = shmget(BUFFKEY, ILOSC_KOLEJEK*sizeof(struct Queue), 0600);
    struct Queue *buffer = (struct Queue*)shmat(buffid, NULL, 0);
    int mutex = semget(MUTSYSKEY, ILOSC_KOLEJEK, 0600);
    int emptyid = semget(EMPTYKEY, ILOSC_KOLEJEK, 0600);
    int fullid = semget(FULLKEY, ILOSC_KOLEJEK, 0600);
    int groupEmptyId = semget(GROUPEMPTY, 1, 0600);
    ///
    printf("*********************Consumer %d\n", nr);
    while(1){
        downS(fullid, nr);
        downS(mutex, nr);
            printf("\tCONSUMER %d\n", nr);
            getFromBuf(&buffer[nr]);
            printfQueue(&buffer[nr], nr);
        upS(mutex, nr);
        upS(emptyid, nr);
        upS(groupEmptyId, 0); // usunieto 1 elem z jakiejs kolejki = wzrosla liczba pustych elementow
        sleep(2);
    }
}
int main() {
    int buffid = shmget(BUFFKEY, ILOSC_KOLEJEK*sizeof(struct Queue), IPC_CREAT|0600); //pamiec wspoldzielona
    struct Queue *buffer = (struct Queue*)shmat(buffid, NULL, 0);

    int mutex = semget(MUTSYSKEY, ILOSC_KOLEJEK, IPC_CREAT|IPC_EXCL|0600); //mutex - blokuje dostep do danej kolejki
    if(mutex==-1){
        mutex = semget(MUTSYSKEY, ILOSC_KOLEJEK, 0600);
        if(mutex==-1) perror("BLAD SEMID");
    }
    for(int i=0; i<ILOSC_KOLEJEK; ++i)
        semctl(mutex, i, SETVAL, (int)1);

    int emptyid = semget(EMPTYKEY, ILOSC_KOLEJEK, IPC_CREAT|IPC_EXCL|0600); //ile jest elemtnow empty w danej kolejce
    if(emptyid==-1){
        emptyid = semget(EMPTYKEY, ILOSC_KOLEJEK, 0600);
        if(emptyid==-1) perror("BLAD EMPTYID");
    }
    for(int i=0; i<ILOSC_KOLEJEK; ++i)
        semctl(emptyid, i, SETVAL, (int)BUFSIZE);

    int fullid = semget(FULLKEY, ILOSC_KOLEJEK, IPC_CREAT|IPC_EXCL|0600); //ile jest elemnetow full w danej kolejce
    if(fullid==-1){
        fullid = semget(FULLKEY, ILOSC_KOLEJEK, 0600);
        if(fullid==-1) perror("BLAD FULLID");
    }
    for(int i=0; i<ILOSC_KOLEJEK; ++i)
        semctl(fullid, i, SETVAL, (int)0);

    int groupEmptyId = semget(GROUPEMPTY, 1, IPC_CREAT|IPC_EXCL|0600); //ile jest w ogolnosci we wszystkich kolejkach elem empty
    if(groupEmptyId==-1){
        groupEmptyId = semget(GROUPEMPTY, 1, 0600);
        if(groupEmptyId==-1)    perror("BLAD GROUPEMPTYID");
    }
    semctl(groupEmptyId, 0, SETVAL, (int)(ILOSC_KOLEJEK*BUFSIZE));

    int mutexProd = semget(MUTEXPRODKEY, 1, IPC_CREAT|IPC_EXCL|0600); //mutex producentow- blokuje producentow
    if(mutexProd==-1){
        mutexProd = semget(MUTEXPRODKEY, 1, 0600);
        if(mutexProd==-1) perror("BLAD SEMID");
    }
    for(int i=0; i<1; ++i)
        semctl(mutexProd, i, SETVAL, (int)1);

    for(int i=0; i<ILOSC_KOLEJEK; i++){
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

    sleep(50);
    for(int i=0; i<ILOSC_KONSUMENTOW+ILOSC_PRODUCENTOW; ++i)
        kill(pid[i], SIGKILL);
    shmdt(buffer);
}


