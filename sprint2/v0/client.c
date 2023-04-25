#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>


#define tailleBufferMax  200
char mess[tailleBufferMax]; // le bufffer
typedef struct dSClient infos;

struct dSClient {
    int dSC;
    char* pseudo;
};

/*créer la struct*/
infos* creer_dSCClient(){
    infos* dSCli = (infos*) malloc(sizeof(int)*sizeof(char));
    (*dSCli).dSC = -1;
    (*dSCli).pseudo = (char*) malloc(tailleBufferMax * sizeof(char));
    return dSCli;
} 

/////////////////////////////////////////////////////////////////////////
    void EnvoiTailleMessage(int *adressetailleBuffer, int recepteur)
    {
        if (send(recepteur, adressetailleBuffer, sizeof(int), 0) == -1)
        {
            perror("Taille non envoyé\n");
            exit(1);
        }
        printf("Taille envoyée\n");
    }

    ////////////////////////////////////////////////////

    void EnvoiMessage(int tailleBuffer, char *message, int recepteur)
    {
        if (send(recepteur, message, tailleBuffer, 0) == -1)
        {
            perror("message non envoyé\n");
            exit(1);
        }
        printf("Message envoyé\n");
    }

    ////////////////////////////////////////////////////


    char* ReceptionMessage(int tailleBufferReception, int envoyeur)
    {
        char *r = malloc(tailleBufferReception*sizeof(char));
        if (recv(envoyeur, r, tailleBufferReception, 0) == -1) {
            perror("Réponse non reçue");
            exit(1);
        }
        printf("Réponse reçue : %s\n", r);
        return r;
    }

    ////////////////////////////////////////////////////

    int ReceptionTailleBuffer(int envoyeur)
    {
        int tailleBuffer;
        if (recv(envoyeur, &tailleBuffer, sizeof(int), 0) == -1)
        {
            perror("Taille non envoyé\n");
            exit(1);
        }
        printf("la taille %d\n", tailleBuffer);
        return tailleBuffer;
    }


void* envoie(void * args){
    infos* arg = (infos*) args;
    int dS = (long)arg->dSC;

    while (1){
        printf(" Entrez votre message: \n");
            
        fgets(mess, tailleBufferMax, stdin); // on place le message dans le buffer
        // A finir TODO
        char* texte = 
        strcat(" envoyé par : ",arg->pseudo);

        char* name = " (envoyé par : " + arg->pseudo + ")";
        strcat(name, mess);
        //// on veut envoyer les message + pseudo
        if (mess[strlen(mess)-1] == '\n') 
            mess[strlen(mess)-1] = '\0';
        int tailleBuffer = strlen(mess) + 1;

        EnvoiTailleMessage(&tailleBuffer, dS);
        EnvoiMessage(tailleBuffer, mess, dS);

        if (strcmp(mess,"fin") == 0){
            printf("La fin Reception: \n");
            exit(0);
        }
    }
}

void* reception(void * arg){
    int dS = (infos*)arg->dSC;
    while (1){
        int tailleBufferReception = ReceptionTailleBuffer(dS);
        char* mess2 = ReceptionMessage(tailleBufferReception, dS);

        if (strcmp(mess2,"fin") == 0){
            printf("La fin Reception: \n");
            exit(0);
        }
    }
}

int main(int argc, char *argv[])
{

    if (argc != 3)
    {
        perror("./client IPHost port");
        exit(1);
    }

    printf("Début programme\n");

    int dS = socket(PF_INET, SOCK_STREAM, 0);
    if (dS == -1)
    {
        perror(" Socket non créé");
        exit(1);
    }
    printf("Socket Créé\n");

    struct sockaddr_in aS;
    aS.sin_family = AF_INET;
    inet_pton(AF_INET, argv[1], &(aS.sin_addr));
    aS.sin_port = htons(atoi(argv[2]));
    socklen_t lgA = sizeof(struct sockaddr_in);
    if (connect(dS, (struct sockaddr *)&aS, lgA) == -1)
    {
        perror("Socket non connectée");
        exit(1);
    }
    printf("Socket Connecté\n");



    char ack[4];
    recv(dS, ack, sizeof(ack), 0);
    if (strcmp(ack, "ACK") != 0) {
        perror("Erreur lors de la synchronisation avec le serveur");
        exit(1);
    }


    infos* carte = creer_dSCClient();
    carte->dSC = dS;
    printf("Entrez votre pseudo:\n");
    fgets(carte->pseudo, tailleBufferMax, stdin);
    
    
    // Variables:
    pthread_t threadEnvoyeur;
    pthread_t threadRecepteur;

    pthread_create(&threadEnvoyeur, NULL, envoie, (void*) carte);
    pthread_create(&threadRecepteur, NULL, reception, (void*) carte);

    pthread_join(threadRecepteur, NULL);
    pthread_join(threadEnvoyeur, NULL);

    shutdown(carte->dSC, 2);
    printf("Fin du programme\n");
}