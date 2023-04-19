#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int tabClient[2];


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


        //Nos variables:
        int tailleBuffer = 200;
        int tailleBufferReception;
        char *message;
        
        tabClient[0] = dSC1;
        tabClient[1] = dSC2;
        int indexWriter = 0;
        int indexSending = 1;

        
        
        while (1)
        {
            // Reception de la réponse client Writer:
            printf("%d\n", dSC1);

            
            if (recv(tabClient[indexWriter], &tailleBufferReception, sizeof(int), 0) == -1)
            {
                perror("Taille non envoyé\n");
                dSC1 = NULL;
                dSC2 = NULL;
                break;
            }
            printf("la taille %d\n", tailleBufferReception);
            

            // Reception message
            char* message1 = malloc(tailleBufferReception*sizeof(char));
            if (recv(tabClient[indexWriter], message1, tailleBufferReception, 0) == -1)
            {
                perror("Réponse non reçue");
                dSC1 = NULL;
                dSC2 = NULL;
                break;
            }
            printf("Réponse reçue : %s\n", message1);


            // Envoi au client 2 Sending
            if (send(tabClient[indexSending], &tailleBufferReception, sizeof(int), 0) == -1)
            {
                perror("Taille non envoyé\n");
                dSC1 = NULL;
                dSC2 = NULL;
                break;
            }
            printf("Taille envoyée\n");

            // Envoie Message
            if (send(tabClient[indexSending], message1, tailleBufferReception, 0) == -1)
            {
                perror("message non envoyé\n");
                dSC1 = NULL;
                dSC2 = NULL;
                break;
            }
            printf("Message envoyé\n");


            if (strcmp(message1, "fin") == 0){
                dSC1 = NULL;
                dSC2 = NULL;
                printf("Clients déconnectés\n");
                break;
            }

            free(message1);


            /////////////////


            // Reception de la réponse client Writer:
            printf("%d\n", dSC2);

            
            if (recv(tabClient[indexSending], &tailleBufferReception, sizeof(int), 0) == -1)
            {
                perror("Taille non envoyé\n");
                dSC1 = NULL;
                dSC2 = NULL;
                break;
            }
            printf("la taille %d\n", tailleBufferReception);
            


            char* message2 = malloc(tailleBufferReception*sizeof(char));
            if (recv(tabClient[indexSending], message2, tailleBufferReception, 0) == -1)
            {
                perror("Réponse non reçue");
                dSC1 = NULL;
                dSC2 = NULL;
                break;
            }
            printf("Réponse reçue : %s\n", message2);


            // Envoi au client 2 Sending
            if (send(tabClient[indexWriter], &tailleBufferReception, sizeof(int), 0) == -1)
            {
                perror("Taille non envoyé\n");
                dSC1 = NULL;
                dSC2 = NULL;
                break;
            }
            printf("Taille envoyée\n");

            // Envoie Message
            if (send(tabClient[indexWriter], message2, tailleBufferReception, 0) == -1)
            {
                perror("message non envoyé\n");
                dSC1 = NULL;
                dSC2 = NULL;
                break;
            }
            printf("Message envoyé\n");

            if (strcmp(message2, "fin") == 0){
                dSC1 = NULL;
                dSC2 = NULL;
                printf("Clients déconnectés\n");
                break;
            }

            free(message2);           
            
        }
    }

    shutdown(dS, 2);
    printf("Fin du programme\n");
}

