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
#include <time.h>
#include <pthread.h>
#include <ctype.h>
#include "args.h"

#include "debug.h"
#include "memory.h"

typedef struct 
{
    int id;
    int *array_random;
    pthread_mutex_t *ptr_mutex;
    int target;
    int bloco_por_thread;
    int *indices_vistos;
    int *found;
    int size_array;
} thread_params_t;
struct gengetopt_args_info args;
void *task(void *arg);

int main(int argc, char *argv[]) {
    unsigned int seed = time(NULL);  // semente baseada no tempo

    // gengetopt parser: deve ser a primeira linha de código no main
    if(cmdline_parser(argc, argv, &args))
        ERROR(1, "Erro: execução de cmdline_parser\n");
    
    int size_array=args.size_arg;
    int target_number=args.target_arg;
    int num_threads=args.numthreads_arg;
    int bloco_por_thread=size_array/num_threads;
    if(size_array<10){
        ERROR(1,"Size vetor menor que 10");
    }

    if (num_threads < 2 || (size_array % num_threads) != 0) {
        ERROR(1, "Valor de threads menor que 2 OU tamanho do array nao é multiplo do numero de threads!");
    }


    //CRIAÇÂO DO ARRAY RANDOM
   int *array_random=malloc(size_array*sizeof(int));
   if (!array_random) ERROR(2,"ERROR A GERRAR ARRAY RANDOM");


    for(int i=0;i<size_array;i++){
         array_random[i]= rand_r(&seed) % ((size_array*3) + 1);  // gera número entre 0 e max
    }

    //CRIACAO DE THREADS 
 	pthread_t *tids=malloc(sizeof(pthread_t)*num_threads);
	if (!tids) ERROR(2, "malloc failed for tids");

    thread_params_t *thread_params = malloc(sizeof(thread_params_t) * num_threads);
    if (!thread_params) ERROR(2, "malloc failed for thread_params");


     //VARIAVEIS E INICACOES
	pthread_mutex_t mutex;
	pthread_cond_t cond;
    int indices_vistos=0;
    int found=0;
	// Mutex: inicializa o mutex antes de criar a(s) thread(s)	
	if ((errno = pthread_mutex_init(&mutex, NULL)) != 0)
		ERROR(12, "pthread_mutex_init() failed");
	// Var.Condição: inicializa variável de condição
	if ((errno = pthread_cond_init(&cond, NULL)) != 0)
		ERROR(14, "pthread_cond_init() failed!");
    
    

	// Inicialização das estruturas - para cada thread
	for (int i = 0; i < num_threads; i++){
		thread_params[i].id = i;
		thread_params[i].array_random=array_random;
		thread_params[i].bloco_por_thread=bloco_por_thread;
		thread_params[i].target=target_number;
		thread_params[i].ptr_mutex=&mutex;
		thread_params[i].indices_vistos=&indices_vistos;
		thread_params[i].found=&found;
		thread_params[i].size_array=size_array;

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
    
    if (found == 0)
        printf("Target %d not found\n", target_number);

    	
	// Var.Condição: destroi a variável de condição 
	if ((errno = pthread_cond_destroy(&cond)) != 0)
		ERROR(15,"pthread_cond_destroy failed!");



	// Mutex: liberta recurso
	if ((errno = pthread_mutex_destroy(&mutex)) != 0)
		ERROR(13, "pthread_mutex_destroy() failed");

    
    free(array_random);
    cmdline_parser_free(&args);


    return 0;
}

// Zona das funções  ** (não copiar este comentário) ** 
// Thread
void *task(void *arg) {
    thread_params_t *params = (thread_params_t *) arg;
    int inicio = params->id * params->bloco_por_thread;
    int fim = inicio + params->bloco_por_thread;
    if (fim > params->size_array) fim = params->size_array;

    for (int i = inicio; i < fim; i++) {
        pthread_mutex_lock(params->ptr_mutex);
        if (*params->found) {
            pthread_mutex_unlock(params->ptr_mutex);
            return NULL;
        }
        pthread_mutex_unlock(params->ptr_mutex);

        if (params->array_random[i] == params->target) {
            pthread_mutex_lock(params->ptr_mutex);
            if (!*params->found) {
                *params->found = 1;
                printf("Thread %d found target '%d' at index %d\n",
                       params->id, params->target, i);
            }
            pthread_mutex_unlock(params->ptr_mutex);
            return NULL;
        }
    }

    return NULL;
}
