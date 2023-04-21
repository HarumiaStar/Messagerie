#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <pthread.h>


int tabClient[2];
int tailleBufferReception;


void* client1_vers_client2(){

    while (1){

        // Reception de la réponse client Writer:
        int recvTaille = recv(tabClient[0], &tailleBufferReception, sizeof(int), 0);
        if (recvTaille == 0){
            printf("Client 1 déconnecté\n");
            pthread_exit(NULL);
        }
        if (recvTaille == -1){
            perror("Taille non envoyé\n");
            exit(1);
        }
        printf("la taille %d\n", tailleBufferReception);
        

        // Reception message
        char* message1 = malloc(tailleBufferReception*sizeof(char));
        int recvMessage = recv(tabClient[0], message1, tailleBufferReception, 0);
        if(recvMessage == 0){
            printf("Client 1 déconnecté\n");
            pthread_exit(NULL);
        }
        if(recvMessage == -1){
            perror("Réponse non reçue");
            exit(1);
        }
        printf("Réponse reçue : %s\n", message1);

        // Envoi au client 2 Sending
        int sendTaille = send(tabClient[1], &tailleBufferReception, sizeof(int), 0);
        if (sendTaille == 0){
            printf("Client 2 déconnecté\n");
            pthread_exit(NULL);
        }
        if (sendTaille == -1){
            perror("Taille non envoyé\n");
            exit(1);
        }
        printf("Taille envoyée\n");

        // Envoie Message
        int sendMessage = send(tabClient[1], message1, tailleBufferReception, 0);
        if (sendMessage == 0){
            printf("Client 2 déconnecté\n");
            pthread_exit(NULL);
        }
        if (sendMessage == -1){
            perror("message non envoyé\n");
            exit(1);
        }
        printf("Message envoyé\n");

    }

}



void* client2_vers_client1(){

    while (1) {
        // Reception message
        int recvTaille = recv(tabClient[1], &tailleBufferReception, sizeof(int), 0);
        if (recvTaille == 0){
            printf("Client 2 déconnecté\n");
            pthread_exit(NULL);
        }
        if (recvTaille == -1){
            perror("Taille non envoyé\n");
            exit(1);
        }
        printf("la taille %d\n", tailleBufferReception);
        
        char* message2 = malloc(tailleBufferReception*sizeof(char));
        int recvMessage = recv(tabClient[1], message2, tailleBufferReception, 0);
        if (recvMessage == 0){
            printf("Client 2 déconnecté\n");
            pthread_exit(NULL);
        }
        if (recvMessage == -1){
            perror("Réponse non reçue");
            exit(1);
        }
        printf("Réponse reçue : %s\n", message2);

        // Envoi au client 1 
        int sendTaille = send(tabClient[0], &tailleBufferReception, sizeof(int), 0);
        if (sendTaille == 0){
            printf("Client 1 déconnecté\n");
            pthread_exit(NULL);
        }
        if (sendTaille == -1){
            perror("Taille non envoyé\n");
            exit(1);
        }
        printf("Taille envoyée\n");

        // Envoie Message
        int sendMessage = send(tabClient[0], message2, tailleBufferReception, 0);
        if (sendMessage == 0){
            printf("Client 1 déconnecté\n");
            pthread_exit(NULL);
        }
        if (sendMessage == -1){
            perror("message non envoyé\n");
            exit(1);
        }
        printf("Message envoyé\n");

        free(message2);          
    }

}


int main(int argc, char *argv[])
{

    if (argc != 2)
    {
        perror("./serveur port");
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


    pthread_t thread[2];
    
    // Le code:

    while(1){

        // Premier Client:
        struct sockaddr_in aC;
        socklen_t lg1 = sizeof(struct sockaddr_in);
        int dSC1 = accept(dS, (struct sockaddr *)&aC, &lg1);

        if (dSC1 == -1)
        {
            perror("Client 1 non connecté)");
            exit(1);
        }
        printf("Client 1 Connecté\n");


        // Deuxième Client:
        struct sockaddr_in aC2;
        socklen_t lg2 = sizeof(struct sockaddr_in);
        int dSC2 = accept(dS, (struct sockaddr *)&aC2, &lg2);
        if (dSC2 == -1)
        {
            perror("Client 2 non connecté)");
            exit(1);
        }
        printf("Client 2 Connecté\n");

        tabClient[0] = dSC1;
        tabClient[1] = dSC2;


        if( dSC1 != -1 && dSC2 != -1){

            pthread_create (&thread[0], NULL, client1_vers_client2, NULL); // sendTo // receiveFrom
            pthread_create (&thread[1], NULL, client2_vers_client1, NULL); // sendTo // receiveFrom

            pthread_join(thread[0], NULL);
            pthread_join(thread[1], NULL);

        }    
        
    }

    shutdown(dS, 2);
    printf("Fin du programme\n");
}
