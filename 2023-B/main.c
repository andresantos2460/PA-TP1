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
#define NUM_REGIOES 2

typedef struct 
{
    int id;
    int num_candidatos;
    int *candidatos;
    int regiao;
    pthread_mutex_t *ptr_mutex;
   
} thread_params_t;
struct gengetopt_args_info args;
void *task(void *arg);

int main(int argc, char *argv[]) {
    // gengetopt parser: deve ser a primeira linha de código no main
    if(cmdline_parser(argc, argv, &args))
        ERROR(1, "Erro: execução de cmdline_parser\n");
    
        int candidatos=args.candidatos_arg;
        if(candidatos<1)ERROR(1,"Número de candidatos invalido");

    pid_t pid;

  


    // processo pai cria N processos
    for(int i = 0; i < NUM_REGIOES; i++){
        pid = fork();
        if (pid == 0) {		

            
        unsigned int rand_state = time(NULL) ^ getpid();
        int eleitores = rand_r(&rand_state)%5+1;
        int regiao=i+1;
        int total_writes=0;

        int *array_candidatos=malloc(sizeof(int)*candidatos);
        if(!array_candidatos)ERROR(2,"ERRO a criar malloc");
        memset(array_candidatos, 0, sizeof(int)*candidatos); // ← ADICIONAR ISTO

        pthread_t *tids=malloc(sizeof(pthread_t)*eleitores);
        if (!tids) ERROR(2, "malloc failed for tids");

        thread_params_t *thread_params = malloc(sizeof(thread_params_t) * eleitores);
            if (!thread_params) ERROR(2, "malloc failed for thread_params");
        pthread_mutex_t mutex;
        // Mutex: inicializa o mutex antes de criar a(s) thread(s)	
        if ((errno = pthread_mutex_init(&mutex, NULL)) != 0)
            ERROR(12, "pthread_mutex_init() failed");


        // Inicialização das estruturas - para cada thread
        for (int i = 0; i < eleitores; i++){
            thread_params[i].id = i + 1;
            thread_params[i].ptr_mutex = &mutex;
            thread_params[i].candidatos = array_candidatos;
            thread_params[i].num_candidatos = candidatos;
            thread_params[i].regiao = regiao;

        }
        
        // Criação das threads + passagem de parâmetro
        for (int i = 0; i < eleitores; i++){
            if ((errno = pthread_create(&tids[i], NULL, task, &thread_params[i])) != 0)
                ERROR(10, "Erro no pthread_create()!");	
        }


        // Espera que todas as threads terminem
        for (int i = 0; i < eleitores; i++){
            if ((errno = pthread_join(tids[i], NULL)) != 0)
                ERROR(11, "Erro no pthread_join()!\n");
        }
        printf("Regiao %d\n",regiao);
        for (int i = 0; i < candidatos; i++)
        {
            printf("[%d] :  %d\n",i,array_candidatos[i]);
        }

        free(tids);
        free(thread_params);
        free(array_candidatos);


	    // Mutex: liberta recurso
	    if ((errno = pthread_mutex_destroy(&mutex)) != 0)
		    ERROR(13, "pthread_mutex_destroy() failed");
		

         
            exit(0);			
        } else if (pid > 0) {	// Processo pai
            // usar preferencialmente a zona a seguir ao for
        } else					// < 0 - erro
            ERROR(2, "Erro na execucao do fork()");
    }
    
    // Apenas processo pai
    
    // Espera pelos processos filhos
    for(int i = 0; i < NUM_REGIOES; i++){
        wait(NULL);
    }






    cmdline_parser_free(&args);


    return 0;
}

// Zona das funções  ** (não copiar este comentário) ** 
// Thread
void *task(void *arg) {
    thread_params_t *params = (thread_params_t *) arg;
    unsigned int rand_state = time(NULL) ^ getpid() ^ (unsigned long) pthread_self();
    int random_candidato = rand_r(&rand_state)%params->num_candidatos; // gera um valor aleatório

    	// Mutex: entra na secção crítica 
	if ((errno = pthread_mutex_lock(params->ptr_mutex)) != 0){		
		WARNING("pthread_mutex_lock() failed");
		return NULL;
	}
        params->candidatos[random_candidato]+=1;
	// Mutex: sai da secção crítica 
	if ((errno = pthread_mutex_unlock(params->ptr_mutex)) != 0){
		WARNING("pthread_mutex_unlock() failed");
		return NULL;
	}
    return NULL;
}
