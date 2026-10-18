#include "pipe_lab.h"

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

int task_one_way(void) {
    int fd[2];
    char buf[16] ;
    const char *msg = "ping";

    if (pipe(fd) == -1) { perror("pipe"); return 1; }

    pid_t pid = fork();
    if (pid < 0) { perror("fork"); return 1; }

    if (pid == 0) {
        close(fd[1]);
        ssize_t n = read(fd[0], buf, sizeof(buf) - 1);
        if (n > 0) {
            buf[n] = '\0';
        }
        printf("child received: %s\n", buf);
        fflush(stdout);
        close(fd[0]);
        _exit(0);
    }

    close(fd[0]);
    write(fd[1], msg , strlen(msg));
    close(fd[1]);
    waitpid(pid, NULL, 0);
    printf("parent: sent ping\n");
    fflush(stdout); 
    return 0;
}

int task_two_way(void) {
    int p2c[2], c2p[2];
    int val , res;

    if (pipe(p2c) == -1 || pipe(c2p) == -1) { perror("pipe"); return 1; }

    pid_t pid = fork();
    if (pid < 0) { perror("fork"); return 1; }

    if (pid == 0) {
        close(p2c[1]);
        close(c2p[0]);
        
        read(p2c[0], &val, sizeof(val));

        val *= 2;
        printf("child computed: %d\n", val);
        fflush(stdout);
        write(c2p[1], &val, sizeof(val));
        close(p2c[0]);
        close(c2p[1]);
        _exit(0);
    }

    close(p2c[0]);
    close(c2p[1]);

    val = 21;
    write(p2c[1], &val, sizeof(val));

    read(c2p[0], &res, sizeof(res));

    close(p2c[1]);
    close(c2p[0]);

    waitpid(pid, NULL, 0);
    printf("parent received: %d\n", res);
    fflush(stdout); 
    return 0;
}

int task_eof(void) {
    int fd[2];
    const char *msg = "hello";
    char buf[16];
    int total_bytes = 0;

    if (pipe(fd) == -1) { perror("pipe"); return 1; }

    pid_t pid = fork();
    if (pid < 0) { perror("fork"); return 1; }

    if (pid == 0) {

        close(fd[1]);
        ssize_t bytes_read;
        while ((bytes_read = read(fd[0], buf, sizeof(buf))) > 0) {
            total_bytes += bytes_read;
        }
        printf("child: got EOF after %d bytes\n", total_bytes);
        fflush(stdout);
        close(fd[0]);
        _exit(0);
    }

    close(fd[0]);
    write(fd[1], msg, strlen(msg));
    close(fd[1]);
    waitpid(pid, NULL, 0);
    return 0;
}

int task_broken_pipe(void) {
    int data_fd[2];
    int sync_fd[2];
    if (pipe(data_fd) == -1 || pipe(sync_fd) == -1) { perror("pipe"); return 1; }

    pid_t pid = fork();
    if (pid < 0) { perror("fork"); return 1; }

    if (pid == 0) {

        close(data_fd[0]);
        close(data_fd[1]);
        
        char ack = 'x';
        write(sync_fd[1], &ack, 1);
        
        close(sync_fd[1]);
        close(sync_fd[0]);
        _exit(0);
    }

    close(sync_fd[1]);

    char ack;
    read(sync_fd[0], &ack, 1);
    close(sync_fd[0]);
    close(data_fd[0]);

    signal(SIGPIPE, SIG_IGN);

    char b = 'a';
    if( write(data_fd[1], &b , 1) == -1 && errno == EPIPE ){
        printf("parent: broken pipe detected\n");
        fflush(stdout); 
    }
    close(data_fd[1]);
    waitpid(pid, NULL, 0);
    return 0;
}

/* ------------------------------------------------------------------ */
int main(void) {
    if (task_one_way() != 0)     { fprintf(stderr, "task_one_way failed\n");     return 1; }
    if (task_two_way() != 0)     { fprintf(stderr, "task_two_way failed\n");     return 1; }
    if (task_eof() != 0)         { fprintf(stderr, "task_eof failed\n");         return 1; }
    if (task_broken_pipe() != 0) { fprintf(stderr, "task_broken_pipe failed\n"); return 1; }
    printf("all tasks done\n");
    return 0;
}