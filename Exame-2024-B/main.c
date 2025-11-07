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
    int min;
    int max;
	int *ptr_jogadores_tentativa;
	int *ptr_juri_avaliacao;
    int *ronda;
    int *found_winner;
    int *estado;
    int n_jogadores;
    int winner_number;
    int *jogadores_prontos;
	pthread_cond_t *ptr_cond;
	pthread_mutex_t *ptr_mutex;
}thread_params_t;

int rand_between(int min, int max);
void *task_juri(void *arg);
void *task_jogador(void *arg);


int main(int argc, char *argv[]) {

    struct gengetopt_args_info args;
	// gengetopt parser: deve ser a primeira linha de código no main
	if(cmdline_parser(argc, argv, &args))
		ERROR(1, "Erro: execução de cmdline_parser\n");

    int num_players=args.num_players_arg;   
    int numero_a_advinhar=args.number_arg;

    if(numero_a_advinhar==0){

        numero_a_advinhar=rand_between(2,100);

    }else{

        if(numero_a_advinhar<2 ||numero_a_advinhar>100)ERROR(1,"numero fora do parametro valido!");

    }
    if(num_players<1)ERROR(1,"numero de jogadores invalido");


    printf("Number To Gueed: %d \n",numero_a_advinhar);

    int *jogadores_tentativa=malloc(sizeof(int)*num_players);
    if(!jogadores_tentativa)ERROR(2,"Error setting malloc");
    memset(jogadores_tentativa, 0, sizeof(int)*num_players);


     int *juri_avaliacao=malloc(sizeof(int)*num_players);
    if(!juri_avaliacao)ERROR(2,"Error setting malloc");
    memset(juri_avaliacao,0,sizeof(int)*num_players);

    int ronda=1;
    int found_winner=0;
    int estado=0;
    int jogadores_prontos=0;

    pthread_t *tids=malloc(sizeof(pthread_t)*num_players);
    if (!tids) ERROR(2, "malloc failed for tids");

    thread_params_t *thread_params = malloc(sizeof(thread_params_t) * num_players);
    if (!thread_params) ERROR(2, "malloc failed for thread_params");

	pthread_t juri;
	thread_params_t thread_params_juri;

    pthread_mutex_t mutex;
	pthread_cond_t cond;


    	// Mutex: inicializa o mutex antes de criar a(s) thread(s)	
	if ((errno = pthread_mutex_init(&mutex, NULL)) != 0)
		ERROR(12, "pthread_mutex_init() failed");


    	// Var.Condição: inicializa variável de condição
	if ((errno = pthread_cond_init(&cond, NULL)) != 0)
		ERROR(14, "pthread_cond_init() failed!");


	thread_params_juri.id = 999;
    thread_params_juri.ptr_jogadores_tentativa=jogadores_tentativa;
    thread_params_juri.ptr_juri_avaliacao=juri_avaliacao;
    thread_params_juri.ptr_mutex=&mutex;
    thread_params_juri.ptr_cond=&cond;
    thread_params_juri.ronda=&ronda;
    thread_params_juri.found_winner=&found_winner;
    thread_params_juri.estado=&estado;
    thread_params_juri.n_jogadores=num_players;
    thread_params_juri.winner_number=numero_a_advinhar;
    thread_params_juri.jogadores_prontos=&jogadores_prontos;
    thread_params_juri.min=2;
    thread_params_juri.max=100;

    if ((errno = pthread_create(&juri, NULL, task_juri, &thread_params_juri)) != 0)
    ERROR(10, "Erro no pthread_create()!");	


	// Inicialização das estruturas - para cada thread
	for (int i = 0; i < num_players;i++){
		thread_params[i].id = i;
		thread_params[i].ptr_jogadores_tentativa=jogadores_tentativa;
		thread_params[i].ptr_juri_avaliacao=juri_avaliacao;
		thread_params[i].ptr_mutex=&mutex;
		thread_params[i].ptr_cond=&cond;
		thread_params[i].found_winner=&found_winner;
		thread_params[i].estado=&estado;
		thread_params[i].n_jogadores=num_players;
		thread_params[i].winner_number=numero_a_advinhar;
        thread_params[i].jogadores_prontos=&jogadores_prontos;
        thread_params[i].min=2;
        thread_params[i].max=100;

	}
	
	// Criação das threads + passagem de parâmetro
	for (int i = 0; i < num_players;i++){
     
        if ((errno = pthread_create(&tids[i], NULL, task_jogador, &thread_params[i])) != 0)
		ERROR(10, "Erro no pthread_create()!");	
        
		
	}
   /* Espera que todas as threads terminem */
    if ((errno = pthread_join(juri, NULL)) != 0)
        ERROR(11, "pthread_join() failed!");

    // Espera que todas as threads terminem
	for (int i = 0; i < num_players;i++){
		if ((errno = pthread_join(tids[i], NULL)) != 0)
			ERROR(11, "Erro no pthread_join()!\n");
	}
	
	// Var.Condição: destroi a variável de condição 
	if ((errno = pthread_cond_destroy(&cond)) != 0)
		ERROR(15,"pthread_cond_destroy failed!");


        
	// Mutex: liberta recurso
	if ((errno = pthread_mutex_destroy(&mutex)) != 0)
		ERROR(13, "pthread_mutex_destroy() failed");
	// gengetopt: libertar recurso (assim que possível)
	cmdline_parser_free(&args);

    free(jogadores_tentativa);
    free(juri_avaliacao);
    free(tids);
    free(thread_params);


    return 0;
}
// Zona das funções  ** (não copiar este comentário) ** 
// Thread
void *task_juri(void *arg) 
{
	// cast para o tipo de dados enviado pela 'main thread'
	thread_params_t *params = (thread_params_t *) arg;

	while (1)
    {
        if ((errno = pthread_mutex_lock(params->ptr_mutex)) != 0) {		
            WARNING("pthread_mutex_lock() failed");
            return NULL;
        }
      

        while (*params->estado==0){
            printf("[JURY] Waiting for round #%d to finish...\n",*params->ronda);
            if ((errno = pthread_cond_wait(params->ptr_cond, params->ptr_mutex)!=0)){   
            WARNING("pthread_cond_wait() failed");
            	// Mutex: sai da secção crítica 
            return NULL;
            }
      
      

        }

        for (int i = 0; i < params->n_jogadores; i++)
        {
            if(params->ptr_jogadores_tentativa[i]<params->winner_number){
                printf("[JURY] Player %d guessed %d.\n",i+1,params->ptr_jogadores_tentativa[i]);
                params->ptr_juri_avaliacao[i]=1;
            }else if (params->ptr_jogadores_tentativa[i]>params->winner_number){
                printf("[JURY] Player %d guessed %d.\n",i+1,params->ptr_jogadores_tentativa[i]);
                params->ptr_juri_avaliacao[i]=-1;
            }else{
                printf("[JURY] Player %d is a WINNER!\n",i+1);
                *params->found_winner=1;
            }
        }
                  // Verifica se já chegou ao fim
        if (*params->found_winner==1) {
            printf("[JURY] Game finished in %d round(s).\n",*params->ronda);
            *params->estado = 0;
        // Var.Condição: notifica a primeira thread à espera na variável de condição
        if ((errno = pthread_cond_broadcast(params->ptr_cond)) != 0) {
            WARNING("pthread_cond_broadcast() failed");
            return NULL;
        }
            pthread_mutex_unlock(params->ptr_mutex);
            break;
        }
        
        (*params->jogadores_prontos)=0;
        (*params->estado)=0;
        (*params->ronda)++;
        // Var.Condição: notifica a primeira thread à espera na variável de condição
        if ((errno = pthread_cond_broadcast(params->ptr_cond)) != 0) {
            WARNING("pthread_cond_broadcast() failed");
            return NULL;
        }

        if ((errno = pthread_mutex_unlock(params->ptr_mutex)) != 0) {
            WARNING("pthread_mutex_unlock() failed");
            return NULL;
        }
    }
    
  
    
	
	return NULL;
}

