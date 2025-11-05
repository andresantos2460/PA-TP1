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
#include <ctype.h>
#include "args.h"

#include "debug.h"
#include "memory.h"
int NUM_THREADS=26;
// declaração global  ** (não copiar este comentário) ** 
// Estrutura a 'passar' às threads
typedef struct 
{
    int id;
    int *indice_partilhado;  // ponteiro para variável partilhada
    int fim;
    char *palavra;
    pthread_mutex_t *ptr_mutex;
    pthread_cond_t *ptr_cond;
} thread_params_t;
struct gengetopt_args_info args;
void *task(void *arg);

int main(int argc, char *argv[]) {

   	pthread_t tids[26];
    thread_params_t thread_params[26];

    // gengetopt parser: deve ser a primeira linha de código no main
    if(cmdline_parser(argc, argv, &args))
        ERROR(1, "Erro: execução de cmdline_parser\n");
    char *palavra = args.palavra_arg;
    for (int i = 0; i < strlen(palavra); i++) {
        if (!isalpha((unsigned char)palavra[i])) {
            ERROR(1, "PALAVRA not ASCII");
        }
    }
   

    //VARIAVEIS E INICACOES
	pthread_mutex_t mutex;
	pthread_cond_t cond;
    int indice_atual = 0;
	// Mutex: inicializa o mutex antes de criar a(s) thread(s)	
	if ((errno = pthread_mutex_init(&mutex, NULL)) != 0)
		ERROR(12, "pthread_mutex_init() failed");
	// Var.Condição: inicializa variável de condição
	if ((errno = pthread_cond_init(&cond, NULL)) != 0)
		ERROR(14, "pthread_cond_init() failed!");
    
    

	// Inicialização das estruturas - para cada thread
	for (int i = 0; i < NUM_THREADS; i++){
		thread_params[i].id = i;
		thread_params[i].ptr_mutex = &mutex;
		thread_params[i].ptr_cond = &cond;
		thread_params[i].indice_partilhado=&indice_atual;
		thread_params[i].palavra=palavra;
		thread_params[i].fim=strlen(palavra);

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

    	
	// Var.Condição: destroi a variável de condição 
	if ((errno = pthread_cond_destroy(&cond)) != 0)
		ERROR(15,"pthread_cond_destroy failed!");



	// Mutex: liberta recurso
	if ((errno = pthread_mutex_destroy(&mutex)) != 0)
		ERROR(13, "pthread_mutex_destroy() failed");
 

    cmdline_parser_free(&args);


    return 0;
}

// Zona das funções  ** (não copiar este comentário) ** 
// Thread

    
void *task(void *arg) 
{
    thread_params_t *params = (thread_params_t *) arg;
    pthread_mutex_t *mutex = params->ptr_mutex;
    pthread_cond_t *cond = params->ptr_cond;
    
    while (1) {
        if ((errno = pthread_mutex_lock(mutex)) != 0) {		
            WARNING("pthread_mutex_lock() failed");
            return NULL;
        }

        // Verifica se já chegou ao fim
        if (*(params->indice_partilhado) >= params->fim) {
            pthread_mutex_unlock(mutex);
            break;
        }

        // Calcula a thread que precisa processar a letra atual
        char letra_atual = tolower(params->palavra[*(params->indice_partilhado)]);
        int thread_preciso = letra_atual - 'a';
        
           // Espera até ser a vez desta thread
        while (params->id != thread_preciso) {
            if ((errno = pthread_cond_wait(cond, mutex)) != 0) {   
                WARNING("pthread_cond_wait() failed");
                return NULL;
            }
            if (*(params->indice_partilhado) >= params->fim) {
                pthread_mutex_unlock(mutex);
                return NULL;
            }

            letra_atual = tolower(params->palavra[*(params->indice_partilhado)]);
            thread_preciso = letra_atual - 'a';
        }

        // Agora é a vez desta thread - IMPRIME ANTES de incrementar
       printf("%c [Thread %c] (ID=%lu) ID_LETRA=%d\n", letra_atual, letra_atual, pthread_self(), params->id);

        // Avança para a próxima letra
        (*(params->indice_partilhado))++;

        // Acorda todas as threads
        if ((errno = pthread_cond_broadcast(cond)) != 0) {
            WARNING("pthread_cond_broadcast() failed");
            pthread_mutex_unlock(mutex);
            return NULL;
        }

        if ((errno = pthread_mutex_unlock(mutex)) != 0) {
            WARNING("pthread_mutex_unlock() failed");
            return NULL;
        }
    }

    return NULL;
}
