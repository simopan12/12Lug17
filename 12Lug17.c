#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <time.h>

struct struttura
{
    int c1;
    int c2;
    char c3[250];
};

typedef int pipe_t[2];

int mia_random(int n)
{
    int casuale;
    casuale = rand() % n;
    casuale++;
    return casuale;
}

int main(int argc, char const *argv[])
{
    if (argc < 5 || (argc - 1) % 2 != 0)
    {
        printf("ERRORE NELL'INSERIMENTO DEI PARAMETRI RICHIESTI\n");
        exit(1);
    }

    int pid;
    int ritorno, status, pidFiglio;
    int *pidNipote;
    int N;
    int i, j;
    int r;
    int X;
    char comando[10];
    int p[2];
    int x, z;
    bool finito;
    int nr;
    N = (argc - 1) / 2;

    pidNipote = malloc(N * sizeof(int));
    pipe_t *pipeFiglioPadre;
    /*allocazione pipe figli-padre*/
    if ((pipeFiglioPadre = (pipe_t *)malloc(N * sizeof(pipe_t))) == NULL)
    {
        printf("Errore allocazione pipe padre\n");
        exit(4);
    }
   
    /* creazione pipe di comunicazione tra figlio e padre*/
    for (i = 0; i < N; i++)
    {
        if (pipe(pipeFiglioPadre[i]) < 0)
        {
            printf("Errore creazione pipe\n");
            exit(6);
        }
    }
    /* creazione di altre N pipe di comunicazione/sincronizzazione con il padre */
    
    for (i = 0; i < N; i ++)
    {
        if ((pid = fork()) < 0)
        {
            printf("Errore nella creazione\n");
            exit(2);
        }

        if (pid == 0)
        { // codice del figlio

            for (j = 0; j < N; j++)
            {
                /* il figlio non legge da nessuna pipeFiglioPadre e non scrive su nessuna pipePadreFiglio */
                close(pipeFiglioPadre[j][0]);
                if (j != i)
                { /* inoltre non scrive e non legge se non su/dalle sue pipe */
                    close(pipeFiglioPadre[j][1]);
                }
            }

            if (pipe(p) < 0)
            {
                printf("Errore di comunicazione della pipe tra figlio e nipote\n");
                exit(10);
            }

            if ((pidNipote[i] = fork()) < 0)
            {
                printf("Errore nella creazione del processo nipote\n");
                exit(2);
            }
            r=0;
            if (pidNipote[i] == 0)
            { // codice del nipote

                pidNipote[i]=getpid();
                X = atoi(argv[(i*2) + 2]);

                srand(time(NULL));

                r = mia_random(X);

                sprintf(comando, "-%d", r);
                close(pipeFiglioPadre[i][0]);
                close(1);
                dup(p[1]);
                close(p[0]);
                close(p[1]);
                execlp("head", "head", comando, argv[(i*2) + 1], (char *)0);

                perror("Problemi di esecuzione della head da parte del nipote");
				exit(-1);
            }

            close(p[1]);
            
            
            z = 0;
            x=1;
            struct struttura S;
            while (read(p[0],&S.c3[z],1))
            {
                if(S.c3[z]=='\n'){
                    S.c3[z]='\0';
                    S.c1=pidNipote[i];
                    S.c2=x;
                    write(pipeFiglioPadre[i][1],&S,sizeof(S));
                    x++;
                    z=0;
                }else{
                    z++;
                }
                
            }

            /*figlio aspetta nipote*/
            if ((pidFiglio = wait(&status)) < 0)
            {
                printf("Errore in wait\n");
                exit(-1);
            }
            if ((status & 0xFF) != 0)
            {
                printf("Nipote con pid %d terminato in modo anomalo\n", pidFiglio);
                exit(-1);
            }

            exit(r);
        }
    }



    /* chiusura pipe */
    for (i = 0; i < N; i++)
    { /* il padre non legge da nessuna pipePadreFiglio e non scrive su nessuna pipeFiglioPadre */
        close(pipeFiglioPadre[i][1]);
    }
    finito = false;
    while (!finito)
    {
        finito = true;
        for (i = 0; i < N; i++)
        {
            struct struttura S;
            nr=read(pipeFiglioPadre[i][0], &S, sizeof(struct struttura));
            if(nr!=0)
            {
                finito = false;
                printf("il figlio n°%d, ha comunicato tramite il nipote con pid %d, la struct contenente la linea n°%d:\n%s\n", i, S.c1, S.c2, S.c3);
            }
            
        }
    }

    for (i = 0; i < N; i++)
    {
        if ((pidFiglio = wait(&status)) < 0)
        {
            printf("Errore in wait\n");
            exit(10);
        }

        if ((status & 0xFF) != 0)
            printf("Figlio con pid %d terminato in modo anomalo\n", pidFiglio);
        else
        {
            ritorno = (int)((status >> 8) & 0xFF);
            printf("Il figlio con pid=%d ha ritornato il valore %d (se 255 problemi)\n", pidFiglio, ritorno);
        }
    }
    exit(0);
}
