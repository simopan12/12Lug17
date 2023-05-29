/* Soluzione della Prova d'esame del 12 Luglio 2017 - Parte C */
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <time.h>

typedef int pipe_t[2];
typedef struct
{
    int pid_nipote;        /* campo c1 del testo */
    int numero_linea;      /* campo c2 del testo */
    char linea_letta[250]; /* campo c3 del testo */
    /* bastano 250 caratteri per contenere ogni riga insieme con il terminatore di linea e con il terminatore di stringa (vedi specifica) */
} strut;

int mia_random(int n)
{ /* funzione che calcola un numero random compreso fra 1 e n (fornita nel testo) */
    int casuale;
    casuale = rand() % n;
    casuale++;
    return casuale;
}

int main(int argc, char **argv)
{
    /* -------- Variabili locali ---------- */
    int pid;         /* process identifier per le fork() */
    int N;           /* numero di file passati sulla riga di comando (uguale al numero di numeri) */
    int status;      /* variabile di stato per la wait */
    pipe_t *piped;   /* array dinamico di pipe descriptors per comunicazioni figli-padre  */
    pipe_t p;        /* una sola pipe per ogni coppia figlio-nipote */
    int i, j;        /* indici per i cicli */
    int X, r;        /* variabili usate dal nipote per il numero di linee (sicuramente strettamente minore di 255, garantito dalla parte shell) e per il valore random (anche lui < di 255) */
    int nr;          /* variabile usata dal padre per controllo sulla read dai figli */
    char opzione[5]; /* variabile stringa che serve per passare al comando head l'opzione con il numero delle linee da stampare */
                     /* supponiamo che bastino 3 cifre per il numero, oltre al simbolo di opzione '-' e al terminatore di stringa */
    int ritorno;     /* variabile che viene ritornata da ogni figlio al padre e che contiene il numero random calcolato dal nipote */
    strut S;         /* struttura usata dai figli e dal padre */
    int finito;      /* variabile che serve al padre per sapere se non ci sono piu' strutture da leggere */
                     /* ------------------------------------ */

    /* Controllo sul numero di parametri */
    if (argc < 3 || (argc - 1) % 2) /* Meno di due parametri e non pari */
    {
        printf("Errore nel numero dei parametri dato che argc=%d\n", argc);
        exit(1);
    }

    /* Calcoliamo il numero di file/numeri passati */
    N = (argc - 1) / 2;

    /* Controlliamo i numeri passati: non usiamo ora la funzione atoi() per trasformare queste stringhe in numeri */
    /* dato che i numeri saranno convertiti dai nipoti. Quindi controlliamo il primo carattere (quello di indice 0) di ogni stringa numerica */
    /* e se e' il carattere '-' oppure '0' vuol dire che il numero non e' strettamente positivo e diamo errore */
    for (i = 0; i < N; i++)
    {
        /* le stringhe che rappresentano i numeri si trovano con l'indice i moltiplicato per 2 e sommando 2, cioe' nelle posizioni 2, 4, 6, etc. e quindi saltando le stringhe che rappresentano i nomi dei file */
        if (argv[(i * 2) + 2][0] == '-' || argv[(i * 2) + 2][0] == '0')
        {
            printf("Errore %s non rappresenta un numero strettamente positivo \n", argv[(i * 2) + 2]);
            exit(2);
        }
    }

    printf("DEBUG-Numero processi da creare %d\n", N);

    /* Allocazione dell'array di N pipe descriptors */
    piped = (pipe_t *)malloc(N * sizeof(pipe_t));
    if (piped == NULL)
    {
        printf("Errore nella allocazione della memoria\n");
        exit(3);
    }

    /* Creazione delle N pipe figli-padre */
    for (i = 0; i < N; i++)
    {
        if (pipe(piped[i]) < 0)
        {
            printf("Errore nella creazione della pipe\n");
            exit(4);
        }
    }

    /* Le N pipe nipoti-figli deriveranno dalla creazione di una pipe in ognuno dei figli che poi creeranno un nipote */

    /* Ciclo di generazione dei figli */
    for (i = 0; i < N; i++)
    {
        if ((pid = fork()) < 0)
        {
            printf("Errore nella fork\n");
            exit(5);
        }

        if (pid == 0)
        {
            /* codice del figlio */
            /* in caso di errori nei figli o nei nipoti decidiamo di tornare -1 che corrispondera' al valore 255 che non puo' essere un valore accettabile di ritorno (questo e' garantito dalla parte SHELL */

            /* Chiusura delle pipe non usate nella comunicazione con il padre */
            for (j = 0; j < N; j++)
            {
                close(piped[j][0]);
                if (i != j)
                    close(piped[j][1]);
            }

            /* prima creiamo la pipe di comunicazione fra nipote e figlio */
            if (pipe(p) < 0)
            {
                printf("Errore nella creazione della pipe nipote-figlio\n");
                exit(-1); /* si veda commento precedente */
            }

            if ((pid = fork()) < 0) /* ogni figlio crea un nipote */
            {
                printf("Errore nella fork di creazione del nipote\n");
                exit(-1); /* si veda commento precedente */
            }
            if (pid == 0)
            {
                /* codice del nipote */
                /* in caso di errori nei figli o nei nipoti decidiamo di tornare -1 che corrispondera' al valore 255 che non puo' essere un valore accettabile di ritorno */
                srand(time(NULL)); /* inizializziamo il seme per la generazione random di numeri (come indicato nel testo) */
                                   /* il nipote calcola il numero di linee del suo file e quindi calcola in modo random il numero di linee che inviera' al figlio */
                X = atoi(argv[(i * 2) + 2]);
                r = mia_random(X);
                /* Costruiamo la stringa di opzione per il comando head  */
                sprintf(opzione, "-%d", r);
                /* chiusura della pipe rimasta aperta di comunicazione fra figlio-padre che il nipote non usa */
                close(piped[i][1]);
                /* ogni nipote deve simulare il piping dei comandi nei confronti del figlio e quindi deve chiudere lo standard output e quindi usare la dup sul lato di scrittura della propria pipe */
                close(1);
                dup(p[1]);
                /* ogni nipote adesso puo' chiudere entrambi i lati della pipe: il lato 0 NON viene usato e il lato 1 viene usato tramite lo standard ouput */
                close(p[0]);
                close(p[1]);
                /* Il nipote diventa il comando head: bisogna usare le versioni dell'exec con la p in fondo in modo da usare la variabile di ambiente PATH  */ /* le stringhe che rappresentano i nomi dei file si trovano con l'indice i moltiplicato per 2 e sommando 1, cioe' nelle posizioni 1, 3, 5, etc. e quindi saltando le stringhe che rappresentano i numeri*/
                execlp("head", "head", opzione, argv[(i * 2) + 1], (char *)0);
                /* attenzione ai parametri nella esecuzione di head: opzione, nome del file e terminatore della lista */
                /* NOTA BENE: non avendo operato l'implementazione della ridirezione dello standard input, ma passando il nome del file come parametro, se per caso il file NON esiste o non fosse leggibile, il comando head fallirebbe e non riuscirebbe a mandare alcuna linea al figlio e quindi il valore 0 tornato dal figlio dovrebbe essere interpretato come problemi appunto nel nipote; in alternativa, per catturare questo problema si potrebbe implementare la ridireazione dello standard input e far terminare con exit(-1) il nipote se la open non va a buon fine, ma in questo caso bisognerebbe che il figlio verificasse il valore di ritorno del nipote e nel caso di -1/255 tornare questo valore al padre! */

                /* Non si dovrebbe mai tornare qui!!*/
                /* usiamo perror che scrive su standard error, dato che lo standard output e' collegato alla pipe */
                perror("Problemi di esecuzione della head da parte del nipote");
                exit(-1); /* si veda commento precedente */
            }
            /* codice figlio */
            /* chiusura lato della pipe nipote-figlio non usato */
            close(p[1]);
            /* adesso il figlio legge dalla pipe fino a che ci sono caratteri e cioe' linee inviate dal nipote tramite la head */
            j = 0;              /* inizializziamo l'indice della linea */
            ritorno = 0;        /* inizializziamo il numero di linee lette che alla fine rappresentera' il numero random calcolato dal nipote */
            S.pid_nipote = pid; /* inizializziamo il campo con il pid del processo nipote */
            while (read(p[0], &(S.linea_letta[j]), 1))
            {
                /* se siamo arrivati alla fine di una linea */
                if (S.linea_letta[j] == '\n')
                {
                    /* si deve incrementare il valore di ritorno che rappresenta anche il numero di linea */
                    ritorno++;
                    /* nell'array linea_letta abbiamo l'ultima linea ricevuta: incrementiamo la sua lunghezza per inserire il terminatore di stringa. Infatti ... */
                    j++;
                    S.linea_letta[j] = '\0'; /* poiche' il padre deve stampare la linea su standard output, decidiamo che il figlio converta tale linea in una stringa per facilitare il compito del padre */
                    S.numero_linea = ritorno;
                    /* il figlio comunica al padre */
                    write(piped[i][1], &S, sizeof(S));
                    j = 0; /* riazzeriamo l'indice per la prossima linea */
                }
                else
                    j++; /* incrementiamo l'indice della linea */
            }

            /* il figlio deve aspettare il nipote per correttezza */
            if ((pid = wait(&status)) < 0)
            {
                printf("Errore in wait\n");
                exit(-1); /* si veda commento precedente */
            }
            if ((status & 0xFF) != 0)
            {
                printf("Nipote con pid %d terminato in modo anomalo\n", pid);
                exit(-1); /* si veda commento precedente */
            }
            else
                /* questa stampa non e' richiesta dal testo */
                /* printf("Il nipote con pid=%d ha ritornato %d\n", pid, (int)((status >> 8) & 0xFF)); */
                exit(ritorno); /* il figlio ritorna il numero di linee ricevute dal nipote che rappresentano implicitamente il numero random calcolato dal nipote, come richiesto dal testo */
        }
    }

    /* Codice del padre */
    /* Il padre chiude i lati delle pipe che non usa */
    for (i = 0; i < N; i++)
        close(piped[i][1]);

    /* Il padre recupera le informazioni dai figli: prima in ordine di strutture e quindi in ordine di indice */
    finito = 0; /* all'inizio supponiamo che non sia finito nessun figlio */
    while (!finito)
    {
        finito = 1; /* mettiamo finito a 1 perche' potrebbero essere finiti tutti i figli */
        for (i = 0; i < N; i++)
        {
            /* si legge la struttura inviata  dal figlio i-esimo */
            nr = read(piped[i][0], &S, sizeof(S));
            if (nr != 0)
            {
                finito = 0;                                                                                                                                          /* almeno un processo figlio non e' finito */
                                                                                                                                                                     /* Nota bene: la stampa della linea con il formato %s NON ha problemi perche' il figlio ha passato una stringa */
                printf("Il nipote con pid %d ha letto dal file %s nella riga %d questa linea:\n%s", S.pid_nipote, argv[(i * 2) + 1], S.numero_linea, S.linea_letta); /* NOTA BENE: dato che la linea contiene il terminatore di linea nella printf NON abbiamo inserito lo \n alla fine */
            }
        }
    }

    /* Il padre aspetta i figli */
    for (i = 0; i < N; i++)
    {
        if ((pid = wait(&status)) < 0)
        {
            printf("Errore in wait\n");
            exit(6);
        }

        if ((status & 0xFF) != 0)
            printf("Figlio con pid %d terminato in modo anomalo\n", pid);
        else
        {
            ritorno = (int)((status >> 8) & 0xFF);
            printf("Il figlio con pid=%d ha ritornato %d (se 255 problemi nel figlio o nel nipote; se 0 problemi nel nipote)\n", pid, ritorno);
        }
    }

    exit(0);
}