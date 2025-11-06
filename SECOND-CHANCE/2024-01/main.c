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
#include <ctype.h>
#include <pthread.h>

#include "debug.h"
#include "args.h"
#include "memory.h"


#define NUM_THREADS				        26



// declaração global  ** (não copiar este comentário) ** 
// Estrutura a 'passar' às threads
typedef struct 
{
    int id;
	char letra;
    char *palavra;
    int *indice;
	pthread_mutex_t *ptr_mutex;
}thread_params_t;

void *task(void *arg);

int main(int argc, char *argv[]) {

    struct gengetopt_args_info args;

	// gengetopt parser: deve ser a primeira linha de código no main
	if(cmdline_parser(argc, argv, &args))
		ERROR(1, "Erro: execução de cmdline_parser\n");


    char *palavra=args.word_arg;

    for(int i=0;i<strlen(palavra);i++){
        
        if(!isalpha(palavra[i]))ERROR(2,"Palavra invalida!");
    }

    int indice=0;
    // na função main/outra  ** (não copiar este comentário) ** 
	pthread_t tids[NUM_THREADS];
    thread_params_t thread_params[NUM_THREADS];
	pthread_mutex_t mutex;
    	// Mutex: inicializa o mutex antes de criar a(s) thread(s)	
	if ((errno = pthread_mutex_init(&mutex, NULL)) != 0)
		ERROR(12, "pthread_mutex_init() failed");


	// Inicialização das estruturas - para cada thread
	for (int i = 0; i < NUM_THREADS; i++){
		thread_params[i].letra = i + 'a';
		thread_params[i].id = i;
		thread_params[i].palavra =palavra;
		thread_params[i].indice =&indice;
	    thread_params[i].ptr_mutex = &mutex;

	}
	
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



	// Mutex: liberta recurso
	if ((errno = pthread_mutex_destroy(&mutex)) != 0)
		ERROR(13, "pthread_mutex_destroy() failed");


	// gengetopt: libertar recurso (assim que possível)
	cmdline_parser_free(&args);
    return 0;
}



// Zona das funções  ** (não copiar este comentário) ** 
// Thread
void *task(void *arg) {
    thread_params_t *params = (thread_params_t *)arg;
    int palavra_len = strlen(params->palavra);

    while (1) {
        if ((errno = pthread_mutex_lock(params->ptr_mutex)) != 0) {
            WARNING("pthread_mutex_lock() failed");
            return NULL;
        }

        // Verifica se terminou (dentro do mutex)
        if (*params->indice >= palavra_len) {
            pthread_mutex_unlock(params->ptr_mutex);
            break;
        }

        // Verifica se é a vez desta thread
        int letra_number = tolower(params->palavra[*params->indice]) - 'a';
        
        if (letra_number == params->id) {
            printf("%c [thread '%c'] (ID=%lu)\n",
                   params->palavra[*params->indice],
                   params->letra,
                   pthread_self());
            (*params->indice)++;
        }

        if ((errno = pthread_mutex_unlock(params->ptr_mutex)) != 0) {
            WARNING("pthread_mutex_unlock() failed");
            return NULL;
        }

    }   

    return NULL;
}