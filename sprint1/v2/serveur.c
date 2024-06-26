#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <pthread.h>


int* tabClient;
int tailleBufferReception;
int lenghTabClient;


void* communication(void* dSC){

    long index = (long)(intptr_t) dSC;

    while (1){

        // Reception de la réponse client Writer:
        int recvTaille = recv(tabClient[index], &tailleBufferReception, sizeof(int), 0);
        if (recvTaille == 0){
            printf("Client 1 déconnecté\n");
            pthread_exit(NULL);
        }
        if (recvTaille == -1){
            perror("Taille non reçue\n");
            exit(1);
        }
        printf("la taille %d\n", tailleBufferReception);
        

        // Reception message
        char* message1 = malloc(tailleBufferReception*sizeof(char));
        int recvMessage = recv(tabClient[index], message1, tailleBufferReception, 0);
        if(recvMessage == 0){
            printf("Client %d déconnecté \n", index);
            pthread_exit(NULL);
        }
        if(recvMessage == -1){
            perror("Réponse non reçue");
            exit(1);
        }
        printf("Réponse reçue : %s\n", message1);

        long i = 0;
        while ( i < lenghTabClient){
            if (i != index){
                // Envoi au client
                int sendTaille = send(tabClient[i], &tailleBufferReception, sizeof(int), 0);
                if (sendTaille == 0){
                    printf("Client %d déconnecté \n", i);
                    pthread_exit(NULL);
                }
                if (sendTaille == -1){
                    perror("Taille non envoyé\n");
                    exit(1);
                }
                printf("Taille envoyée\n");

                // Envoie Message
                int sendMessage = send(tabClient[i], message1, tailleBufferReception, 0);
                if (sendMessage == 0){
                    printf("Client %d déconnecté \n", i);
                    pthread_exit(NULL);
                }
                if (sendMessage == -1){
                    perror("message non envoyé\n");
                    exit(1);
                }
                printf("Message envoyé\n");
            }
            i+=1;
        }
    }

}

int verfifieDSC(){
    for (int i = 0; i < lenghTabClient; i++ ){
        printf("Je suis le client : %d à l'index : %d \n", tabClient[i], i);
        if (tabClient[i] == -1) return -1;
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

    lenghTabClient = nbcli;
    
    // Le code:

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

        tabClient[i] = dSC;
    }

    for (int i = 0; i < lenghTabClient; i++ ){
        pthread_create(&thread[i], NULL, communication, (void *)(intptr_t)i); // sendTo // receiveFrom
    }
        

    for (int i = 0; i < lenghTabClient; i++ ){
        pthread_join(thread[i], NULL);
    }

    shutdown(dS, 2);
    printf("Fin du programme\n");
}
