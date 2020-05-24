/*
 un processo apre un file in scrittura (se esiste già sovrascrive i contenuti del file), poi lancia M (=10) threads.

"fase 1" vuol dire: dormire per un intervallo random di tempo compreso tra 0 e 3 secondi, poi scrivere nel file il messaggio:
"fase 1, thread id=, sleep period= secondi"
"fase 2" vuol dire: scrivere nel file il messaggio "fase 2, thread id=, dopo la barriera" poi dormire per 10 millisecondi,
scrivere nel file il messggio "thread id= bye!".

per ogni thread: effettuare "fase 1", poi aspettare che tutti i thread abbiano completato la fase 1 (barriera: little book of
semaphores, pag. 29); poi effettuare "fase 2" e terminare il thread.
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>

#include <errno.h>
#include <pthread.h>
#include <semaphore.h>


#define NUMBER_OF_THREADS 10

sem_t semaphore;
pthread_barrier_t thread_barrier;
int fd;

#define CHECK_ERR(a,msg) {if ((a) == -1) { perror((msg)); exit(EXIT_FAILURE); } }


void * thread_function(void * arg) {
	pthread_t tid = pthread_self();
	srand(time(NULL));
	int time = rand() % 4;

	sleep(time);

	if (sem_wait(&semaphore) == -1) {
		perror("sem_wait");
		exit(EXIT_FAILURE);
	}

	int written = dprintf(fd, "fase 1, thread id = %lu, sleep period = %d\n", tid, time);
	CHECK_ERR(written, "dprintf() error")

	if (sem_post(&semaphore) == -1) {
		perror("sem_post");
		exit(EXIT_FAILURE);
	}

	int s = pthread_barrier_wait(&thread_barrier);
	if (!(s == PTHREAD_BARRIER_SERIAL_THREAD || s == 0)){
		perror("pthread barrier error");
		exit(EXIT_FAILURE);
	}

	if (sem_wait(&semaphore) == -1) {
		perror("sem_wait");
		exit(EXIT_FAILURE);
	}

	written = dprintf(fd, "fase 2, thread id = %lu, dopo la barriera\n", tid);
	CHECK_ERR(written, "dprintf() error")

	if (sem_post(&semaphore) == -1) {
		perror("sem_post");
		exit(EXIT_FAILURE);
	}

	usleep(10000);

	if (sem_wait(&semaphore) == -1) {
		perror("sem_wait");
		exit(EXIT_FAILURE);
	}

	written = dprintf(fd, "thread id = %lu, bye!\n", tid);
	CHECK_ERR(written, "dprintf() error")

	if (sem_post(&semaphore) == -1) {
		perror("sem_post");
		exit(EXIT_FAILURE);
	}

	return NULL;
}

int main(int argc, char *argv[]) {
	char * file_name = "/home/utente/prova.txt";
	pthread_t t[NUMBER_OF_THREADS];
	int res;

	fd = open(file_name, O_RDWR | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR);
	CHECK_ERR(res,"open()")

	res = sem_init(&semaphore, 0, 1);
	CHECK_ERR(res,"sem_init")

	res = pthread_barrier_init(&thread_barrier, NULL, NUMBER_OF_THREADS);
	CHECK_ERR(res,"pthread_create()")

	for(int i=0; i<NUMBER_OF_THREADS; i++){
		res = pthread_create(&t[i], NULL, thread_function, NULL);
		CHECK_ERR(res,"pthread_create()")
	}

	for(int i=0; i<NUMBER_OF_THREADS; i++){
		res = pthread_join(&t[i], NULL);
		CHECK_ERR(res,"pthread_join()")
	}

	res = sem_destroy(&semaphore);
	CHECK_ERR(res,"sem_destroy")

	res = pthread_barrier_destroy(&thread_barrier);
	CHECK_ERR(res,"pthread_barrier_destroy() error")

	res = close(fd);
	CHECK_ERR(res,"close()")

	return EXIT_SUCCESS;
}
