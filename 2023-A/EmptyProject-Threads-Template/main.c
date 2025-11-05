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
#include "args.h"
#include "debug.h"
#include "memory.h"


typedef struct 
{
    int id;
    int *array1;
    int *array2;
    int length;
    int *count;
    char *sort;
	pthread_mutex_t *ptr_mutex;
} thread_params_t;

void trata_sinal(int signal);
void *task(void *arg);

volatile sig_atomic_t flag = 0;  // só para comunicação com o handler
int main(int argc, char *argv[]) {
	struct gengetopt_args_info args;
    unsigned int rand_state = time(NULL) ^ getpid();

	struct sigaction act;
    // configura handler
    act.sa_handler = trata_sinal;     // função que será chamada
    sigemptyset(&act.sa_mask);        // não bloqueia outros sinais
    act.sa_flags = 0;                 // flags padrão

	// gengetopt parser: deve ser a primeira linha de código no main
	if(cmdline_parser(argc, argv, &args))
		ERROR(1, "Erro: execução de cmdline_parser\n");
   //variaveis
    char *sort=args.sort_arg;
    int length=args.length_arg;
  //validacao
    if(length<0)ERROR(1,"valor de length invalido!");
    
    if(strcmp(sort,"asc")!=0&&strcmp(sort,"desc")!=0)ERROR(1,"ERROR OPCAO DE SORT ERRADA");

    //ARRAYS E PREENCHER
    int *array1=malloc(sizeof(int)*length);
    if(!array1)ERROR(2,"ERROR a criar ARRAY 1");

    int *array2=malloc(sizeof(int)*length);
    if(!array2)ERROR(2,"ERROR a criar ARRAY 2");
    
    for(int i=0;i<length;i++){
        int value1=rand_r(&rand_state)%100;
        int value2=rand_r(&rand_state)%100;
        array1[i]=value1;
        array2[i]=value2;
    }
  
    //print primeiro Vetor
    printf("First Vector:["); 

    for(int i=0;i<length;i++){
       printf(" %d ",array1[i]);
    }
    printf("]\n"); 

    //print segundo vetor
    printf("Second Vector:["); 

    for(int i=0;i<length;i++){
       printf(" %d ",array2[i]);
    }
    printf("]\n"); 

    // Captura o SIGINT (Ctrl+C)
    if (sigaction(SIGINT, &act, NULL) < 0) {
        perror("sigaction(SIGINT)");
        exit(1);
    }
    //SINALLL
    printf("[PID:%d] A esperar pelo sinal SIGINT...\n", getpid());

    while (flag == 0) {
        sleep(1);
    }
    printf("Executing SWAP...\n");
    printf("After Swap \n");

    //AFTER SINAL!


    pthread_t tids[2];
    thread_params_t thread_params[2];
	pthread_mutex_t mutex;
    // Mutex: inicializa o mutex antes de criar a(s) thread(s)	
	if ((errno = pthread_mutex_init(&mutex, NULL)) != 0)
		ERROR(12, "pthread_mutex_init() failed");

    int count=0;
	// Inicialização das estruturas - para cada thread
	for (int i = 0; i < 2; i++){
		thread_params[i].id = i+1;
		thread_params[i].array1=array1;
		thread_params[i].array2=array2;
		thread_params[i].length =length;
		thread_params[i].sort = sort;
		thread_params[i].count = &count;
		thread_params[i].ptr_mutex = &mutex;

	}
	
	// Criação das threads + passagem de parâmetro
	for (int i = 0; i < 2; i++){
		if ((errno = pthread_create(&tids[i], NULL, task, &thread_params[i])) != 0)
			ERROR(10, "Erro no pthread_create()!");	
	}


    // Espera que todas as threads terminem
	for (int i = 0; i < 2; i++){
		if ((errno = pthread_join(tids[i], NULL)) != 0)
			ERROR(11, "Erro no pthread_join()!\n");
	}


    //print primeiro Vetor
    printf("First Vector:["); 

    for(int i=0;i<length;i++){
       printf(" %d ",array1[i]);
    }
    printf("]\n"); 

    //print segundo vetor
    printf("Second Vector:["); 

    for(int i=0;i<length;i++){
       printf(" %d ",array2[i]);
    }
    printf("]\n"); 

    printf("Swap Count: %d \n",count);


	// Mutex: liberta recurso
	if ((errno = pthread_mutex_destroy(&mutex)) != 0)
		ERROR(13, "pthread_mutex_destroy() failed");
		
    free(array1);
    free(array2);
    
	cmdline_parser_free(&args);

    return 0;
}

