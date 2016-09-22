#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

typedef void (*capture_func)(void *);

int capture(capture_func f, void *ctx, char **out) {
    pid_t pid;
    int opipe[2];

    if(pipe(opipe) != 0) { return -1; }
    if((pid = fork()) == -1) { return -1; }

    if(pid == 0) {
        close(opipe[0]);

        dup2(opipe[1], 1);
        dup2(opipe[1], 2);

        f(ctx);

        _exit(0);
    } else {
        int status;
        char buf[1024];
        ssize_t nread;
        size_t outlen;
        FILE *ostream = open_memstream(out, &outlen);

        close(opipe[1]);

        while((nread = read(opipe[0], buf, 1024)) > 0) {
            fwrite(buf, 1, nread, ostream);
        }
        fclose(ostream);

        waitpid(pid, &status, 0);
        return WEXITSTATUS(status);
    }
}
