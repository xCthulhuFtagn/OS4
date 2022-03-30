#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>

int main(int argc, char* argv[]){
    if (argc != 4) {
        printf("Wrong number of arguments!\n");
        exit(EXIT_FAILURE);
    }
    int id = fork(), pipe4orig_desc[2], pipe4key_desc[2], output_desc, st;
    if(pipe(pipe4key_desc) == -1 || pipe(pipe4orig_desc) == -1) {
        printf("Pipe opening failure\n");
        exit(EXIT_FAILURE);
    }
    switch (id)
    {
    case -1:
        printf("Fork in main process broke\n");
        exit(EXIT_FAILURE);
    case 0:
    {
        int id2 = fork();
        switch (id2)
        {
        case -1:
            printf("Fork in subprocess broke\n");
            exit(EXIT_FAILURE);
        case 0:
            close(pipe4key_desc[0]);
            close(pipe4key_desc[1]);
            close(pipe4orig_desc[0]);
            dup2(pipe4orig_desc[1], STDOUT_FILENO);
            if (execlp("./printer", "./printer", argv[1], NULL) == EXIT_FAILURE)
                exit(EXIT_FAILURE); //original
            close(pipe4orig_desc[1]);
            exit(EXIT_SUCCESS);
        default:
            close(pipe4orig_desc[0]);
            close(pipe4orig_desc[1]);
            close(pipe4key_desc[0]);
            dup2(pipe4key_desc[1], STDOUT_FILENO);
            if (execlp("./printer", "./printer", argv[2], NULL) == EXIT_FAILURE) // returns control unlike execvp
                exit(EXIT_FAILURE); //key
            close(pipe4key_desc[1]);
            exit(EXIT_SUCCESS);
            waitpid(id2, &st, 0);
            exit(WEXITSTATUS(st));
        }
        break;
    }
    default:
    {
        close(pipe4orig_desc[1]);
        close(pipe4key_desc[1]);
        waitpid(id, &st, 0);
        if (WEXITSTATUS(st) != EXIT_FAILURE) {
            if((output_desc = open(argv[3], O_WRONLY | O_CREAT | O_TRUNC, S_IWUSR | S_IRUSR)) < 0){
                printf("Output file could not be opened\n");
                exit(EXIT_FAILURE);
            }
            int biba, boba; //professional naming
            char biba_buf[BUFSIZ], boba_buf[BUFSIZ];
            int off = 0;
            while ((biba = read(pipe4key_desc[0], biba_buf, BUFSIZ)) >= 0 && (boba = read(pipe4orig_desc[0], boba_buf + off, BUFSIZ - off)) >= 0) {
                if (biba >= boba) {
                    for (int i = 0; i < boba; ++i) boba_buf[i] ^= biba_buf[i];
                } else {
                    int i;
                    for (i = 0; i < biba; ++i) boba_buf[i] ^= biba_buf[i];
                    off = i;
                    memcpy(boba_buf, boba_buf + off, boba - biba);
                    lseek(pipe4key_desc[0], 0, SEEK_SET);
                }
                if (write(output_desc, boba_buf, ((biba <= boba)? biba : boba)) < 0) {
                    close(pipe4orig_desc[0]);
                    close(pipe4key_desc[0]);
                    close(output_desc);
                    printf("Writing error\n");
                    exit(EXIT_FAILURE);
                }
            }
            if (biba < 0 || boba < 0) {
                close(output_desc);
                printf("Reading error\n");
                exit(EXIT_FAILURE);
            }
            if (close(pipe4orig_desc[0]) == -1 || close(pipe4key_desc[0]) == -1) {
                printf("One of the pipes could not be closed\n");
                exit(EXIT_FAILURE);
            }
            if (close(output_desc) == -1) {
                printf("Output file could not be closed\n");
                exit(EXIT_FAILURE);
            }
        } else {
            printf("The process of reading from pipes has failed\n");
            exit(EXIT_FAILURE);
        }
        break;
    }
    }
    exit(EXIT_SUCCESS);
}