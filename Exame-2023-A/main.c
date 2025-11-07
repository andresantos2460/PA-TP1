/**
 * @file main.c
 * @brief Description
 * @date 2018-1-1
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


// declaração global  ** (não copiar este comentário) ** 
// Estrutura a 'passar' às threads
typedef struct 
{
	int id;
	int data_size;
	int *indice;
    int *ptr_data;
    pthread_mutex_t *ptr_mutex;
}thread_params_t;
void *task(void *arg);
void trata_sinal(int signal);
int flag = 0;  // só para comunicação com o handler

int main(int argc, char *argv[]) {

	struct gengetopt_args_info args;
	
	// gengetopt parser: deve ser a primeira linha de código no main
	if(cmdline_parser(argc, argv, &args))
		ERROR(1, "Erro: execução de cmdline_parser\n");

        int num_threads=args.thread_count_arg;
        int data_size=args.data_size_arg;
        int indice=0;
        int *array_data_size=malloc(sizeof(int)*data_size);
        if(!array_data_size)ERROR(1,"Erro a criar vetor");
        memset(array_data_size,0,sizeof(int)*data_size);

        for(int i=0; i<data_size;i++){

            array_data_size[i]=i;
            
        }

        pthread_t *tids=malloc(sizeof(pthread_t)*num_threads);
        if (!tids) ERROR(2, "malloc failed for tids");

        thread_params_t *thread_params = malloc(sizeof(thread_params_t) * num_threads);
        if (!thread_params) ERROR(2, "malloc failed for thread_params");

	    pthread_mutex_t mutex;

        // Mutex: inicializa o mutex antes de criar a(s) thread(s)	
	    if ((errno = pthread_mutex_init(&mutex, NULL)) != 0)
		ERROR(12, "pthread_mutex_init() failed");

        struct sigaction act;
        
        act.sa_handler = trata_sinal;    // Define o handler
        sigemptyset(&act.sa_mask);       // Não bloqueia outros sinais
        act.sa_flags = 0;                // Sem flags especiais
        // act.sa_flags |= SA_RESTART;   // Descomentaria se quisesse restart automático
        
        // Captura SIGINT (Ctrl+C)
        if(sigaction(SIGINT, &act, NULL) < 0)
            ERROR(3, "sigaction (sa_handler) - SIGINT");

		

        // Inicialização das estruturas - para cada thread
        for (int i = 0; i < num_threads; i++){
            thread_params[i].id = i + 1;
            thread_params[i].ptr_data = array_data_size;
            thread_params[i].data_size = data_size;
            thread_params[i].ptr_mutex = &mutex;
            thread_params[i].indice = &indice;

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
        printf("process finished...\n");

// Captura do sinal ??? 
	  

        for(int i=0;i<data_size;i++){
            printf("%d: %d\n",i,array_data_size[i]);
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
void *task(void *arg) 
{
	// cast para o tipo de dados enviado pela 'main thread'
	thread_params_t *params = (thread_params_t *) arg;
	
    unsigned int rand_state = time(NULL) ^ getpid() ^ (long)pthread_self();
    int last_indice=0;
    
  
   while (1){

            if (flag == 1) {
            // Entra no mutex para ler valores consistentes
            if ((errno = pthread_mutex_lock(params->ptr_mutex)) != 0) {
                WARNING("pthread_mutex_lock() failed");
                return NULL;
            }
            
            int processados = *params->indice;
            int total = params->data_size;
            float percentagem = (processados * 100.0) / total;
            
            printf("\n[SIGINT] Progresso: %d/%d valores processados (%.2f%%)\n",
                   processados, total, percentagem);
            
            flag = 0;  // Reset da flag
            
            if ((errno = pthread_mutex_unlock(params->ptr_mutex)) != 0) {
                WARNING("pthread_mutex_unlock() failed");
                return NULL;
            }
        }


        // Sleep ANTES de pegar trabalho (múltiplo de 100ms)
    int random_ms = ((rand_r(&rand_state) % 10) + 1) * 100; // 100-1000ms
    usleep(random_ms * 1000); // converte para microsegundos
    
   

	// Mutex: entra na secção crítica 
	if ((errno = pthread_mutex_lock(params->ptr_mutex)) != 0){		
		WARNING("pthread_mutex_lock() failed");
		return NULL;
	}
    if(*params->indice==0){
        printf("processing...\n");

    }
	
	if(*params->indice>(params->data_size-1)){
        if ((errno = pthread_mutex_unlock(params->ptr_mutex)) != 0){
            WARNING("pthread_mutex_unlock() failed");
            return NULL;
        }
        break;
    }


    last_indice=*params->indice;
    (*params->indice)++;
	
	// Mutex: sai da secção crítica 
	if ((errno = pthread_mutex_unlock(params->ptr_mutex)) != 0){
		WARNING("pthread_mutex_unlock() failed");
		return NULL;
	}

    params->ptr_data[last_indice]= (last_indice* 234323) % 345;

   }
   



	return NULL;
}
// zona das funções
void trata_sinal(int signal)
{
    if (signal == SIGINT) {
        flag = 1;
    }
}

