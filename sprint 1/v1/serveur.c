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
        if (recv(tabClient[0], &tailleBufferReception, sizeof(int), 0) == -1)
        {
            perror("Taille non envoyé\n");
            exit(1);
        }
        printf("la taille %d\n", tailleBufferReception);
        

        // Reception message
        char* message1 = malloc(tailleBufferReception*sizeof(char));
        if (recv(tabClient[0], message1, tailleBufferReception, 0) == -1)
        {
            perror("Réponse non reçue");
            exit(1);
        }
        printf("Réponse reçue : %s\n", message1);


        // Envoi au client 2 Sending
        if (send(tabClient[1], &tailleBufferReception, sizeof(int), 0) == -1)
        {
            perror("Taille non envoyé\n");
            exit(1);
        }
        printf("Taille envoyée\n");

        // Envoie Message
        if (send(tabClient[1], message1, tailleBufferReception, 0) == -1)
        {
            perror("message non envoyé\n");
            exit(1);
        }
        printf("Message envoyé\n");

    }

}



void* client2_vers_client1(){

    while (1) {
        // Reception message
        if (recv(tabClient[1], &tailleBufferReception, sizeof(int), 0) == -1)
        {
            perror("Taille non envoyé\n");
            exit(1);
        }
        printf("la taille %d\n", tailleBufferReception);
        
        char* message2 = malloc(tailleBufferReception*sizeof(char));
        if (recv(tabClient[1], message2, tailleBufferReception, 0) == -1)
        {
            perror("Réponse non reçue");
            exit(1);
        }
        printf("Réponse reçue : %s\n", message2);


        // Envoi au client 2 
        if (send(tabClient[0], &tailleBufferReception, sizeof(int), 0) == -1)
        {
            perror("Taille non envoyé\n");
            exit(1);
        }
        printf("Taille envoyée\n");

        // Envoie Message
        if (send(tabClient[0], message2, tailleBufferReception, 0) == -1)
        {
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


        if( dSC1 != NULL && dSC2 != NULL){

            pthread_create (&thread[0], NULL, client1_vers_client2, (void *)0); // sendTo // receiveFrom
            pthread_create (&thread[1], NULL, client2_vers_client1, (void *)1); // sendTo // receiveFrom

            pthread_join(thread[0], NULL);
            pthread_join(thread[1], NULL);

        }    
        
    }

    shutdown(dS, 2);
    printf("Fin du programme\n");
}


