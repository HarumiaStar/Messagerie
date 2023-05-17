#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <dirent.h>
#include <sys/types.h>
#include <unistd.h>


#define tailleBufferMax  200
char mess[tailleBufferMax]; // le bufffer

typedef struct {
    int dSC;
    char* pseudo;
} dSClient;
char* argv1;
int dS;
int dSF;
//promis jurer il y aura ces fonctions
void EnvoiMessage(int, char*, int);
void EnvoiTailleMessage(int *, int);

/*créer la struct*/
dSClient* creer_dSCClient(){
    dSClient* dSCli = (dSClient*) malloc(sizeof(dSClient));
    (*dSCli).dSC = -1;
    (*dSCli).pseudo = (char*) malloc(tailleBufferMax * sizeof(char));
    return dSCli;
} 

// Gestionnaire de signaux
void sig_handler(int sig) {
	printf("\nSIGINT attrapé, on stop le programme %i\n", getpid());
    char* texte = "@fin";
    int tailleBuffer = strlen(texte) + 1;
    EnvoiTailleMessage(&tailleBuffer, dS);
    EnvoiMessage(tailleBuffer, texte, dS);
    exit(0);
}

int verif_commande(char* message) {
    char cmp = '@';
    if (message[0] == cmp) {
        return 1;
    }
    else {
        return 0;
    }
}

int commande(char* mess){
    if (verif_commande(mess) == 1){
        char* message = (char*) malloc(strlen(mess) * sizeof(char));
        strcpy(message, mess); // on copie le message pour pouvoir le modifier
        char* cmd = strtok(message, " "); // on isole @(truc)
        cmd = strtok(cmd, "@"); // on récup l'(truc)

        if (strncmp(cmd,"fin", 3) == 0) return 0; //on renvoie 0 si truc == fin
        if (strncmp(cmd,"sendFile",7) == 0)return -2; // on renvoie -2 si truc == sendFile
        
    }
    return -1;
}

void sendFile(){
    
    // Allocations memoires
    char* fileToSend = (char*)malloc(50*sizeof(char));
    char* chemin_fichier = (char*)malloc(150*sizeof(char));


    printf("Quel fichier voulez vous envoyer?\n");
    fgets(fileToSend, 50, stdin);

    if (fileToSend[strlen(fileToSend)-1] == '\n') 
            fileToSend[strlen(fileToSend)-1] = '\0';

    // Creation du chemin du fichier
    strcpy(chemin_fichier,"./filesClient/");
    strcat(chemin_fichier,fileToSend);

    printf("Chemin: %s\n",chemin_fichier);

    // ouverture du fichier
    FILE *fichier = fopen(chemin_fichier, "r");
    if (fichier == NULL){
        perror("Erreur lors de l'ouverture du fichier");
        return;
    }
    // envoi code commande fichier au serveur: (previent le serveur qu'on va lui envoyer un fichier)
    char* messCom = (char*)malloc(50*sizeof(char));
    strcpy(messCom,"@file");
    
    //envoi taille de messCom:
    int a = 5;
    int taille= send(dS,&a,sizeof(int),0);
    if(taille ==-1){
        printf("ERRROR\n");
    }
    
    //envoi du messCom
    int envoi = send(dS,messCom, sizeof(char)*5,0);
    if(envoi == -1){
        printf("erreur\n");
    }
    printf("on a envoyé la commande: %s \n", messCom);

    //Connexion socket sendFile
    dSF = socket(PF_INET, SOCK_STREAM, 0);
    if (dSF == -1)
    {
        perror(" Socket non créé");
        exit(1);
    }
    printf("Socket Créé\n");

    struct sockaddr_in aSF;
    aSF.sin_family = AF_INET;
    inet_pton(AF_INET, argv1, &(aSF.sin_addr));
    aSF.sin_port = htons(4000);
    socklen_t lgAF = sizeof(struct sockaddr_in);
    if (connect(dSF, (struct sockaddr *)&aSF, lgAF) == -1)
    {
        perror("Socket non connectée");
        exit(1);
    }
    printf("Socket Connecté\n");

    char ackF[4];
    recv(dSF, ackF, sizeof(ackF), 0);
    if (strcmp(ackF, "ACK") != 0) {
        perror("Erreur lors de la synchronisation avec le serveur");
        exit(1);
    }
    // envoi nom fichier au serveur 
    int sended = send(dSF, fileToSend,strlen(fileToSend),0);
    if (sended == -1){
        printf("Errreur lors de l'envoi du nom de fichier\n");
        
    }

    // calcule de la taille du fichier 
    int fsize;
    fseek(fichier, 0, SEEK_END);

    fsize = ftell(fichier);
    rewind(fichier);// remise du curseur de lecture à 0

    printf("%d\n",fsize);

    // envoi taille du fichier au serveur
    int sending = send(dSF,&fsize,sizeof(int),0 );
    if (sending == -1){
        printf("Errreur lors de l'envoi de la taille du fichier\n");
        
    }
    
    char* buffer = malloc(500*sizeof(char));
    // send le fichier par bouts
    int bytes_sent=0;
    int bytes_read=0;
    while ((bytes_read = fread(buffer, 1,500, fichier) )!= 0){
        int sended = send(dSF, buffer,bytes_read,0);
        if (sended == -1){
            printf("Erreur lors de l'envoi du fichier\n");
        }
        bytes_sent+=sended;
        printf("%d\n",bytes_sent);
    }
    close(dSF);
    fclose(fichier);
    free(buffer);
    free(messCom);
    
}

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


