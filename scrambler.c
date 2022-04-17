#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>
#include <limits.h>

#define D_ARG 

// проверка выделения памяти
void check_mem(void * ptr){
    if (ptr == NULL) {
        perror("Memory problem");
        exit(EXIT_FAILURE);
    }
}

char** split_args(char * str) {
    int i = 0, cur_arg_num = 1;
    char** splitted = (char**)malloc(cur_arg_num * sizeof(char *));
    splitted[i] = strtok(str, " ");
    while (splitted[i] != NULL){
        ++i;
        if (i == cur_arg_num){
            cur_arg_num += 1;
            splitted = (char**)realloc(splitted, cur_arg_num * sizeof(char *));
            check_mem((void*)splitted);
        }
        splitted[i] = strtok(NULL, " ");
    }  
    return splitted;
}

int main(int argc, char* argv[]){
    if (argc == 2 && strcmp(argv[1], "--help") == 0) {
        printf("Right input: ./scrambler \"command1\" \"command2\" {output_file}\n");
        exit(EXIT_SUCCESS);
    }
    if (argc != 4) {
        printf("Wrong number of arguments!\n");
        exit(EXIT_FAILURE);
    }
    char command[BUFSIZ];
    char** input1, **input2;
    int id, pipe4orig_desc[2], pipe4key_desc[2], output_desc, st;
    size_t spaces_n1 = 0, spaces_n2 = 0;
    if (pipe(pipe4key_desc) == -1 || pipe(pipe4orig_desc) == -1) {
        printf("Pipe opening failure\n");
        exit(EXIT_FAILURE);
    }
    id = fork();
    switch (id)
    {
    case -1:
        printf("Fork in main process broke\n");
        exit(EXIT_FAILURE);
    case 0:
    {
        int id2 = fork();
        close(pipe4key_desc[0]);
        close(pipe4orig_desc[0]);
        switch (id2)
        {
        case -1:
            printf("Fork in subprocess broke\n");
            exit(EXIT_FAILURE);
        case 0: //original
            {
            close(pipe4key_desc[0]);
            close(pipe4key_desc[1]);
            if (dup2(pipe4orig_desc[1], STDOUT_FILENO) < 0) {
                close(pipe4orig_desc[1]);
                fprintf(stderr, "Duplication of didn't work for original\n");
                exit(EXIT_FAILURE);
            }
            input1 = split_args(argv[1]);
            return execv(input1[0], input1);
            close(pipe4orig_desc[1]);
            exit(EXIT_FAILURE);
            }
        default: //key
            close(pipe4orig_desc[0]);
            close(pipe4orig_desc[1]);
            close(pipe4key_desc[0]);
            if (dup2(pipe4key_desc[1], STDOUT_FILENO) < 0) {
                close(pipe4key_desc[1]);
                fprintf(stderr, "Duplication didn't work for key\n");
                exit(EXIT_FAILURE);
            }
            input2 = split_args(argv[2]);
            return execv(input2[0], input2);
            close(pipe4key_desc[1]);
            exit(EXIT_FAILURE); //key
        }
        break;
    }
    }
    close(pipe4orig_desc[1]);
    close(pipe4key_desc[1]);
    size_t read_key, read_orig;
    char key_buf[PIPE_BUF], orig_buf[PIPE_BUF];
    if ((output_desc = open(argv[3], O_WRONLY | O_CREAT | O_TRUNC, S_IWUSR | S_IRUSR)) < 0) {
        printf("Output file could not be opened\n");
        exit(EXIT_FAILURE);
    }
    size_t key_size = 0, orig_size = 0;
    size_t off_key = 0, off_orig = 0;
    while ((key_size = read(pipe4key_desc[0], key_buf + off_key, PIPE_BUF - off_key)) > 0 && (orig_size = read(pipe4orig_desc[0], orig_buf + off_orig, PIPE_BUF - off_orig)) > 0) {
        key_size += off_key;
        orig_size += off_orig;
        if (key_size == orig_size) {
            for (int i = 0; i < key_size; ++i) key_buf[i] ^= orig_buf[i];
            write(output_desc, key_buf, key_size);
            key_size = 0;
            orig_size = 0;
            off_key = 0;
            off_orig = 0;
        } else {
            off_key = key_size;
            off_orig = off_orig;
        }
    }
    if(key_size != orig_size){
        close(pipe4orig_desc[0]);
        close(pipe4key_desc[0]);
        close(output_desc);
        printf("Oops, the key seems to be smaller than the original");
        exit(EXIT_FAILURE);
    }
    //free(orig_buf);
    //free(key_buf);
    free(input1);
    free(input2);
    close(pipe4orig_desc[0]);
    close(pipe4key_desc[0]);
    if ((0 > wait(&st) || WEXITSTATUS(st) == EXIT_FAILURE) 
    || (0 > wait(&st) || WEXITSTATUS(st) == EXIT_FAILURE)) {
        printf("Something went wrong: one of commands was corrupted\n");
        exit(EXIT_FAILURE);
    }
    close(output_desc);
    exit(EXIT_SUCCESS);
}