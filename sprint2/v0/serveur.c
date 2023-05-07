#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <pthread.h>
#include <semaphore.h>

typedef struct {
    int dSC;
    char pseudo[32];
} dSClient;

dSClient* tabClientStruct;
int tailleBufferReception;
int lengthTabClient;

typedef struct {
    long index;
    sem_t *semaphore;
} thread_args;

int send_message(int client_fd, const void *message, size_t message_size) {
    // Vérifier si la socket est encore ouverte
    if (client_fd < 0) {
        printf("Erreur : la socket a été fermée (client %d) \n", client_fd);
        return -1;
    }

    // Envoie taille
    int sendTaille = send(client_fd, &message_size, sizeof(int), 0);
    if (sendTaille == 0){
        printf("Client déconnecté (sendTaille)\n");
        return -1;
    }
    if (sendTaille == -1){
        perror("Taille non envoyée\n");
        return -1;
    }
    printf("Taille envoyée\n");

    // Envoie message
    int sendMessage = send(client_fd, message, message_size, 0);
    if (sendMessage == 0){
        printf("Client déconnecté (sendMessage)\n");
        return -1;
    }
    if (sendMessage == -1){
        perror("Message non envoyé\n");
        return -1;
    }
    printf("Message envoyé\n");

    return 0;
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


void* communication(void* arg){

    thread_args *args = (thread_args *) arg;
    long index = args->index;
    sem_t *semaphore = args->semaphore;

    while (1){

        // Reception de la réponse client Writer: 
        int recvTaille = recv(tabClientStruct[index].dSC, &tailleBufferReception, sizeof(int), 0);
       if (recvTaille == 0) {
            printf("Client %ld déconnecté (recvTaille =0) \n", index);
            tabClientStruct[index].dSC = -1;
            strncpy(tabClientStruct[index].pseudo, "__DISCONNECTED__", sizeof(tabClientStruct[index].pseudo));
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
            printf("Client %ld déconnecté (recvMessage = 0) \n", index);
            strncpy(tabClientStruct[index].pseudo, "__DISCONNECTED__", sizeof(tabClientStruct[index].pseudo));
            pthread_exit(NULL);
        }
        if(recvMessage == -1){
            perror("Réponse non reçue");
            pthread_exit(NULL);
        }
        printf("Réponse reçue : %s\n", message1);

        long i = 0;
        int sortie = 0;
        while (i < lengthTabClient && sortie == 0){
            if (i != index){
                int cmd = commande(message1);
                if (cmd == -1){ // On envoie le message à tout le monde
                    if (strncmp(tabClientStruct[i].pseudo, "__DISCONNECTED__", 15) != 0) {
                        int sended = send_message(tabClientStruct[i].dSC, message1, tailleBufferReception);
                        if (sended == -1){
                            printf("Erreur lors de l'envoi du message à %s\n", tabClientStruct[i].pseudo);
                        }
                    }
                }
                else if (cmd == -2) { // Déconnecter l'utilisateur actuel uniquement
                    printf("Déconnexion du client %ld\n", index);
                    printf("Client %s déconnecté (fin) \n", tabClientStruct[index].pseudo);
                    close(tabClientStruct[index].dSC);
                    tabClientStruct[index].dSC = -1;
                    sortie = 1;
                }
                else if (cmd == -3) { // Pas de client trouvé pour ce pseudo
                    char* message = "Pas de client trouvé pour ce pseudo";
                    int sended = send_message(tabClientStruct[index].dSC, message, strlen(message));
                    if (sended == -1){
                        printf("Erreur lors de l'envoi du message à %s\n", tabClientStruct[i].pseudo);
                    }
                    break;

                } else if (cmd == i){ // On envoie le message privé
                    int sended = send_message(tabClientStruct[i].dSC, message1, tailleBufferReception);
                    if (sended == -1){
                        printf("Erreur lors de l'envoi du message à %s\n", tabClientStruct[i].pseudo);
                    }
                }
            }
            i+=1;
        }
        if (sortie == 1) {
            strncpy(tabClientStruct[index].pseudo, "__DISCONNECTED__", sizeof(tabClientStruct[index].pseudo));
            break;
        }
    }
    sem_post(semaphore);
    pthread_exit(NULL);
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
    tabClientStruct = malloc(nbcli * sizeof(dSClient));

    lengthTabClient = nbcli;

    // Ajout du sémaphore
    sem_t semaphore;
    sem_init(&semaphore, 0, nbcli);

    for (int i = 0; i < nbcli; i++){
        tabClientStruct[i].dSC = -1;
        strcpy(tabClientStruct[i].pseudo, "__DISCONNECTED__");
    }
    
    // Le code:
    while (1){
        int i = 0;
        while (i < nbcli) {
            // Recherche d'un emplacement vide dans le tableau tabClientStruct
            int availableIndex = -1;
            for (int j = 0; j < nbcli; j++) {
                printf("Client %d : %s\n", j, tabClientStruct[j].pseudo);
                if (strncmp(tabClientStruct[j].pseudo, "__DISCONNECTED__", 15) == 0) {
                    availableIndex = j;
                    break;
                }
            }

            // Si aucun emplacement vide n'est trouvé, utilisez l'indice suivant
            if (availableIndex == -1) {
                availableIndex = i;
            } else {
                i = availableIndex;
            }

            // Ajoutez une variable booléenne pour vérifier si un emplacement disponible a été trouvé
            int availableSlotFound = -1;
            if (availableIndex == i) {
                availableSlotFound = 1;
            }

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

            if (strncmp(pseudo, "fin", 3) == 0 || pseudoDejaUtilise(pseudo, tabClientStruct, nbcli) == 1){
                char* messagePseudo = "Pseudo déjà utilisé ou pseudo invalide";
                send_message(dSC, messagePseudo, strlen(messagePseudo));

                // envoyer @fin au client pour qu'il se déconnecte (avec la taille)
                char* messageFin = "@fin";
                send_message(dSC, messageFin, strlen(messageFin));
                               
                tabClientStruct[i].dSC = -1;
                strncpy(tabClientStruct[i].pseudo, "__DISCONNECTED__", sizeof(tabClientStruct[i].pseudo));
                i--;
                continue;
            }

            // Mise à jour de l'indice pour les opérations suivantes
            i = availableIndex;

            dSClient* d = (dSClient*) malloc(sizeof(dSClient));
            d->dSC = dSC;
            strncpy(d->pseudo, pseudo, sizeof(d->pseudo));

            tabClientStruct[i] = *d;

            // Ajout de l'attente du sémaphore et la création du thread avec les arguments
            sem_wait(&semaphore);
            thread_args *args = (thread_args *) malloc(sizeof(thread_args));
            args->index = i;
            args->semaphore = &semaphore;
            pthread_create(&thread[i], NULL, communication, (void *)args);

            // Si un emplacement disponible a été trouvé, n'incrémentez pas i
            if (availableSlotFound == -1) {
                i++;
            }
        }
    }
    shutdown(dS, 2);

    sem_destroy(&semaphore);
    printf("Fin du programme\n");
}
