#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[])
{

    if (argc != 3)
    {
        perror("./client IPHost port");
        exit(1);
    }

    int tailleBuffer = 200;

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
    if (connect(dS, (struct sockaddr *)&aS, lgA))
    {
        perror("Socket non connectée");
        exit(1);
    }
    printf("Socket Connecté\n");

    // Envoi du premier message :

    char m[tailleBuffer]; // Le buffer

    // On get la taille du message en demandant le message à envoyer à l'utilisateur:
    printf(" Entrez votre message: \n");
    fgets(m, tailleBuffer, stdin); // on place le message dans le buffer
    tailleBuffer = strlen(m) + 1;  // on calcul la taille du message

    // on envoi la taille du message au serveur
    if (send(dS, &tailleBuffer, sizeof(int), 0) == -1)
    {
        perror("message non envoyé");
        exit(1);
    }
    // le serveur sait maintenant la taille du message qu'il va récupérer, on lui envoi donc le message
    if (send(dS, m, tailleBuffer, 0) == -1)
    {
        perror("message non envoyé");
        exit(1);
    }

    //* Fourre tout
    // strcpy(m,"TEST");

    /*char * w = "TEST";
    int nb = strlen(w);
    for (int i = 0; i < nb+1;i++){
      m[i] = w[i];
    }*/

    printf("Message Envoyé \n");

    char message[tailleBuffer]; // le bufffer

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
    void ReceptionMessage(int tailleBufferReception)
    {
        char *r;
        if (recv(dS, &r, sizeof(tailleBufferReception), 0) == -1)
        {
            perror("Réponse non reçue");
            exit(1);
        }
        printf("Réponse reçue : %s\n", r);
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

    do
    {
        printf(" Entrez votre message: \n");

        fgets(message, tailleBuffer, stdin); // on place le message dans le buffer
        tailleBuffer = strlen(message) + 1;
        EnvoiTailleMessage(&tailleBuffer);
        EnvoiMessage(tailleBuffer, message);

        // Reception de la réponse:
        int tailleBufferReception = ReceptionTailleBuffer();
        ReceptionMessage(tailleBufferReception);

    } while (message != "fin");

    /*
      // Envoi du premier message :
      int tailleBuffer2 = 200 ; // reinitialisation de la taille d'un buffer (jsp si nécessaire)

      char n[tailleBuffer2]; // le bufffer

      // On get la taille du message en demandant le message à envoyer à l'utilisateur:
      printf(" Entrez votre message: \n");
      fgets(n,tailleBuffer2,stdin); //on place le message dans le buffer
      tailleBuffer2 = strlen(n)+1; // on calcul la taille du message

      // on envoi la taille du message au serveur
      if(send(dS, &tailleBuffer2, sizeof(int) , 0) == -1){
        perror("message non envoyé");
        exit(1);
      }
      // le serveur sait maintenant la taille du message qu'il va récupérer, on lui envoi donc le message
      if(send(dS, n, tailleBuffer2 , 0) == -1){
        perror("message non envoyé");
        exit(1);
      }

      printf("Message 2 Envoyé \n");


      int r;
      if (recv(dS, &r, sizeof(int), 0) == -1){
        perror("Réponse non reçue");
        exit(1);
      }
      printf("Réponse reçue : %d\n", r) ;

      */

    shutdown(dS, 2);
    printf("Fin du programme\n");
}