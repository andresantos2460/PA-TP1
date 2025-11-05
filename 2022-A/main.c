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
#define L 5
#define C 4

// declaração global  ** (não copiar este comentário) ** 
// Estrutura a 'passar' às threads
typedef struct 
{
	int id;
    pthread_mutex_t (*ptr_mutex)[L][C];
    pthread_mutex_t *ptr_mutex_main;
    int (*matriz)[L][C];
    int repeticoes;
    int *total;

}thread_params_t;

void *task(void *arg);

int main(int argc, char *argv[]) {
   	struct gengetopt_args_info args;

	// gengetopt parser: deve ser a primeira linha de código no main
	if(cmdline_parser(argc, argv, &args))
		ERROR(1, "Erro: execução de cmdline_parser\n");

        int num_threads=args.number_threads_arg;
        int num_writes=args.number_writes_arg;


	    pid_t pid;

	    pid = fork();

        if (pid == 0) {			// Processo filho 

        int matriz[L][C]={0};
        int total=0;

        //MUTEX
        pthread_mutex_t mutex[L][C];
        pthread_mutex_t mutex_main;

        // Mutex: inicializa o mutex antes de criar a(s) thread(s)	
	    if ((errno = pthread_mutex_init(&mutex_main, NULL)) != 0)
		    ERROR(12, "pthread_mutex_init() failed");


        for (int i = 0; i < L; i++)
        {
            for(int j=0;j<C;j++){

                // Mutex: inicializa o mutex antes de criar a(s) thread(s)	
                if ((errno = pthread_mutex_init(&mutex[i][j], NULL)) != 0)
                    ERROR(12, "pthread_mutex_init() failed");

                }
            /* code */
        }
        
       //-------------fim inicio mutex----------------------------//
        
        pthread_t *tids=malloc(sizeof(pthread_t)*num_threads);
        if (!tids) ERROR(2, "malloc failed for tids");

        thread_params_t *thread_params = malloc(sizeof(thread_params_t) * num_threads);
        if (!thread_params) ERROR(2, "malloc failed for thread_params");

                    // Inicialização das estruturas - para cada thread
        for (int i = 0; i < num_threads; i++){
            thread_params[i].id = i + 1;
            thread_params[i].ptr_mutex =&mutex;
            thread_params[i].matriz =&matriz;
            thread_params[i].repeticoes =num_writes;
            thread_params[i].total =&total;
            thread_params[i].ptr_mutex_main =&mutex_main;

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

        //--------------fim mutex ---------------------//
           
        for (int i = 0; i < L; i++)
        {
            for(int j=0;j<C;j++){

                // Mutex: inicializa o mutex antes de criar a(s) thread(s)	
                if ((errno = pthread_mutex_destroy(&mutex[i][j])) != 0)
                    ERROR(12, "pthread_mutex_init() failed");

                }
            /* code */
        }
        // Mutex: liberta recurso
	    if ((errno = pthread_mutex_destroy(&mutex_main)) != 0)
		ERROR(13, "pthread_mutex_destroy() failed");
		

        printf("Number of writes for each element: \n");


          for (int i = 0; i < L; i++)
        {
            for(int j=0;j<C;j++){

                printf("[%d,%d]: %d \n",i,j,matriz[i][j]);

        }
        }
        
        printf("Total number of writes: %d \n",total);
        


            exit(0);			// Terminar processo filho
        } else if (pid > 0) {	// Processo pai 
            // usar preferencialmente a zona a seguir ao if
        } else					// < 0 - erro
            ERROR(2, "Erro na execucao do fork()");

        // Apenas processo pai
        // processo_pai();
        
        // pai espera pelo filho
        wait(NULL);
      
        printf("#END# \n");


    /* Main code */
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
    unsigned int rand_state = time(NULL) ^ getpid() ^ (long)pthread_self();

    for(int i=0;i<params->repeticoes;i++){

        int random_linha = rand_r(&rand_state)%L; // gera um valor aleatório
        int random_coluna = rand_r(&rand_state)%C; // gera outro valor aleatório
        int random_input = (rand_r(&rand_state)%1000)+1; // gera outro valor aleatório
        // Mutex: entra na secção crítica 
        if ((errno = pthread_mutex_lock(&(*params->ptr_mutex)[random_linha][random_coluna])) != 0){		
            WARNING("pthread_mutex_lock() failed");
            return NULL;
        }
        //  #######   Secção crítica   ######
       (*params->matriz)[random_linha][random_coluna]=random_input;

   
        // Mutex: sai da secção crítica 
        if ((errno = pthread_mutex_unlock(&(*params->ptr_mutex)[random_linha][random_coluna])) != 0){
            WARNING("pthread_mutex_unlock() failed");
            return NULL;
        }
         // Mutex: entra na secção crítica 
        if ((errno = pthread_mutex_lock(&(*params->ptr_mutex_main))) != 0){		
            WARNING("pthread_mutex_lock() failed");
            return NULL;
        }
        (*params->total)++;
        
          // Mutex: sai da secção crítica 
        if ((errno = pthread_mutex_unlock(&(*params->ptr_mutex_main))) != 0){
            WARNING("pthread_mutex_unlock() failed");
            return NULL;
        }


    }
	
	return NULL;
}
