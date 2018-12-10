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
#define GROUPEMPTYKEY 1604

#define ILOSC_KOLEJEK 4

static struct sembuf buf;
static struct sembuf bb[4];

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
void upS(int semid, int semnum, int ile){
    buf.sem_num = semnum;
 	buf.sem_op = ile;
	buf.sem_flg = 0;
	if (semop(semid, &buf, 1) == -1){
 		perror("Podnoszenie semafora upS");
		exit(1);
	}
}
void downS(int semid, int semnum, int ile){
    buf.sem_num = semnum;
	buf.sem_op = -ile;
	buf.sem_flg = 0;
 	if (semop(semid, &buf, 1) == -1){
		perror("Opuszczenie semafora downS");
		exit(1);
 	}
}

void downGroup(int semid, int buffA, int buffB, int buffC, int buffD){
    int ilosc=0;
    if(buffA>0){
        bb[ilosc].sem_num = 0;
        bb[ilosc].sem_flg = 0;
        bb[ilosc].sem_op = -1;
        ilosc++;
    }
    if(buffB>0){
        bb[ilosc].sem_num = 1;
        bb[ilosc].sem_flg = 0;
        bb[ilosc].sem_op = -1;
        ilosc++;
    }
    if(buffC>0){
        bb[ilosc].sem_num = 2;
        bb[ilosc].sem_flg = 0;
        bb[ilosc].sem_op = -1;
        ilosc++;
    }
    if(buffD>0){
        bb[ilosc].sem_num = 3;
        bb[ilosc].sem_flg = 0;
        bb[ilosc].sem_op = -1;
        ilosc++;
    }

    if(semop(semid, bb, ilosc) == -1){
        perror("DownGroup");
        exit(1);
    }
}
void upGroup(int semid, int buffA, int buffB, int buffC, int buffD){
    int ilosc=0;
    if(buffA>0){
        bb[ilosc].sem_num = 0;
        bb[ilosc].sem_flg = 0;
        bb[ilosc].sem_op = 1;
        ilosc++;
    }
    if(buffB>0){
        bb[ilosc].sem_num = 1;
        bb[ilosc].sem_flg = 0;
        bb[ilosc].sem_op = 1;
        ilosc++;
    }
    if(buffC>0){
        bb[ilosc].sem_num = 2;
        bb[ilosc].sem_flg = 0;
        bb[ilosc].sem_op = 1;
        ilosc++;
    }
    if(buffD>0){
        bb[ilosc].sem_num = 3;
        bb[ilosc].sem_flg = 0;
        bb[ilosc].sem_op = 1;
        ilosc++;
    }

    if(semop(semid, bb, ilosc) == -1){
        perror("UpGroup");
        exit(1);
    }
}
void downClient(int semid, int buffA, int buffB, int buffC, int buffD){
    int ilosc=0;
    if(buffA>0){
        bb[ilosc].sem_num = 0;
        bb[ilosc].sem_flg = 0;
        bb[ilosc].sem_op = -buffA;
        ilosc++;
    }
    if(buffB>0){
        bb[ilosc].sem_num = 1;
        bb[ilosc].sem_flg = 0;
        bb[ilosc].sem_op = -buffB;
        ilosc++;
    }
    if(buffC>0){
        bb[ilosc].sem_num = 2;
        bb[ilosc].sem_flg = 0;
        bb[ilosc].sem_op = -buffC;
        ilosc++;
    }
    if(buffD>0){
        bb[ilosc].sem_num = 3;
        bb[ilosc].sem_flg = 0;
        bb[ilosc].sem_op = -buffD;
        ilosc++;
    }

    if(semop(semid, bb, ilosc) == -1){
        perror("DownClient");
        exit(1);
    }
}
void upClient(int semid, int buffA, int buffB, int buffC, int buffD){
    int ilosc=0;
    if(buffA>0){
        bb[ilosc].sem_num = 0;
        bb[ilosc].sem_flg = 0;
        bb[ilosc].sem_op = buffA;
        ilosc++;
    }
    if(buffB>0){
        bb[ilosc].sem_num = 1;
        bb[ilosc].sem_flg = 0;
        bb[ilosc].sem_op = buffB;
        ilosc++;
    }
    if(buffC>0){
        bb[ilosc].sem_num = 2;
        bb[ilosc].sem_flg = 0;
        bb[ilosc].sem_op = buffC;
        ilosc++;
    }
    if(buffD>0){
        bb[ilosc].sem_num = 3;
        bb[ilosc].sem_flg = 0;
        bb[ilosc].sem_op = buffD;
        ilosc++;
    }

    if(semop(semid, bb, ilosc) == -1){
        perror("UpClient");
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
void Producent(int sleepProducer){
    srand(time(NULL));
    int buffid = shmget(BUFFKEY, 4*sizeof(struct Queue), 0600);
    struct Queue *buffer = (struct Queue*)shmat(buffid, NULL, 0);
    int semid = semget(MUTSYSKEY, 4, 0600);
    int emptyid = semget(EMPTYKEY, 4, 0600);
    int fullid = semget(FULLKEY, 4, 0600);
    int groupEmptyId = semget(GROUPEMPTYKEY, 1, 0600);
    int numeryKolejek[ILOSC_KOLEJEK];
    int putTo;
    ///
    printf("**************************Producent\n");
while(1){
    losujNumeryKolejek(numeryKolejek);
    downS(groupEmptyId, 0, 1); // czekanie az chociaz jedna kolejka bedzie NIEpelna
    for(int i=0; i<4; ++i){ //szukanie pierwszej nie pelnej kolejki
        if(semctl(emptyid, numeryKolejek[i], GETVAL) != 0){
            putTo = numeryKolejek[i];
            break;
        }
    }
    downS(emptyid, putTo, 1);
    downS(semid, putTo, 1);
        printf("\tPRODUCER wsadza do kolejki %d\n", putTo);
        putToBuf(&buffer[putTo], putTo);
    upS(semid, putTo, 1);
    upS(fullid, putTo, 1);
    sleep(sleepProducer);
}
}
void Consumer(int nr, int bufA, int bufB, int bufC, int bufD, int sleepConsumer){
    int buffid = shmget(BUFFKEY, 4*sizeof(struct Queue), 0600);
    struct Queue *buffer = (struct Queue*)shmat(buffid, NULL, 0);
    int semid = semget(MUTSYSKEY, 4, 0600);
    int emptyid = semget(EMPTYKEY, 4, 0600);
    int fullid = semget(FULLKEY, 4, 0600);
    int groupEmptyId = semget(GROUPEMPTYKEY, 1, 0600);
    ///
    printf("***********Consumer %d \tA: %d, B: %d, C: %d, D: %d\n", nr, bufA, bufB, bufC, bufD);
    while(1){
        downClient(fullid, bufA, bufB, bufC, bufD);
        downGroup(semid, bufA, bufB, bufC, bufD);
            printf("\tCONSUMER %d\n", nr);
            for(int i=0; i<bufA; ++i) getFromBuf(&buffer[0]); //kolejka A
            for(int i=0; i<bufB; ++i) getFromBuf(&buffer[1]); //kolejka B
            for(int i=0; i<bufC; ++i) getFromBuf(&buffer[2]); //kolejka C
            for(int i=0; i<bufD; ++i) getFromBuf(&buffer[3]); //kolejka D
        upGroup(semid, bufA, bufB, bufC, bufD);
        upClient(emptyid, bufA, bufB, bufC, bufD);
        upS(groupEmptyId, 0, bufA+bufB+bufC+bufD); // usunieto 1 elem z jakiejs kolejki = wzrosla liczba pustych elementow
        sleep(sleepConsumer);
    }
}
int main() {
    srand(time(NULL));
    int buffid = shmget(BUFFKEY, 4*sizeof(struct Queue), IPC_CREAT|0600);
    struct Queue *buffer = (struct Queue*)shmat(buffid, NULL, 0);

    int semid = semget(MUTSYSKEY, 4, IPC_CREAT|IPC_EXCL|0600);
    if(semid==-1){
        semid = semget(MUTSYSKEY, 4, 0600);
        if(semid==-1) perror("BLAD SEMID");
    }
    for(int i=0; i<4; ++i)
        semctl(semid, i, SETVAL, (int)1);

    int emptyid = semget(EMPTYKEY, 4, IPC_CREAT|IPC_EXCL|0600);
    if(emptyid==-1){
        emptyid = semget(EMPTYKEY, 4, 0600);
        if(emptyid==-1) perror("BLAD EMPTYID");
    }
    for(int i=0; i<4; ++i)
        semctl(emptyid, i, SETVAL, (int)BUFSIZE);

    int fullid = semget(FULLKEY, 4, IPC_CREAT|IPC_EXCL|0600);
    if(fullid==-1){
        fullid = semget(FULLKEY, 4, 0600);
        if(fullid==-1) perror("BLAD FULLID");
    }
    for(int i=0; i<4; ++i)
        semctl(fullid, i, SETVAL, (int)0);

    int groupEmptyId = semget(GROUPEMPTYKEY, 1, IPC_CREAT|IPC_EXCL|0600); //ile jest w ogolnosci we wszystkich kolejkach elem empty
    if(groupEmptyId==-1){
        groupEmptyId = semget(GROUPEMPTYKEY, 1, 0600);
        if(groupEmptyId==-1)    perror("BLAD GROUPEMPTYID");
    }
    semctl(groupEmptyId, 0, SETVAL, (int)(ILOSC_KOLEJEK*BUFSIZE));

    for(int i=0; i<4; i++){
        buffer[i].length=0;
        buffer[i].head=0;
        buffer[i].tail=0;
    }
//////////////
    int sleepConsumer, sleepProducer;
    printf("Podaj na ile sekund ma sie usypiac Producer (jesli podasz -1, zostanie wylosowane): ");
    scanf("%d", &sleepProducer);
    if(sleepProducer == -1){
        sleepProducer = rand()%10 +1;
        printf ("Wylosowano: %d\n", sleepProducer);
    }
    printf("Podaj na ile sekund ma sie usypiac Consumer (jesli podasz -1, zostanie wylosowane): ");
    scanf("%d", &sleepConsumer);
    if(sleepConsumer == -1){
        sleepConsumer = rand()%10 +1;
        printf("Wylosowano: %d\n", sleepConsumer);
    }

    printf("Podaj ilosc konsumentow: ");
    int iloscKonsumentow;
    scanf("%d", &iloscKonsumentow);
    int **tab;
    if(iloscKonsumentow != 0) {
        tab = malloc(iloscKonsumentow*sizeof(int *));
        for(int i=0; i<iloscKonsumentow; ++i)
            tab[i] = malloc(4*sizeof(int));


        for(int i=0; i<iloscKonsumentow; ++i){
            printf("Podaj dla konsumenta nr %d z ktorej kolejki ma zabierac po kolei A,B,C,D: ", i);
            scanf("%d%d%d%d", &tab[i][0], &tab[i][1], &tab[i][2], &tab[i][3]);
        }
    }
//////////////
    pid_t pid[iloscKonsumentow+3];
    pid[0]=fork();
    if(pid[0]==0){ Producent(sleepProducer); }
    pid[1]=fork();
    if(pid[1]==0) { Consumer(0, 1, 2, 3, 0, sleepConsumer); }
    pid[2]=fork();
    if(pid[2]==0) { Consumer(1, 3, 2, 1, 0, sleepConsumer); }

    for(int i=0; i<iloscKonsumentow; ++i){
        pid[i+3]=fork();
        if(pid[i+3]==0){ Consumer(i+2, tab[i][0], tab[i][1], tab[i][2], tab[i][3], sleepConsumer); }
    }

    sleep(2);
    for(int i=0; i<iloscKonsumentow+3; ++i)
        kill(pid[i], SIGKILL);
    if(iloscKonsumentow!=0){ //zwalania tablicy
        for(int i=0; i<iloscKonsumentow; ++i)
            free(tab[i]);

        free(tab);
    }
    shmdt(buffer);
}

