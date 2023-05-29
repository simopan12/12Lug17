#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>

typedef int pipe_t[2];

int main(int argc, char const *argv[])
{
    if(argc<5 || (argc-1)%2!=0){
        printf("ERRORE NELL'INSERIMENTO DEI PARAMETRI RICHIESTI\n");
        exit(1);
    }
    
    int pid;
    int ritorno,status,pidFiglio,pidNipote;
    int fd;
    int N;
    int i;

    N=(argc-1)/2;

    

    return 0;
}
