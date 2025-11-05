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

#include "debug.h"
#include "args.h"
#include "memory.h"

struct gengetopt_args_info args;

typedef struct {
	int id;
    size_t *ptr_numBytes;
    pthread_mutex_t *ptr_mutex;
	FILE *fptr;
	char *buffer;
    int *freq;

} thread_params_t;

void *task(void *arg);


int main(int argc, char *argv[]) {
  
	// gengetopt parser: deve ser a primeira linha de código no main
	if(cmdline_parser(argc, argv, &args))
		ERROR(1, "Erro: execução de cmdline_parser\n");

    int threads_to_do=args.thread_arg;
    size_t byte=args.byte_arg;
    char *filename=args.file_arg;

	 // variavel de contagem de bytes acessivel a todas as threads
    size_t *totalBytesRead = malloc(sizeof(size_t));
    if (!totalBytesRead) ERROR(2, "malloc failed for totalBytesRead");
    *totalBytesRead = 0;

    int *global_freq = malloc(26 * sizeof(int));
    if (!global_freq) ERROR(2, "malloc failed for global_freq");
    memset(global_freq, 0, 26 * sizeof(int));


	pthread_t *tids=malloc(sizeof(pthread_t)*threads_to_do);
	if (!tids) ERROR(2, "malloc failed for tids");

    thread_params_t *thread_params = malloc(sizeof(thread_params_t) * threads_to_do);
    if (!thread_params) ERROR(2, "malloc failed for thread_params");


	pthread_mutex_t mutex;
	//MAIN 
  	// Mutex: inicializa o mutex antes de criar a(s) thread(s)	
	if ((errno = pthread_mutex_init(&mutex, NULL)) != 0)
		ERROR(12, "pthread_mutex_init() failed");

	FILE* fptr;

    // Opening the file in read mode
    fptr = fopen(filename, "r");

    // checking if the file is 
    // opened successfully
    if (fptr == NULL) {
		ERROR(1,"ERROR OPENING FILE ");
    }




	// Inicialização das estruturas - para cada thread
	for (int i = 0; i < threads_to_do; i++){
        thread_params[i].id = i+1;
        thread_params[i].ptr_mutex = &mutex;
        thread_params[i].fptr = fptr;
        thread_params[i].buffer = malloc(byte);
     if (!thread_params[i].buffer) ERROR(2, "malloc failed for thread buffer");
        thread_params[i].ptr_numBytes = totalBytesRead;
        thread_params[i].freq = global_freq;
	}


	// Criação das threads + passagem de parâmetro
	for (int i = 0; i < threads_to_do; i++){

        if ((errno = pthread_create(&tids[i], NULL, task, &thread_params[i])) != 0)
			ERROR(10, "Erro no pthread_create()!");	
      
		
	}


    // Espera que todas as threads terminem
	for (int i = 0; i < threads_to_do; i++){
		if ((errno = pthread_join(tids[i], NULL)) != 0)
			ERROR(11, "Erro no pthread_join()!\n");
	}
    for (int i = 0; i < 26; i++) {
        printf("Frequencia: %c = %d\n", 'A' + i, global_freq[i]);
    }


	// Mutex: liberta recurso
	if ((errno = pthread_mutex_destroy(&mutex)) != 0)
		ERROR(13, "pthread_mutex_destroy() failed");

	if (fclose(fptr) != 0)
	ERROR(6, "fclose() - não foi possível fechar o ficheiro");

		
    // free dos buffers das threads
    for (int i = 0; i < threads_to_do; i++) {
        free(thread_params[i].buffer);
    }

    // free dos vetores dinamicos
    free(global_freq);
    free(tids); 
    free(thread_params);
    free(totalBytesRead); 
	cmdline_parser_free(&args);

    return 0;
}
	void *task(void *arg){

		thread_params_t *params = (thread_params_t *)arg;
		size_t bytes_read;
		int local_freq[26] = {0}; // Frequência local para esta thread

			while (1){
					
			if ((errno = pthread_mutex_lock(params->ptr_mutex)) != 0){		
				WARNING("pthread_mutex_lock() failed");
				return NULL;
			}
				
				bytes_read = fread(params->buffer, 1, args.byte_arg, params->fptr);
				*params->ptr_numBytes += bytes_read;

				printf("THREAD: %d , leu: %zu \n",params->id,bytes_read);
				
				if ((errno = pthread_mutex_unlock(params->ptr_mutex)) != 0){
				WARNING("pthread_mutex_unlock() failed");
				return NULL;
			}

				// Se não lemos nada, saímos do loop
				if (bytes_read == 0) {
					break;
				}

				for (int i = 0; (size_t)i < bytes_read; i++) {
					unsigned char c = params->buffer[i];  // ← pega o caractere atual

					if (isalpha(c)) {
						c = tolower(c);
						local_freq[c - 'a']++;
					}
				}
							// Secção crítica: atualizar frequências globais
				if ((errno = pthread_mutex_lock(params->ptr_mutex)) != 0){		
					WARNING("pthread_mutex_lock() failed");
					return NULL;
				}
				
				for (int i = 0; i < 26; i++) {
					params->freq[i] += local_freq[i];
				}
				
				if ((errno = pthread_mutex_unlock(params->ptr_mutex)) != 0){
					WARNING("pthread_mutex_unlock() failed");
					return NULL;
				}

			}
			return NULL;
	}
