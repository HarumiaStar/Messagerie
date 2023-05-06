#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <pthread.h>

typedef struct {
    int dSC;
    char pseudo[32];
} dSClient;

int* tabClient;
dSClient* tabClientStruct;
int tailleBufferReception;
int lengthTabClient;


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

// Regarde à qui on envoie le message privé
int commande(char* mess){
    if (verif_commande(mess) == 1){
        char* message = (char*) malloc(strlen(mess) * sizeof(char));
        strcpy(message, mess); // on copie le message pour pouvoir le modifier
        char* cmd = strtok(message, " "); // on isole @(truc)
        cmd = strtok(cmd, "@"); // on récup l'(truc)
        printf("cmd %s \n", cmd);

        printf(" strcmp : %d \n", strncmp(cmd,"fin", 3));
        if (strncmp(cmd,"fin", 3) == 0) return -2; //on renvoie -2 si truc == fin
        else {
            printf("Message privé ...\n");
            char target_pseudo[32]; // Assurez-vous de définir une longueur maximale pour les pseudos

            if (sscanf(message, "@%s\n", target_pseudo) != 1) {
                printf("Erreur de lecture du pseudo\n");
            }

            int target_index = -3;
            for (int i = 0; i < lengthTabClient; ++i) {
                printf("%s ?= %s\n", tabClientStruct[i].pseudo, target_pseudo);
                if (strcmp(tabClientStruct[i].pseudo, target_pseudo) == 0) {
                    target_index = i;
                    break;
                }
            }
            printf("Message privé à %s dont l'id est %d\n", target_pseudo, target_index);
            return target_index; //on renvoie si truc => index
        }
    }
    return -1; // pas une commande
}


void* communication(void* dSC){

    long index = (long)(intptr_t) dSC;

    while (1){

        // Reception de la réponse client Writer: 
        int recvTaille = recv(tabClientStruct[index].dSC, &tailleBufferReception, sizeof(int), 0);
        if (recvTaille == 0){
            printf("Client %ld déconnecté\n", index);
            pthread_exit(NULL);
        }
        if (recvTaille == -1){
            perror("Taille non reçue\n");
            pthread_exit(NULL);
        }
        printf("la taille %d\n", tailleBufferReception);
        

        // Reception message
        char* message1 = malloc(tailleBufferReception*sizeof(char));
        int recvMessage = recv(tabClientStruct[index].dSC, message1, tailleBufferReception, 0);
        if(recvMessage == 0){
            printf("Client %ld déconnecté \n", index);
            pthread_exit(NULL);
        }
        if(recvMessage == -1){
            perror("Réponse non reçue");
            pthread_exit(NULL);
        }
        printf("Réponse reçue : %s\n", message1);

        long i = 0;
        while ( i < lengthTabClient){
            if (i != index){
                int cmd = commande(message1);
                if (cmd == -1 || cmd == -2){
                    // Envoi au client
                    int sendTaille = send(tabClientStruct[i].dSC, &tailleBufferReception, sizeof(int), 0);
                    if (sendTaille == 0){
                        printf("Client %ld déconnecté (sendTaille)\n", i);
                        pthread_exit(NULL);
                    }
                    if (sendTaille == -1){
                        perror("Taille non envoyé\n");
                        pthread_exit(NULL);
                    }
                    printf("Taille envoyée\n");

                    // Envoie Message
                    int sendMessage = send(tabClientStruct[i].dSC, message1, tailleBufferReception, 0);
                    if (sendMessage == 0){
                        printf("Client %ld déconnecté (sendMessage)\n", i);
                        pthread_exit(NULL);
                    }
                    if (sendMessage == -1){
                        perror("message non envoyé\n");
                        pthread_exit(NULL);
                    }
                    printf("Message envoyé\n");
                }
                else if (cmd == -3) { // Pas de client trouvé pour ce pseudo
                    // Envoi au client
                    int sendTaille = send(tabClientStruct[index].dSC, &tailleBufferReception, sizeof(int), 0);
                    if (sendTaille == 0){
                        printf("Client %ld déconnecté (sendTaille)\n", index);
                        pthread_exit(NULL);
                    }
                    if (sendTaille == -1){
                        perror("Taille non envoyé\n");
                        pthread_exit(NULL);
                    }
                    printf("Taille envoyée\n");

                    // Envoie Message
                    char* message = "Pas de client trouvé pour ce pseudo";
                    int sendMessage = send(tabClientStruct[index].dSC, message, strlen(message), 0);
                    if (sendMessage == 0){
                        printf("Client %ld déconnecté (sendMessage)\n", index);
                        pthread_exit(NULL);
                    }
                    if (sendMessage == -1){
                        perror("message non envoyé\n");
                        pthread_exit(NULL);
                    }
                    printf("Message envoyé\n");
                    break;

                } else if (cmd == i){
                    printf("on entre\n");
                    // Envoi au client
                    int sendTaille = send(tabClientStruct[i].dSC, &tailleBufferReception, sizeof(int), 0);
                    if (sendTaille == 0){
                        printf("Client %ld déconnecté (sendTaille)\n", i);
                        pthread_exit(NULL);
                    }
                    if (sendTaille == -1){
                        perror("Taille non envoyé\n");
                        pthread_exit(NULL);
                    }
                    printf("Taille envoyée\n");

                    // Envoie Message
                    int sendMessage = send(tabClientStruct[i].dSC, message1, tailleBufferReception, 0);
                    if (sendMessage == 0){
                        printf("Client %ld déconnecté (sendMessage)\n", i);
                        pthread_exit(NULL);
                    }
                    if (sendMessage == -1){
                        perror("message non envoyé\n");
                        pthread_exit(NULL);
                    }
                    printf("Message envoyé\n");
                }
            }
            i+=1;
        }
    }

}

