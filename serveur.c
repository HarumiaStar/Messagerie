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

    struct sockaddr_in aC;
    socklen_t lg = sizeof(struct sockaddr_in);
    int dSC = accept(dS, (struct sockaddr *)&aC, &lg);

    if (dSC == -1)
    {
        perror("Client non conncté)");
        exit(1);
    }
    printf("Client Connecté\n");

// Nos Fonctions:

    /////////////////////////////////////////////////////////////////////////
    void EnvoiTailleMessage(int *tailleBuffer)
    {
        if (send(dS, &tailleBuffer, sizeof(int), 0) == -1)
        {
            perror("Taille non envoyé\n");
            exit(1);
        }
        printf("Taille envoyée\n");
    }

    ////////////////////////////////////////////////////

    void EnvoiMessage(int tailleBuffer, char *message)
    {
        if (send(dS, &message, tailleBuffer, 0) == -1)
        {
            perror("message non envoyé\n");
            exit(1);
        }
        printf("Message envoyé\n");
    }

    ////////////////////////////////////////////////////

    int ReceptionTailleBuffer()
    {
        int tailleBuffer;
        if (recv(dS, &tailleBuffer, sizeof(int), 0) == -1)
        {
            perror("Taille non envoyé\n");
            exit(1);
        }
        printf("la taille %d\n", tailleBuffer);
        return tailleBuffer;
    }

    ///////////////////////////////////////////////////////
    
    char* ReceptionMessage(int tailleBufferReception)
    {
        char *r;
        if (recv(dS, &r, sizeof(tailleBufferReception), 0) == -1)
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


    // Le code:
    while (1)
    {
        // Reception de la réponse client:
        int tailleBufferReception = ReceptionTailleBuffer();
        message = ReceptionMessage(tailleBufferReception);

        // Envoi au client 2
        EnvoiTailleMessage(&tailleBufferReception);
        EnvoiMessage(tailleBufferReception, message);

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

    shutdown(dSC, 2);
    shutdown(dS, 2);
    printf("Fin du programme\n");
}