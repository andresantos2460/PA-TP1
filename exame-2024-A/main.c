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
#include "args.h"
#include "memory.h"

#define NUM_THREADS				        4

//  Protóptipos

// declaração global  ** (não copiar este comentário) ** 
// Estrutura a 'passar' às threads
typedef struct 
{
	int id;
	int tipo;
    int work_size;
    int *array_works;
    int *verificadas;
    pthread_mutex_t *ptr_mutex;
}thread_params_t;

void *task(void *arg);

int main(int argc, char *argv[]) {
  
	struct gengetopt_args_info args;
    unsigned int r_state = time(NULL) ^ getpid();

	// gengetopt parser: deve ser a primeira linha de código no main
	if(cmdline_parser(argc, argv, &args))
		ERROR(1, "Erro: execução de cmdline_parser\n");

    int work_size=args.work_size_arg;
    if(work_size<1)ERROR(1,"numero tem de ser > 0");

    int *array_works=malloc(sizeof(int)*work_size);
    if(!array_works)ERROR(2,"ERROR CREATING MALLOC!");
    
    printf("Work units:\n");

    for(int i=0;i<work_size;i++){

        int random = (rand_r(&r_state)%2)+1;
        array_works[i]=random;

        printf("%d - Type: %d\n",i+1,array_works[i]);

    }
   
    int *verificadas=malloc(sizeof(int)*work_size);
    if(!verificadas)ERROR(2,"ERROR CREATING MALLOC!");
    memset(verificadas,0,sizeof(int)*work_size);




// na função main/outra  ** (não copiar este comentário) ** 
	pthread_t tids[NUM_THREADS];
    thread_params_t thread_params[NUM_THREADS];
	pthread_mutex_t *mutex=malloc(sizeof(pthread_mutex_t)*work_size);
    if(!mutex)ERROR(2,"ERROR CREATING MALLOC!");


    for(int i=0;i<work_size;i++){

        // Mutex: inicializa o mutex antes de criar a(s) thread(s)	
        if ((errno = pthread_mutex_init(&mutex[i], NULL)) != 0)
            ERROR(12, "pthread_mutex_init() failed");
    }

    
	// Inicialização das estruturas - para cada thread
	for (int i = 0; i < NUM_THREADS; i++){
		thread_params[i].id = i;
		thread_params[i].tipo = (i%2)+1;
		thread_params[i].array_works = array_works;
		thread_params[i].ptr_mutex =mutex;
		thread_params[i].work_size =work_size;
		thread_params[i].verificadas =verificadas;

 
	}
	printf("Starting work!:\n");
	// Criação das threads + passagem de parâmetro
	for (int i = 0; i < NUM_THREADS; i++){
		if ((errno = pthread_create(&tids[i], NULL, task, &thread_params[i])) != 0)
			ERROR(10, "Erro no pthread_create()!");	
	}


    // Espera que todas as threads terminem
	for (int i = 0; i < NUM_THREADS; i++){
		if ((errno = pthread_join(tids[i], NULL)) != 0)
			ERROR(11, "Erro no pthread_join()!\n");
	}



    for(int i=0;i<work_size;i++){

        // Mutex: liberta recurso
        if ((errno = pthread_mutex_destroy(&mutex[i])) != 0)
            ERROR(13, "pthread_mutex_destroy() failed");
    

    }

    free(verificadas);  // ← FALTA!
    free(mutex);        // ← FALTA!
    free(array_works);
	// gengetopt: libertar recurso (assim que possível)
	cmdline_parser_free(&args);

	

    return 0;
}
// Zona das funções  ** (não copiar este comentário) ** 
// Thread
void *task(void *arg) 
{
	// cast para o tipo de dados enviado pela 'main thread'
	thread_params_t *params = (thread_params_t *) arg;
    printf("worker thread #%d of type %d is starting. \n",params->id,params->tipo);

        /* code */
        for (int i = 0; i < params->work_size; i++){

            if(params->tipo==params->array_works[i]){
                        // Mutex: entra na secção crítica 
            if ((errno = pthread_mutex_lock(&params->ptr_mutex[i])) != 0){		
                WARNING("pthread_mutex_lock() failed");
                return NULL;
            }
                if(params->verificadas[i]!=0){
                    if ((errno = pthread_mutex_unlock(&params->ptr_mutex[i])) != 0){
                    WARNING("pthread_mutex_unlock() failed");
                    return NULL;
                    }
                    continue;
                }
                params->verificadas[i]=1;
                sleep(1);
                printf("Worker of type %d has finished work item number %d. \n",params->tipo,i+1);


                   // Mutex: sai da secção crítica 
            if ((errno = pthread_mutex_unlock(&params->ptr_mutex[i])) != 0){
                WARNING("pthread_mutex_unlock() failed");
                return NULL;
            }
         
        
        }


    }
    printf("worker thread #%d of type %d is done.\n", params->id, params->tipo);
    


	return NULL;
}
