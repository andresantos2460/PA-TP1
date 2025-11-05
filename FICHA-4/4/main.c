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
#include <dirent.h>

#include "debug.h"
#include "memory.h"
#include "args.h"

struct gengetopt_args_info args;

typedef struct {
    int id;
    pthread_mutex_t *mutex;
    pthread_cond_t *cond;
    char ***files;           // triplo ponteiro para modificar o char**
    int *ready;
    int *next_file_index;
} thread_params_t;
int is_PDF( char *path);
void *producer(void *arg);
void *consumer(void *arg);
char **ler_ficheiros(const char *path);

int main(int argc, char *argv[]) {
    if (cmdline_parser(argc, argv, &args))
        ERROR(1, "cmdline_parser failed");

    int num_consumers = args.thread_arg;
    int total_threads = num_consumers + 1;

    pthread_mutex_t mutex;
    pthread_cond_t cond;
    char **shared_files = NULL;
    int ready = 0;
    int next_index = 0;

    if (pthread_mutex_init(&mutex, NULL) != 0)
        ERROR(12, "pthread_mutex_init failed");
    
    if (pthread_cond_init(&cond, NULL) != 0)
        ERROR(12, "pthread_cond_init failed");

    pthread_t *threads = malloc(sizeof(pthread_t) * total_threads);
    if (!threads) 
        ERROR(2, "malloc failed");

    thread_params_t *params = malloc(sizeof(thread_params_t) * total_threads);
    if (!params) 
        ERROR(2, "malloc failed");

    // Inicializar parâmetros - todos apontam para &shared_files
    for (int i = 0; i < total_threads; i++) {
        params[i].id = i;
        params[i].mutex = &mutex;
        params[i].cond = &cond;
        params[i].files = &shared_files;  // todos veem o mesmo ponteiro
        params[i].ready = &ready;
        params[i].next_file_index = &next_index;
    }

    // Criar producer (thread 0)
    if (pthread_create(&threads[0], NULL, producer, &params[0]) != 0)
        ERROR(10, "pthread_create failed");

    // Criar consumers (threads 1..N) - podem ser criados imediatamente
    for (int i = 1; i < total_threads; i++) {
        if (pthread_create(&threads[i], NULL, consumer, &params[i]) != 0)
            ERROR(10, "pthread_create failed");
    }

    // Esperar por todas as threads
    for (int i = 0; i < total_threads; i++) {
        if (pthread_join(threads[i], NULL) != 0)
            ERROR(11, "pthread_join failed");
    }

    // Cleanup
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);
    
    if (shared_files) {
        for (int i = 0; shared_files[i] != NULL; i++)
            free(shared_files[i]);
        free(shared_files);
    }
    free(threads);
    free(params);
    cmdline_parser_free(&args);

    return 0;
}

void *producer(void *arg) {
    thread_params_t *p = arg;

    char **files = ler_ficheiros(args.file_arg);
    
    pthread_mutex_lock(p->mutex);
    *(p->files) = files;  // modifica o shared_files através do ponteiro
    *(p->ready) = 1;
    pthread_cond_broadcast(p->cond);
    pthread_mutex_unlock(p->mutex);

    return NULL;
}

void *consumer(void *arg) {
    thread_params_t *p = arg;

    // Esperar que o producer carregue os ficheiros
    pthread_mutex_lock(p->mutex);
    while (*(p->ready) == 0) {
        pthread_cond_wait(p->cond, p->mutex);
    }
    char **files = *(p->files);  // lê o shared_files
    pthread_mutex_unlock(p->mutex);

    if (!files) 
        return NULL;

    // Processar ficheiros sem repetição
    while (1) {
        pthread_mutex_lock(p->mutex);
        int idx = (*(p->next_file_index))++;
        pthread_mutex_unlock(p->mutex);

        if (files[idx] == NULL)
            break;
        if(is_PDF(files[idx])==1){
            printf("%s\n",files[idx]);
        }
    }

    return NULL;
}
int is_PDF(char *path) {
    FILE *fp;
    unsigned char buffer[4];
    const unsigned char pdf_header[4] = {0x25, 0x50, 0x44, 0x46};  // %PDF
    fp = fopen(path, "rb");
    if (fp == NULL) {
        // se quiser pode imprimir o erro, mas retorna 0 (não é PDF)
        // perror("Erro ao abrir o arquivo");
        return 0;
    }

    size_t lidos = fread(buffer, 1, 4, fp);
    fclose(fp);

    if (lidos < 4) {
        // printf("Arquivo muito pequeno.\n");
        return 0;
    }

    if (memcmp(buffer, pdf_header, 4) == 0)
        return 1;  // é PDF
    else
        return 0;  // não é PDF
}
char **ler_ficheiros(const char *path) {
    DIR *dir = opendir(path);
    if (!dir) {
        perror("opendir");
        return NULL;
    }

    struct dirent *entry;
    int count = 0;
    char **buffer = NULL;
    char fullpath[PATH_MAX];  // precisa de <limits.h>

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        // Monta o caminho completo: path + "/" + nome
        snprintf(fullpath, sizeof(fullpath), "%s/%s", path, entry->d_name);

        buffer = realloc(buffer, sizeof(char*) * (count + 2));
        if (!buffer) {
            perror("realloc");
            closedir(dir);
            return NULL;
        }
        
        buffer[count] = strdup(fullpath);
        if (!buffer[count]) {
            perror("strdup");
            closedir(dir);
            return NULL;
        }
        count++;
    }

    closedir(dir);

    if (buffer)
        buffer[count] = NULL;

    return buffer;
}
