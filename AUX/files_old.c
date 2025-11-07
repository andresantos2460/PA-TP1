/*
 * file_lowlevel_demo.c
 *
 * Demonstra várias operações de baixo nível com ficheiros em C.
 * Compilar com:
 *     gcc -Wall -O2 file_lowlevel_demo.c -o file_lowlevel_demo
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>      // open, O_* flags
#include <unistd.h>     // read, write, close, lseek
#include <sys/stat.h>   // fstat, struct stat
#include <string.h>     // strlen
#include <errno.h>      // errno, perror
#include <sys/mman.h>   // mmap, munmap
#include <dirent.h>     // opendir, readdir, closedir

int main(void) {
    int fd;
    ssize_t n;
    char buf[128];
    struct stat st;

    /* ---------------------------------------------------------------------- */
    /* 1. ABRIR / CRIAR FICHEIRO */
    /* ---------------------------------------------------------------------- */
    fd = open("exemplo.txt", O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        perror("open");
        return 1;
    }

    /* ---------------------------------------------------------------------- */
    /* 2. ESCREVER DADOS */
    /* ---------------------------------------------------------------------- */
    const char *msg = "Olá, este é um teste de escrita low-level!\n";
    n = write(fd, msg, strlen(msg));
    if (n < 0) {
        perror("write");
        close(fd);
        return 1;
    }

    /* ---------------------------------------------------------------------- */
    /* 3. MOVER CURSOR E LER */
    /* ---------------------------------------------------------------------- */
    if (lseek(fd, 0, SEEK_SET) == -1) {
        perror("lseek");
        close(fd);
        return 1;
    }

    memset(buf, 0, sizeof(buf));
    n = read(fd, buf, sizeof(buf)-1);
    if (n < 0) {
        perror("read");
        close(fd);
        return 1;
    }
    printf("Conteúdo lido:\n%s\n", buf);

    /* ---------------------------------------------------------------------- */
    /* 4. OBTÉM METADADOS */
    /* ---------------------------------------------------------------------- */
    if (fstat(fd, &st) == -1) {
        perror("fstat");
    } else {
        printf("Tamanho: %lld bytes\n", (long long)st.st_size);
    }

    /* ---------------------------------------------------------------------- */
    /* 5. MAPEAR EM MEMÓRIA (MMAP) */
    /* ---------------------------------------------------------------------- */
    void *map = mmap(NULL, st.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (map == MAP_FAILED) {
        perror("mmap");
    } else {
        printf("Primeiros 10 bytes via mmap: ");
        fwrite(map, 1, (st.st_size < 10 ? st.st_size : 10), stdout);
        printf("\n");
        munmap(map, st.st_size);
    }

    /* ---------------------------------------------------------------------- */
    /* 6. SINCRONIZAR E FECHAR */
    /* ---------------------------------------------------------------------- */
    if (fsync(fd) == -1)
        perror("fsync");

    if (close(fd) == -1) {
        perror("close");
        return 1;
    }

    /* ---------------------------------------------------------------------- */
    /* 7. LER DIRETÓRIO ATUAL */
    /* ---------------------------------------------------------------------- */
    DIR *d = opendir(".");
    if (!d) {
        perror("opendir");
        return 1;
    }
    struct dirent *e;
    printf("\nListagem do diretório atual:\n");
    while ((e = readdir(d))) {
        printf(" - %s\n", e->d_name);
    }
    closedir(d);

    /* ---------------------------------------------------------------------- */
    /* 8. REMOVER FICHEIRO (UNLINK) */
    /* ---------------------------------------------------------------------- */
    if (unlink("exemplo.txt") == -1) {
        perror("unlink");
    } else {
        printf("\nFicheiro 'exemplo.txt' removido.\n");
    }

    return 0;
}


/*
 * file_lowlevel_reference.c
 *
 * Referência de operações de baixo nível com ficheiros em C (POSIX).
 * Apenas syscalls - SEM funções de alto nível como fopen/fclose/fprintf.
 *
 * Compilar: gcc -Wall -O2 file_lowlevel_reference.c -o file_lowlevel_ref
 */

#include <fcntl.h>      // open, O_* flags
#include <unistd.h>     // read, write, close, lseek, unlink, dup, dup2
#include <sys/stat.h>   // fstat, stat, chmod, mkdir
#include <sys/mman.h>   // mmap, munmap
#include <dirent.h>     // opendir, readdir, closedir
#include <string.h>     // strlen, memset
#include <stdio.h>      // apenas para printf (demonstração)
#include <errno.h>      // errno

/* ========================================================================== */
/* OPERAÇÕES BÁSICAS COM FICHEIROS                                           */
/* ========================================================================== */

void demo_open_close() {
    printf("\n=== OPEN / CLOSE ===\n");
    
    // Abrir para leitura
    int fd = open("teste.txt", O_RDONLY);
    if (fd == -1) {
        perror("open (read)");
        return;
    }
    close(fd);
    
    // Criar ficheiro novo (trunca se existir)
    fd = open("novo.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        perror("open (create)");
        return;
    }
    close(fd);
    
    // Abrir para escrita no fim (append)
    fd = open("log.txt", O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd == -1) {
        perror("open (append)");
        return;
    }
    close(fd);
    
    // Abrir para leitura e escrita
    fd = open("data.bin", O_RDWR | O_CREAT, 0644);
    if (fd == -1) {
        perror("open (rdwr)");
        return;
    }
    close(fd);
}

