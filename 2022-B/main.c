/**
 * @file main.c
 * @brief Description
 * @date 2015-10-24
 * @author name of author
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <pthread.h>

#include "debug.h"
#include "memory.h"
#include "args.h"

#define SIZE_ARRAY 16
// declaração global  ** (não copiar este comentário) ** 
// Estrutura a 'passar' às threads
typedef struct 
{
	int id;
	int num_reservations;
    int *seats;
    pthread_mutex_t *ptr_mutex;
}thread_params_t;

void *task(void *arg);

int main(int argc, char *argv[]) {
    
	struct gengetopt_args_info args;

	// gengetopt parser: deve ser a primeira linha de código no main
	if(cmdline_parser(argc, argv, &args))
		ERROR(1, "Erro: execução de cmdline_parser\n");


        int num_threads=args.number_threads_arg;
        int num_reservations=args.num_of_reservations_arg;


        if(num_threads<1||num_reservations<1)ERROR(1,"Valores inputed invalidos!");


        int *seats=malloc(sizeof(int)*SIZE_ARRAY);
        if(!seats)ERROR(2,"ERROR CREATE MALLOC!");
        memset(seats,0,sizeof(int)*SIZE_ARRAY);

        pthread_t *tids=malloc(sizeof(pthread_t)*num_threads);
        if (!tids) ERROR(2, "malloc failed for tids");

        thread_params_t *thread_params = malloc(sizeof(thread_params_t) * num_threads);
        if (!thread_params) ERROR(2, "malloc failed for thread_params");

        pthread_mutex_t *mutex = malloc(sizeof(pthread_mutex_t) * SIZE_ARRAY);
        if (!mutex) ERROR(2, "malloc failed for mutex");

        for(int i=0;i<SIZE_ARRAY;i++){

            // Mutex: inicializa o mutex antes de criar a(s) thread(s)	
            if ((errno = pthread_mutex_init(&mutex[i], NULL)) != 0)
                ERROR(12, "pthread_mutex_init() failed");
        }
	
	

        // Inicialização das estruturas - para cada thread
        for (int i = 0; i < num_threads; i++){
            thread_params[i].id = i + 1;
            thread_params[i].seats = seats;
            thread_params[i].ptr_mutex=mutex;
            thread_params[i].num_reservations=num_reservations;

        }
        
        // Criação das threads + passagem de parâmetro
        for (int i = 0; i < num_threads; i++){
            if ((errno = pthread_create(&tids[i], NULL, task, &thread_params[i])) != 0)
                ERROR(10, "Erro no pthread_create()!");	
        }


        // Espera que todas as threads terminem
        for (int i = 0; i < num_threads; i++){
            if ((errno = pthread_join(tids[i], NULL)) != 0)
                ERROR(11, "Erro no pthread_join()!\n");
        }


        for (int i = 0; i < SIZE_ARRAY; i++)
        {
            if(seats[i]==0){
             printf("Seats %d : Not Reserved \n",i+1);
            }else{
             printf("Seats %d : thread %d \n",i+1,seats[i]);
            }

        }
        


        for(int i=0;i<SIZE_ARRAY;i++){

            // Mutex: liberta recurso
            if ((errno = pthread_mutex_destroy(&mutex[i])) != 0)
                ERROR(13, "pthread_mutex_destroy() failed");
		

        }

        free(tids);
        free(thread_params);
        free(seats);
        free(mutex);




	// gengetopt: libertar recurso (assim que possível)
	cmdline_parser_free(&args);

    /* Main code */

    return 0;
}

// Zona das funções  ** (não copiar este comentário) ** 
// Thread
void *task(void *arg) 
{
	// cast para o tipo de dados enviado pela 'main thread'
	thread_params_t *params = (thread_params_t *) arg;
	
	for (int i = 0; i < params->num_reservations; i++){

        unsigned int rand_state = time(NULL) ^ getpid() ^ (long)pthread_self();
        int random_seat = rand_r(&rand_state)%16;


        // Mutex: entra na secção crítica 
        if ((errno = pthread_mutex_lock(&params->ptr_mutex[random_seat])) != 0){		
            WARNING("pthread_mutex_lock() failed");
            return NULL;
        }
        
       if(params->seats[random_seat]==0){

            params->seats[random_seat]=params->id;
       }
        
        // Mutex: sai da secção crítica 
        if ((errno = pthread_mutex_unlock(&params->ptr_mutex[random_seat])) != 0){
            WARNING("pthread_mutex_unlock() failed");
            return NULL;
        }


    }
    
	return NULL;
}
