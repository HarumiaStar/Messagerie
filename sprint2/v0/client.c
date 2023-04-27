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

int verif_commande(char* message) {
    char cmp = '@';
    printf("message[0] = %c \n", message[0]);
    if (message[0] == cmp) {
        printf("Ceci est une commande\n");
        return 1;
    }
    else {
        printf("C'est un message\n");
        return 0;
    }
}

int commande(char* mess){
    if (verif_commande(mess) == 1){
        char* message = (char*) malloc(strlen(mess) * sizeof(char));
        strcpy(message, mess); // on copie le message pour pouvoir le modifier
        char* cmd = strtok(message, " "); // on isole @(truc)
        cmd = strtok(cmd, "@"); // on récup l'(truc)
        printf("cmd %s \n", cmd);

        printf(" strcmp : %d \n", strncmp(cmd,"fin", 3));
        if (strncmp(cmd,"fin", 3) == 0) return 0; //on renvoie 0 si truc == fin
    }
    return -1;
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
        char* texte = malloc(tailleBufferMax*sizeof(char*));
        char* name = malloc(tailleBufferMax*sizeof(char*));
        strcpy(name, " - envoyé par : "); // texte de rajout après message
        char* pseu = malloc(tailleBufferMax*sizeof(char*));
        strcpy(pseu, arg->pseudo);
        
        strcpy( texte, mess );
        strcat(name, pseu);
        strcat(texte, name );

        //// on veut envoyer les message + pseudo
        if (texte[strlen(texte)-1] == '\n') 
            texte[strlen(texte)-1] = '\0';
        int tailleBuffer = strlen(texte) + 1;

        if (mess[strlen(mess)-1] == '\n') 
            mess[strlen(mess)-1] = '\0';

        printf("mess %s", mess);

        EnvoiTailleMessage(&tailleBuffer, dS);
        EnvoiMessage(tailleBuffer, texte, dS);

        if (commande(mess) == 0){
            printf("La fin Reception: \n");
            exit(0);
        }
    }
}

void* reception(void * args){
    infos* arg = (infos*) args;
    long dS = (long)arg->dSC;
    while (1){
        int tailleBufferReception = ReceptionTailleBuffer(dS);
        char* mess2 = ReceptionMessage(tailleBufferReception, dS);

        if (commande(mess2) == 0){
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