void *task_jogador(void *arg) 
{
	// cast para o tipo de dados enviado pela 'main thread'
	thread_params_t *params = (thread_params_t *) arg;

    while (1){
         if ((errno = pthread_mutex_lock(params->ptr_mutex)) != 0) {		
            WARNING("pthread_mutex_lock() failed");
            return NULL;
        }


      
        // ✅ CRÍTICO: Verifica found_winner na condição do while
        while (*params->estado == 1 && *params->found_winner == 0) {
            if ((errno = pthread_cond_wait(params->ptr_cond, params->ptr_mutex)) != 0) {
                WARNING("pthread_cond_wait() failed");
                pthread_mutex_unlock(params->ptr_mutex);
                return NULL;
            }
        }

        // Verifica se jogo acabou
        if (*params->found_winner == 1) {
            pthread_mutex_unlock(params->ptr_mutex);
            break;
        }
        
            int i=params->id;

            if(params->ptr_juri_avaliacao[i]==0){
                int random=rand_between(params->min,params->max);
                params->ptr_jogadores_tentativa[i]=random;
            }
            if(params->ptr_juri_avaliacao[i]==1){
                params->min=params->ptr_jogadores_tentativa[i]+1;
                int random=rand_between(params->min,params->max);
                params->ptr_jogadores_tentativa[i]=random;
                params->ptr_juri_avaliacao[i] = 0;   // ✅ limpa após consumir
            }
             if(params->ptr_juri_avaliacao[i]==-1){
                params->max=params->ptr_jogadores_tentativa[i]-1;
                int random=rand_between(params->min,params->max);
                params->ptr_jogadores_tentativa[i]=random;
                params->ptr_juri_avaliacao[i] = 0;   // ✅ limpa após consumir
            }
            (*params->jogadores_prontos)++;


            if(*params->jogadores_prontos==params->n_jogadores){
                *params->estado=1;
        // Var.Condição: notifica a primeira thread à espera na variável de condição
                if ((errno = pthread_cond_signal(params->ptr_cond)) != 0){
                    WARNING("pthread_cond_signal() failed");
                    return NULL;
                }
                           
            }
            

            if ((errno = pthread_mutex_unlock(params->ptr_mutex)) != 0) {
                    WARNING("pthread_mutex_unlock() failed");
                    return NULL;
                }
 
    
}
	return NULL;


}


int rand_between(int min, int max) {
    static _Thread_local unsigned int seed = 0;
    
    // Inicializa seed apenas uma vez por thread
    if (seed == 0) {
        seed = time(NULL) ^ getpid() ^ (uintptr_t)pthread_self();
    }
    
    return min + rand_r(&seed) % (max - min + 1);
}
