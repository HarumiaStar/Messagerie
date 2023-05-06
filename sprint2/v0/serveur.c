#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>


sem_t semaphore; // declaration sem

typedef struct dsc dSClient;

struct dSClient {
    int* dSC;
    char* pseudo;
};

dSClient* tabClientStruct;

int* tabClient;
int tailleBufferReception;
int lengthTabClient;




int recherchePlace(int* tabClient, int taille){
    int i = 0;
    while(i < taille && tabClient[i] != 0){
        i++;
    }
    return i;
}

////////////////////////////////
int send_message(int client_fd, const void *message, size_t message_size) {
    // Vérifier si la socket est encore ouverte
    if (client_fd < 0) {
        printf("Erreur : la socket a été fermée\n");
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


/////////////////////////////////////////////////
int verif_commande(char* message) {
    char cmp = '@';
    if (message[0] == cmp) {
        printf("Ceci est une commande\n");
        return 1;
    }
    else {
        printf("C'est un message\n");
        return 0;
    }
}
//////////////////////////////////////////////////


// Regarde à qui on envoie le message privé et si la commande est la deconnexion
int commande(char* mess){
    if (verif_commande(mess) == 1){
        char* message = (char*) malloc(strlen(mess) * sizeof(char));
        strcpy(message, mess); // on copie le message pour pouvoir le modifier
        char* cmd = strtok(message, " "); // on isole @(truc)
        cmd = strtok(cmd, "@"); // on récup le (truc)
        
        //Si deconnexion:
        if (strncmp(cmd,"fin", 3) == 0) return -2; //on renvoie -2 si truc == fin
        //Si message privé
        else {
            return atoi(cmd); //l'index de la personne dans le tabClient 
        }
    }
    return -1; // pas une commande
}


void* communication(void* dSC){
    
    long index = (long)(intptr_t) dSC;

    while (1){

        // Reception de la taille de la réponse client Writer: 
        
        int tailleBufferReception;
        int recvTaille = recv(tabClient[index], &tailleBufferReception, sizeof(int), 0);
        if (recvTaille == 0) {
            printf("Client %ld déconnecté\n", index);
            break;
        }
        if (recvTaille == -1) {
            perror("Taille non reçue\n");
            break;
        }
        printf("la taille %d\n", tailleBufferReception);  


        // Reception message
        char* message1 = malloc(tailleBufferReception*sizeof(char));
        int recvMessage = recv(tabClient[index], message1, tailleBufferReception, 0);
        if(recvMessage == 0){
            printf("Client %ld déconnecté \n", index);
            break;
        }
        if(recvMessage == -1){
            perror("Réponse non reçue");
            break;
        }
        printf("Réponse reçue : %s\n", message1);

        
        // traitement message(message, taille, ...)

        long i = 0;
        int cmd = commande(message1); // cmd => à qui envoyer le message
        
        // Fin de communication où envoyer à tous
        if(cmd ==-2 || cmd == -1 ){
            // Propagation
            for(int i=0; i < lengthTabClient; i++){
                if(i == index || tabClient[i] == 0){
                    continue;
                }
                send_message(tabClient[i], message1, tailleBufferReception);
            }
            // Fin de communication 
            if (cmd == -2){
                printf("Client %ld déconnecté \n", i);
                break;
            }
        }
        
        // envoyer à 1 personne
        else if (cmd >= 0 && tabClient[cmd] != 0){
            send_message(tabClient[cmd], message1, tailleBufferReception);  
        }


    }
    tabClient[index] = 0;
    sem_post(&semaphore); // JE LIBERE LE JETON
    pthread_exit(NULL);
}




int main(int argc, char *argv[])
{

    if (argc != 3){
        perror("./serveur port nbcli");
        exit(1);
    }

    printf("Début programme\n");

    int dS = socket(PF_INET, SOCK_STREAM, 0);
    if (dS == -1){
        perror(" Socket non créé");
        exit(1);
    }
    printf("Socket Créé\n");

    struct sockaddr_in ad;
    ad.sin_family = AF_INET;
    ad.sin_addr.s_addr = INADDR_ANY;
    ad.sin_port = htons(atoi(argv[1]));
    if (bind(dS, (struct sockaddr *)&ad, sizeof(ad)) == -1){
        perror("Socket pas nommée");
        exit(1);
    }

    printf("Socket Nommé\n");

    if (listen(dS, 7) == -1){
        perror("Mode écoute non activé");
        exit(1);
    }
    printf("Mode écoute\n");


    int nbcli = atoi(argv[2]);
    sem_init(&semaphore, 0, nbcli); // nbcli = nbre de jetons dispos

    pthread_t* thread = (pthread_t*) malloc(nbcli*sizeof(int));
    free(tabClient);
    tabClient = malloc(nbcli * sizeof(int));

    lengthTabClient = nbcli;

    //initialisation du tableau à 0:

    for (int i = 0; i < nbcli; i++) {
        tabClient[i] = 0;
    }
    
    // Gestion des connexions
    while (1){
        
        for (int i = 0; i < nbcli; i++) {
            sem_wait(&semaphore); ///  JESSAYE RECUP JETON

            // des que j'en ai un je fais la suite
            int index = recherchePlace(tabClient, lengthTabClient);

            // Creer Clients :
            struct sockaddr_in aC;
            socklen_t lg = sizeof(struct sockaddr_in);
            int dSC = accept(dS, (struct sockaddr *)&aC, &lg);
            if (dSC == -1){
                printf("Client %d non connecté \n", i);
                i -= 1;
                continue;
            }
            printf("Client %d Connecté \n", i);

            char ack[] = "ACK";
            send(dSC, ack, strlen(ack) + 1, 0);

            //Stocke à la place libre et lancement thread d'ecoute 
            tabClient[index] = dSC; 
            pthread_create(&thread[index], NULL, communication, (void *)(intptr_t)index); 
        }
    }

    shutdown(dS, 2);
    printf("Fin du programme\n");
}