/* ========================================================================== */
/* LEITURA E ESCRITA                                                         */
/* ========================================================================== */

void demo_read_write() {
    printf("\n=== READ / WRITE ===\n");
    
    int fd = open("exemplo.txt", O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        perror("open");
        return;
    }
    
    // Escrever dados
    const char *msg = "Linha 1\nLinha 2\nLinha 3\n";
    ssize_t written = write(fd, msg, strlen(msg));
    if (written == -1) {
        perror("write");
        close(fd);
        return;
    }
    printf("Escritos %zd bytes\n", written);
    
    // Voltar ao início
    lseek(fd, 0, SEEK_SET);
    
    // Ler dados
    char buffer[256];
    memset(buffer, 0, sizeof(buffer));
    ssize_t nread = read(fd, buffer, sizeof(buffer) - 1);
    if (nread == -1) {
        perror("read");
        close(fd);
        return;
    }
    printf("Lidos %zd bytes:\n%s", nread, buffer);
    
    close(fd);
}

/* ========================================================================== */
/* POSICIONAMENTO NO FICHEIRO (LSEEK)                                        */
/* ========================================================================== */

void demo_lseek() {
    printf("\n=== LSEEK ===\n");
    
    int fd = open("data.txt", O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        perror("open");
        return;
    }
    
    write(fd, "0123456789ABCDEF", 16);
    
    // Voltar ao início
    lseek(fd, 0, SEEK_SET);
    
    // Ir para posição 5
    lseek(fd, 5, SEEK_SET);
    char c;
    read(fd, &c, 1);
    printf("Byte na posição 5: %c\n", c);
    
    // Avançar 3 bytes da posição atual
    lseek(fd, 3, SEEK_CUR);
    read(fd, &c, 1);
    printf("Byte após avançar +3: %c\n", c);
    
    // Ir para 2 bytes antes do fim
    lseek(fd, -2, SEEK_END);
    read(fd, &c, 1);
    printf("Byte a -2 do fim: %c\n", c);
    
    // Obter posição atual
    off_t pos = lseek(fd, 0, SEEK_CUR);
    printf("Posição atual: %lld\n", (long long)pos);
    
    close(fd);
}

/* ========================================================================== */
/* METADADOS DO FICHEIRO (STAT/FSTAT)                                       */
/* ========================================================================== */

void demo_stat() {
    printf("\n=== STAT / FSTAT ===\n");
    
    int fd = open("metadata.txt", O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        perror("open");
        return;
    }
    
    write(fd, "Dados de teste", 14);
    
    // fstat - via file descriptor
    struct stat st;
    if (fstat(fd, &st) == -1) {
        perror("fstat");
        close(fd);
        return;
    }
    
    printf("Tamanho: %lld bytes\n", (long long)st.st_size);
    printf("Permissões: %o\n", st.st_mode & 0777);
    printf("UID: %d, GID: %d\n", st.st_uid, st.st_gid);
    printf("Último acesso: %ld\n", st.st_atime);
    printf("Última modificação: %ld\n", st.st_mtime);
    
    // stat - via path
    if (stat("metadata.txt", &st) == -1) {
        perror("stat");
    } else {
        printf("Mesmas infos via stat() pelo path\n");
    }
    
    close(fd);
}

/* ========================================================================== */
/* DUPLICAÇÃO DE FILE DESCRIPTORS (DUP/DUP2)                                */
/* ========================================================================== */

void demo_dup() {
    printf("\n=== DUP / DUP2 ===\n");
    
    int fd = open("dup_test.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        perror("open");
        return;
    }
    
    // Duplicar file descriptor
    int fd2 = dup(fd);
    if (fd2 == -1) {
        perror("dup");
        close(fd);
        return;
    }
    
    printf("fd original: %d, fd duplicado: %d\n", fd, fd2);
    
    // Ambos apontam para o mesmo ficheiro
    write(fd, "Escrita via fd1\n", 16);
    write(fd2, "Escrita via fd2\n", 16);
    
    close(fd);
    close(fd2);
    
    // dup2 - duplicar para fd específico
    fd = open("dup2_test.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        perror("open");
        return;
    }
    
    int old_stdout = dup(STDOUT_FILENO);  // Guardar stdout
    dup2(fd, STDOUT_FILENO);              // Redirecionar stdout para ficheiro
    
    printf("Esta linha vai para o ficheiro!\n");  // Vai para dup2_test.txt
    
    dup2(old_stdout, STDOUT_FILENO);      // Restaurar stdout
    close(old_stdout);
    close(fd);
    
    printf("Stdout restaurado!\n");  // Volta para terminal
}

/* ========================================================================== */
/* SINCRONIZAÇÃO (FSYNC)