void* envoie(void * args){
    dSClient* arg = (dSClient*) args;
    int dS = (long)arg->dSC;

    while (1){
        printf(" Entrez votre message: \n");
            
        fgets(mess, tailleBufferMax, stdin); // on place le message dans le buffer
        
        char* texte = malloc(tailleBufferMax*sizeof(char*));
        char* name = malloc(tailleBufferMax*sizeof(char*));
        strcpy(name, " - envoyé par : "); // texte de rajout après message
        char* pseu = malloc(tailleBufferMax*sizeof(char*));
        strcpy(pseu, arg->pseudo);
        
        strcpy( texte, mess );
        strcat(name, pseu);
        strcat(texte, name );

        //// on veut envoyer les message + pseudo
        if (texte[strlen(texte)-1] == '\n') 
            texte[strlen(texte)-1] = '\0';
        int tailleBuffer = strlen(texte) + 1;
        printf("taille du message : %d\n", tailleBuffer);

        if (mess[strlen(mess)-1] == '\n') 
            mess[strlen(mess)-1] = '\0';

        //Commande pour lister et choisir un fichier à envoyer au serveur (les fichier du repertoire "filesClient")
        if (commande(mess) == -2){
            printf("Voici la liste de vos fichiers:");
            DIR *dir;
            struct dirent *entry;

            dir = opendir("./filesClient"); // Ouvre le repertoire "filesClient"
            if (dir == NULL) {
                perror("Erreur lors de l'ouverture du repertoire");
                continue; 
            }
            while ((entry = readdir(dir)) != NULL) { // Parcourt les fichiers
                printf("%s\n", entry->d_name); // Affiche le nom du fichier
            }
            
            closedir(dir); //on ferme le repertoire
            sendFile(); // envoi du fichier au serveur
    
            continue;
        }

        


        EnvoiTailleMessage(&tailleBuffer, dS);
        EnvoiMessage(tailleBuffer, texte, dS);

        if (commande(mess) == 0){
            printf("La fin Reception: \n");
            exit(0);
        }


    }
}

void* reception(void * args){
    dSClient* arg = (dSClient*) args;
    long dS = (long)arg->dSC;
    while (1){
        int tailleBufferReception = ReceptionTailleBuffer(dS);
        char* mess2 = ReceptionMessage(tailleBufferReception, dS);

        if (commande(mess2) == 0){
            printf("La fin Reception: \n");
            exit(0);
        }
    }
}



int main(int argc, char *argv[])
{

    // Enregistrement du gestionnaire de signaux
	if(signal(SIGINT, sig_handler) == SIG_ERR){
		puts("Erreur à l'enregistrement du gestionnaire de signaux !");
	}

    if (argc != 3)
    {
        perror("./client IPHost port");
        exit(1);
    }
    argv1 = argv[1];
    printf("Début programme\n");

    dS = socket(PF_INET, SOCK_STREAM, 0);
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
    if (connect(dS, (struct sockaddr *)&aS, lgA) == -1)
    {
        perror("Socket non connectée");
        exit(1);
    }
    printf("Socket Connecté\n");



    char ack[4];
    recv(dS, ack, sizeof(ack), 0);
    if (strcmp(ack, "ACK") != 0) {
        perror("Erreur lors de la synchronisation avec le serveur");
        exit(1);
    }


    dSClient* carte = creer_dSCClient();
    carte->dSC = dS;
    printf("Entrez votre pseudo:\n");
    fgets(carte->pseudo, tailleBufferMax, stdin);
    printf("pseudo : %s\n", carte->pseudo);

    if (send(dS, carte->pseudo, strlen(carte->pseudo), 0) == -1){
        perror("pseudo non envoyé\n");
        exit(1);
    }
    printf("pseudo envoyé\n");
    
    
    // Variables:
    pthread_t threadEnvoyeur;
    pthread_t threadRecepteur;

    pthread_create(&threadEnvoyeur, NULL, envoie, (void*) carte);
    pthread_create(&threadRecepteur, NULL, reception, (void*) carte);

    pthread_join(threadRecepteur, NULL);
    pthread_join(threadEnvoyeur, NULL);

    shutdown(carte->dSC, 2);
    printf("Fin du programme\n");
}