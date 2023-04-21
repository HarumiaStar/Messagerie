#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>

/////////////////////////////////////////////////////////////////////////
    void EnvoiTailleMessage(int *adressetailleBuffer, int recepteur)
    {
        if (send(recepteur, adressetailleBuffer, sizeof(int), 0) == -1)
        {
            perror("Taille non envoyé\n");
            exit(1);
        }
        printf("Taille envoyée\n");
    }

    ////////////////////////////////////////////////////

    void EnvoiMessage(int tailleBuffer, char *message, int recepteur)
    {
        if (send(recepteur, message, tailleBuffer, 0) == -1)
        {
            perror("message non envoyé\n");
            exit(1);
        }
        printf("Message envoyé\n");
    }

    ////////////////////////////////////////////////////
    char* ReceptionMessage(int tailleBufferReception, int envoyeur)
    {
        char *r = malloc(tailleBufferReception*sizeof(char));
        if (recv(envoyeur, r, tailleBufferReception, 0) == -1) {
            perror("Réponse non reçue");
            exit(1);
        }
        printf("Réponse reçue : %s\n", r);
        return r;
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




int main(int argc, char *argv[])
{

    if (argc != 4)
    {
        perror("./client IPHost port ordre(0-1)");
        exit(1);
    }

    int client = atoi(argv[3]);

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

    // Variables:

    int tailleBuffer = 200;
    char message[tailleBuffer]; // le bufffer
    //* Fourre tout
    // strcpy(m,"TEST");

    /*char * w = "TEST";
    int nb = strlen(w);
    for (int i = 0; i < nb+1;i++){
      m[i] = w[i];
    }*/


    

    if (client == 0){
        do
        {
            
            printf(" Entrez votre message: \n");
            
            fgets(message, tailleBuffer, stdin); // on place le message dans le buffer
            if (message[strlen(message)-1] == '\n') 
                message[strlen(message)-1] = '\0';
            tailleBuffer = strlen(message) + 1;
            EnvoiTailleMessage(&tailleBuffer, dS);
            EnvoiMessage(tailleBuffer, message, dS);
            
            if (strcmp(message,"fin") == 0){
                printf("La fin haha :");
                break;
            }
            
            // Reception de la réponse:
            int tailleBufferReception = ReceptionTailleBuffer(dS);
            char* message2 = ReceptionMessage(tailleBufferReception, dS);

            if (strcmp(message2,"fin") == 0){
                printf("La fin Reception: ");
                break;
            }

            tailleBuffer = 200;

        } while (1);
    }
    else {
        
        do
        {
            // Reception de la réponse:
            int tailleBufferReception = ReceptionTailleBuffer(dS);
            char* message2 = ReceptionMessage(tailleBufferReception, dS);

            if (strcmp(message2,"fin") == 0){
                printf("La fin haha reception: ");
                break;
            }

            printf(" Entrez votre message: \n");
            fgets(message, tailleBuffer, stdin); // on place le message dans le buffer
            if (message[strlen(message)-1] == '\n') 
                message[strlen(message)-1] = '\0';
            tailleBuffer = strlen(message) + 1;
            EnvoiTailleMessage(&tailleBuffer, dS);
            EnvoiMessage(tailleBuffer, message, dS);

            tailleBuffer = 200;

            if (strcmp(message,"fin") == 0){
                printf("La fin haha envoie: ");
                break;
            }

        } while (1);
    }
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