// zona das funções
void trata_sinal(int signal)
{
    if (signal == SIGINT) {
        flag = 1;
    }
}



// Zona das funções  ** (não copiar este comentário) ** 
// Thread
void *task(void *arg) {
    thread_params_t *params = (thread_params_t *) arg;
    int *array1=params->array1;
    int *array2=params->array2;
    if(params->id==2){
        if(strcmp(params->sort,"asc")==0){

            for(int i=0;i<params->length;i++){
                if(i%2!=0){
                    continue;
                }
                if(array1[i]>array2[i]){
                    int aux=array1[i]; 
                    array1[i]=array2[i]; 
                    array2[i]=aux;
                // Mutex: entra na secção crítica 
                if ((errno = pthread_mutex_lock(params->ptr_mutex)) != 0){		
                    WARNING("pthread_mutex_lock() failed");
                    return NULL;
                }
                    *params->count+=1;

                 // Mutex: sai da secção crítica 
                if ((errno = pthread_mutex_unlock(params->ptr_mutex)) != 0){
                    WARNING("pthread_mutex_unlock() failed");
                    return NULL;
                }

                }
            }

        }else{
              for(int i=0;i<params->length;i++){
                if(i%2!=0){
                    continue;
                }
                if(array1[i]<array2[i]){
                    int aux=array1[i];
                    array1[i]=array2[i];
                    array2[i]=aux;
                    
                       // Mutex: entra na secção crítica 
                if ((errno = pthread_mutex_lock(params->ptr_mutex)) != 0){		
                    WARNING("pthread_mutex_lock() failed");
                    return NULL;
                }
                    *params->count+=1;


                 // Mutex: sai da secção crítica 
                if ((errno = pthread_mutex_unlock(params->ptr_mutex)) != 0){
                    WARNING("pthread_mutex_unlock() failed");
                    return NULL;
                }

                }
            }
        }
    }else{
        // fazer para impar
         if(strcmp(params->sort,"asc")==0){

            for(int i=0;i<params->length;i++){
                if(i%2==0){
                    continue;
                }
                if(array1[i]>array2[i]){
                    int aux=array1[i];
                    array1[i]=array2[i];
                    array2[i]=aux;
                    
                        // Mutex: entra na secção crítica 
                if ((errno = pthread_mutex_lock(params->ptr_mutex)) != 0){		
                    WARNING("pthread_mutex_lock() failed");
                    return NULL;
                }
                    *params->count+=1;

                 // Mutex: sai da secção crítica 
                if ((errno = pthread_mutex_unlock(params->ptr_mutex)) != 0){
                    WARNING("pthread_mutex_unlock() failed");
                    return NULL;
                }

                }
            }

        }else{

            for(int i=0;i<params->length;i++){
                if(i%2==0){
                    continue;
                }
                if(array1[i]<array2[i]){
                    int aux=array1[i];
                    array1[i]=array2[i];
                    array2[i]=aux;
                    
                        // Mutex: entra na secção crítica 
                if ((errno = pthread_mutex_lock(params->ptr_mutex)) != 0){		
                    WARNING("pthread_mutex_lock() failed");
                    return NULL;
                }
                    *params->count+=1;

                 // Mutex: sai da secção crítica 
                if ((errno = pthread_mutex_unlock(params->ptr_mutex)) != 0){
                    WARNING("pthread_mutex_unlock() failed");
                    return NULL;
                }

                }
            }
        }
    }
    return NULL;
}
