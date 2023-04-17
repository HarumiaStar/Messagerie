#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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
        perror("Client 2 non connzcté)");
        exit(1);
    }
    printf("Client 2 Connecté\n");


// Nos Fonctions depuis le serveur:

    /////////////////////////////////////////////////////////////////////////
    void EnvoiTailleMessage(int *tailleBuffer,int recepteur)
    {
        if (send(recepteur, &tailleBuffer, sizeof(int), 0) == -1)
        {
            perror("Taille non envoyé\n");
            exit(1);
        }
        printf("Taille envoyée\n");
    }

    ////////////////////////////////////////////////////

    void EnvoiMessage(int tailleBuffer, char *message, int recepteur)
    {
        if (send(recepteur, &message, tailleBuffer, 0) == -1)
        {
            perror("message non envoyé\n");
            exit(1);
        }
        printf("Message envoyé\n");
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

    ///////////////////////////////////////////////////////
    
    char* ReceptionMessage(int tailleBufferReception, int envoyeur)
    {
        char* r;
        if (recv(envoyeur, &r, sizeof(tailleBufferReception), 0) == -1)
        {
            perror("Réponse non reçue");
            exit(1);
        }
        printf("Réponse reçue : %s\n", r);
        return r;
    }


    //Nos variables:
    int tailleBuffer = 200;
    char message[tailleBuffer];
    
    int tabClient[2] = {dSC1,dSC2};
    int indexWriter = 0;
    int indexSending = 1;

    // Le code:
    while (1)
    {
        // Reception de la réponse client Writer:
        int tailleBufferReception = ReceptionTailleBuffer(tabClient[indexWriter]);
        strcpy(message, ReceptionMessage(tailleBufferReception, tabClient[indexWriter])) ;

        // Envoi au client 2 Sending
        EnvoiTailleMessage(&tailleBufferReception, tabClient[indexSending]);
        EnvoiMessage(tailleBufferReception, message, tabClient[indexSending]);


        // Changement des variables Writer/Sending:

        if (indexWriter == 0){
            indexSending = 0;
            indexWriter = 1;
        }

        else{
            indexWriter = 0;
            indexSending = 1;
        }
        
    }
    /*
    // Message 1 ////////////////////////////////////////////////////////////////

    if (recv(dSC, &tailleBuffer, sizeof(int), 0) == -1)
    {
        perror("Taille non envoyé\n");
        exit(1);
    }
    printf("la taille %d\n", tailleBuffer);
    char *msg = malloc(tailleBuffer);
    int nb2 = recv(dSC, msg, tailleBuffer, 0);
    if (nb2 == -1)
    {
        perror("message non reçu");
        exit(1);
    }

    printf("Message reçu : %s\n", msg);

    // Fin message 1 /////////////////////////////////////////////////////////

    // Message 2 //////////////////////////////////////////
    int tailleBuffer2;
    if (recv(dSC, &tailleBuffer2, sizeof(int), 0) == -1)
    {
        perror("Taille non envoyé\n");
        exit(1);
    }
    printf("la taille %d\n", tailleBuffer2);

    char msg2[tailleBuffer2];
    int nb3 = recv(dSC, msg2, sizeof(msg2), 0);
    if (nb3 == -1)
    {
        perror("message non reçu");
        exit(1);
    }
    printf("Message 2 reçu : %s\n", msg2);

    // Fin message 2 /////////////////////////////////////


*/
// Je sais pas à quoi ça sert :
/*    int r = 10;

    if (send(dSC, &r, sizeof(int), 0) == -1)
    {
        perror("message non envoyé\n");
        exit(1);
    }
    printf("Message Envoyé\n");
*/
//
    shutdown (dSC1, 2);
    shutdown(dSC2, 2);
    shutdown(dS, 2);
    printf("Fin du programme\n");
}