//  fonction pseudo déjà utilisé
int pseudoDejaUtilise(char* pseudo, dSClient* tabClientStruct, int nbcli){
    for (int i = 0; i < nbcli; i++){
        if (strcmp(pseudo, tabClientStruct[i].pseudo) == 0){
            return 1;
        }
    }
    return 0;
}

int main(int argc, char *argv[])
{

    if (argc != 3)
    {
        perror("./serveur port nbcli");
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

    struct sockaddr_in ad;
    ad.sin_family = AF_INET;
    ad.sin_addr.s_addr = INADDR_ANY;
    ad.sin_port = htons(atoi(argv[1]));
    if (bind(dS, (struct sockaddr *)&ad, sizeof(ad)) == -1)
    {
        perror("Socket pas nommée");
        exit(1);
    }

    printf("Socket Nommé\n");

    if (listen(dS, 7) == -1)
    {
        perror("Mode écoute non activé");
        exit(1);
    }
    printf("Mode écoute\n");


    int nbcli = atoi(argv[2]);
    pthread_t* thread = (pthread_t*) malloc(nbcli*sizeof(int));
    free(tabClient);
    tabClient = malloc(nbcli * sizeof(int));
    tabClientStruct = malloc(nbcli * sizeof(dSClient));

    lengthTabClient = nbcli;
    
    // Le code:
    while (1){

        for (int i = 0; i < nbcli; i++) {
            // Clients :
            struct sockaddr_in aC;
            socklen_t lg = sizeof(struct sockaddr_in);
            int dSC = accept(dS, (struct sockaddr *)&aC, &lg);
            if (dSC == -1)
            {
                printf("Client %d non connecté \n", i);
                i -= 1;
                continue;
            }
            printf("Client %d Connecté \n", i);

            char ack[] = "ACK";
            send(dSC, ack, strlen(ack) + 1, 0);

            // Reception pseudo
            char* pseudo = malloc(32*sizeof(char));
            int recvPseudo = recv(dSC, pseudo, 32, 0);
            if(recvPseudo == 0){
                printf("Client %ld déconnecté \n", index);
                i--;
            }
            if(recvPseudo == -1){
                perror("Pseudo non reçue");
                i--;
            }
            printf("Pseudo client %ld : %s\n", dSC, pseudo);

            if (pseudo[strlen(pseudo)-1] == '\n') 
            pseudo[strlen(pseudo)-1] = '\0';

            if (strcmp(pseudo, "fin") == 0 || pseudoDejaUtilise(pseudo, tabClientStruct, nbcli) == 1){
                char* message = "Pseudo déjà utilisé ou pseudo invalide";
                int tailleMessage = strlen(message);
                int sendTaille = send(dSC, &tailleMessage, sizeof(int), 0);
                if (sendTaille == 0){
                    printf("Client %ld déconnecté (sendTaille)\n", i);
                    pthread_exit(NULL);
                }
                if (sendTaille == -1){
                    perror("Taille non envoyé\n");
                    pthread_exit(NULL);
                }
                printf("Taille envoyée\n");

                // Envoie Message
                int sendMessage = send(dSC, message, strlen(message), 0);
                if (sendMessage == 0){
                    printf("Client %ld déconnecté (sendMessage)\n", i);
                    pthread_exit(NULL);
                }
                if (sendMessage == -1){
                    perror("message non envoyé\n");
                    pthread_exit(NULL);
                }
                printf("Message envoyé\n");

                // envoyer @fin au client pour qu'il se déconnecte (avec la taille)
                int tailleFin = strlen("@fin");
                int sendTailleFin = send(dSC, &tailleFin, sizeof(int), 0);
                if (sendTailleFin == 0){
                    printf("Client %ld déconnecté (sendTailleFin)\n", i);
                    pthread_exit(NULL);
                }
                if (sendTailleFin == -1){
                    perror("TailleFin non envoyé\n");
                    pthread_exit(NULL);
                }
                printf("TailleFin envoyée\n");

                // Envoie Message
                int sendMessageFin = send(dSC, "@fin", strlen("@fin"), 0);
                if (sendMessageFin == 0){
                    printf("Client %ld déconnecté (sendMessageFin)\n", i);
                    pthread_exit(NULL);
                }
                if (sendMessageFin == -1){
                    perror("messageFin non envoyé\n");
                    pthread_exit(NULL);
                }
                printf("MessageFin envoyé\n");
                               
                i--;
                continue;
            }

            dSClient* d = (dSClient*) malloc(sizeof(dSClient));
            d->dSC = dSC;
            strncpy(d->pseudo, pseudo, sizeof(d->pseudo));

            tabClientStruct[i] = *d;
        }

        for (int i = 0; i < lengthTabClient; i++ ){
            pthread_create(&thread[i], NULL, communication, (void *)(intptr_t)i); 
        }
            

        for (int i = 0; i < lengthTabClient; i++ ){
            pthread_join(thread[i], NULL);
        }
    }

    shutdown(dS, 2);
    printf("Fin du programme\n");
}
