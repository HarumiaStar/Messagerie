#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <ctype.h> 
#include <sys/types.h>

#define TAILLE_PSEUDO 32

#define TAILLE_NOM_SALON 50
#define TAILLE_DESCRIPTION_SALON 180

typedef struct {
    char nomSalon[TAILLE_NOM_SALON];
    char descriptionSalon[TAILLE_DESCRIPTION_SALON];
} salon;

typedef struct {
    int idSalon;
    int dSC;
    char pseudo[TAILLE_PSEUDO];
} dSClient;

dSClient* tabClientStruct;
int tailleBufferReception;
int lengthTabClient;

typedef struct {
    long index;
} thread_args;

salon* tabSalons;
int nbSalons;

pthread_mutex_t mutex;
sem_t semaphore;
int dSF;
int dSlistF;
char* tabFichiers[100];

void deconnecterClient(int index) {
    pthread_mutex_lock(&mutex);
    printf("Client %d déconnecté (recvTaille =0) \n", index);
    tabClientStruct[index].dSC = -1;
    strncpy(tabClientStruct[index].pseudo, "__DÉCONNECTÉ__", TAILLE_PSEUDO);
    tabClientStruct[index].idSalon = 0;
    pthread_mutex_unlock(&mutex);
}



int send_message(int client_fd, const void *message, size_t message_size) {
    // Vérifier si la socket est encore ouverte
    if (client_fd < 0) {
        printf("Erreur : la socket a été fermée (client %d) \n", client_fd);
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

// Gestionnaire de signaux
void sig_handler(int sig) {
	printf("\nSIGINT attrapé, on stop le programme %i\n", getpid());
    for (int i = 0; i < lengthTabClient; ++i) {
        if (tabClientStruct[i].dSC != -1) {
            char* texte = "@fin Serveur";
            send_message(tabClientStruct[i].dSC, texte, strlen(texte));
        }
    }
    exit(0);
}
 
void getFile(int index){

    // Reception du nom du fichier 
    printf("Reception du nom du fichier\n");
    char* chemin_fichier = (char*)malloc(150*sizeof(char));
    char* nom_fichier = malloc(50*sizeof(char));

    pthread_mutex_lock(&mutex);
    int recvMessage = recv(tabClientStruct[index].dSC, nom_fichier, 50, 0);
    pthread_mutex_unlock(&mutex);

    if(recvMessage == 0){deconnecterClient(index); free(nom_fichier); return;}
    if(recvMessage == -1){perror("Réponse non reçue"); free(nom_fichier); return;}

    printf("Réponse reçue : %s\n", nom_fichier);

    nom_fichier[strlen(nom_fichier)] = '\0';

    if (nom_fichier[strlen(nom_fichier)-1] == '\n') // On enlève le \n de la chaine
            nom_fichier[strlen(nom_fichier)-1] = '\0';
    
    size_t len = strlen(nom_fichier);
    int i = 0;
    int isDigit = 1;
    while (i < len && isDigit) { // pour tout les caractères de la chaine
        if (!isdigit(nom_fichier[i])) { // si ce caractère est pas un nombre
            isDigit = 0; // on sort de la boucle, ce n'est pas un indice mais le nom du fichier 
        }
        i++;
    }
    if (isDigit){ // C'est l'indice du fichier
        nom_fichier = tabFichiers[atoi(nom_fichier)]; // On récupère le nom du fichier depuis le tableau
    }

    // Creation du chemin du fichier
    strcpy(chemin_fichier,"./filesServeur/");
    strcat(chemin_fichier,nom_fichier);

    printf("Chemin: %s\n",chemin_fichier);

    // ouverture du fichier
    FILE *fichier = fopen(chemin_fichier, "r");
    if (fichier == NULL){
        perror("Erreur lors de l'ouverture du fichier");
        return;
    }


    // prevenir le client de la reception:
    // envoi code commande fichier au client: (previent le serveur qu'on veut envoyer un fichier)
    char* messWrite = (char*)malloc(50*sizeof(char));
    strcpy(messWrite,"@writeFile");
    
    //envoi taille de messWrite:
    int a = 25;
    pthread_mutex_lock(&mutex);
    int taille= send(tabClientStruct[index].dSC,&a,sizeof(int),0);
    pthread_mutex_unlock(&mutex);
    if(taille ==-1){
        printf("ERRROR\n");
    }
    
    //envoi du messWrite
    pthread_mutex_lock(&mutex);
    int envoi = send(tabClientStruct[index].dSC,messWrite, sizeof(char)*25,0);
    pthread_mutex_unlock(&mutex);
    if(envoi == -1){
        printf("erreur\n");
    }
    printf("on a envoyé la commande: %s \n", messWrite);


    //// ICI ///////////////////////////////////////////////
    // connection du socket
    printf("Connexion du client à la socket getFile\n");
    struct sockaddr_in aClistF;
    socklen_t lgClistF = sizeof(struct sockaddr_in);
    int dSClistF = accept(dSlistF, (struct sockaddr *)&aClistF, &lgClistF);
    if (dSClistF == -1)
    {
        printf("Client %d non connecté \n", index);
        return;
    }
    printf("Client %d Connecté \n", index);

    // envoi du nom du fichier au client ( car thread reception )

    int envoie = send(dSClistF,nom_fichier, sizeof(char)*25,0);
    if(envoie == -1){
        printf("erreur\n");
    }

    printf("on a envoyé le nom du fichier \n");


    // calcule de la taille du fichier 
    int fsize;
    fseek(fichier, 0, SEEK_END);

    fsize = ftell(fichier);
    rewind(fichier);// remise du curseur de lecture à 0

    printf("%d\n",fsize);

    // envoi taille du fichier au client
    int sending = send(dSClistF,&fsize,sizeof(int),0 );
    if (sending == -1){
        printf("Errreur lors de l'envoi de la taille du fichier\n");
        
    }

    char* buffer = malloc(500*sizeof(char));
    // send le fichier par bouts
    int bytes_sent=0;
    int bytes_read=0;
    while ((bytes_read = fread(buffer, 1,500, fichier) )!= 0){
        int sended = send(dSClistF, buffer,bytes_read,0);
        if (sended == -1){
            printf("Erreur lors de l'envoi du fichier\n");
        }
        bytes_sent+=sended;
        printf("%d\n",bytes_sent);
    }
    close(dSClistF);
    fclose(fichier);
    free(buffer);
    free(messWrite);


}
void sendFileList(int index) {
    char fileList[500];
    strcpy(fileList, "Voici la liste des fichiers du serveur:\n");
    char line[270];

    DIR *dir;
    struct dirent *entry;

    dir = opendir("./filesServeur");
    if (dir == NULL) {
        perror("Erreur lors de l'ouverture du repertoire");
        return;
    }

    int i = 0;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) {// Si c'est un fichier
            sprintf(line, "%d  -->  %s\n", i, entry->d_name);
            strcat(fileList, line);
            tabFichiers[i] = strdup(entry->d_name);
            i++;
        }
    }

    closedir(dir);

    pthread_mutex_lock(&mutex);
    int sent = send_message(tabClientStruct[index].dSC, fileList, strlen(fileList));
    if (sent == -1) {
        printf("Erreur lors de l'envoi du message au client\n");
    }
    pthread_mutex_unlock(&mutex);
}

void recevoirFichier(int index) {
    // Reception du nom du fichier 

    printf("Connexion du client à la socket sendFile\n");
    struct sockaddr_in aC;
    socklen_t lg = sizeof(struct sockaddr_in);
    int dSCF = accept(dSF, (struct sockaddr *)&aC, &lg);
    if (dSCF == -1)
    {
        printf("Client %d non connecté \n", index);
        return;
    }
    printf("Client %d Connecté \n", index);

    char ack[] = "ACK";
    send(dSCF, ack, strlen(ack) + 1, 0);



    printf("Reception du nom du fichier\n");

    char* nom_fichier = malloc(50*sizeof(char));
    int recvMessage = recv(dSCF, nom_fichier, 50, 0);

    if(recvMessage == 0){close(dSCF); free(nom_fichier); return;}
    if(recvMessage == -1){perror("Réponse non reçue"); free(nom_fichier); return;}

    printf("Réponse reçue : %s\n", nom_fichier);

    // Reception de la taille du fichier 
    printf("Reception de la taille du  fichier\n");
    int tailleBufferReception;
    int recvTaille = recv(dSCF, &tailleBufferReception, sizeof(int), 0);

    if (recvTaille == 0) {close(dSCF); free(nom_fichier); return;}
    if (recvTaille == -1){perror("Taille non reçue\n"); free(nom_fichier); return;}

    printf("la taille %d\n", tailleBufferReception);

    // on créer le chemin du fichier pour le repertoire filesServeur
    char chemin_fichier[256];
    sprintf(chemin_fichier, "./filesServeur/%s", nom_fichier);

    // creation du fichier en mode write
    FILE* fichier = fopen(chemin_fichier, "wb");
    if (fichier == NULL){
        perror("Erreur lors de l'ouverture du fichier");
        free(nom_fichier);
        return;
    }

    int taille_recu = 0; // on calcul la taille 
    char buffer[500];
    while (taille_recu < tailleBufferReception) {
        
        int taille_restante = tailleBufferReception - taille_recu;
        if (taille_restante > sizeof(buffer)) {
            taille_restante = sizeof(buffer);
        }
        int recv_size = recv(dSCF, buffer, taille_restante, 0);
        if (recv_size == -1) {
            perror("Erreur lors de la réception du fichier");
            break;
        }
        fwrite(buffer, 1, recv_size, fichier); // on ecrit dans fichier le buffer reçu
        taille_recu += recv_size;
        printf("taille recu: %d\n",taille_recu);
    }
    close(dSCF);
    fclose(fichier);
    free(nom_fichier);
    printf("Fichier reçu avec succès !\n");
}

int verif_commande(char* message) {
    char cmp = '@';
    printf("message[0] = %c \n", message[0]);
    if (message[0] == cmp) {
        printf("Ceci est une commande\n");
        return 1;
    }
    else {
        printf("C'est un message\n");
        return 0;
    }
}

int trouverIndexDisponible() {
    pthread_mutex_lock(&mutex);
    for (int j = 0; j < lengthTabClient; j++) {
        printf("Client %d : %s\n", tabClientStruct[j].dSC, tabClientStruct[j].pseudo);
        if (tabClientStruct[j].dSC == -1) {
            pthread_mutex_unlock(&mutex);
            return j;
        }
    }
    pthread_mutex_unlock(&mutex);
    return -1;
}

// Regarde à qui on envoie le message privé
int commande(char* mess){
    if (verif_commande(mess) == 1){
        char* message = (char*) malloc(strlen(mess) * sizeof(char));
        strcpy(message, mess); // on copie le message pour pouvoir le modifier
        char* cmd = strtok(message, " "); // on isole @(truc)
        cmd = strtok(cmd, "@"); // on récup l'(truc)
        printf("cmd %s \n", cmd);

        if (strncmp(cmd,"fin", 3) == 0) return -2; //on renvoie -2 si truc == fin
        else if (strncmp(cmd, "list", 4) == 0) return -4; //on renvoie -4 si truc == list
        else if (strncmp(cmd, "help", 4) == 0) return -5; //on renvoie -5 si truc == help
        else if (strncmp(cmd,"file", 4)==0) return -6 ; // on renvoie -6 si truc == file et donc qu'on s'apprête à recupérer un fichier depuis le client
        else if (strncmp(cmd,"fichierServ",11) == 0) return -7; // on renvoi la liste des fichiers du repertoire serv au client
        else if ( strncmp(cmd,"getFile", 7) == 0) return -8; // on envoi le fichier demandé par le client
        else if (strncmp(cmd,"goTo", 4) == 0) return -9; //on renvoie -9 si truc == goTo pour le changement de salon
        else if (strncmp(cmd,"getSalons", 9) == 0) return -10; // on renvoi la liste des salons disponibles
        else {
            printf("Message privé ...\n");
            char target_pseudo[TAILLE_PSEUDO]; // Assurez-vous de définir une longueur maximale pour les pseudos

            if (sscanf(message, "@%s\n", target_pseudo) != 1) {
                printf("Erreur de lecture du pseudo\n");
            }
            pthread_mutex_lock(&mutex);
            int target_index = -3;
            for (int i = 0; i < lengthTabClient; ++i) {
                printf("%s ?= %s\n", tabClientStruct[i].pseudo, target_pseudo);
                if (strcmp(tabClientStruct[i].pseudo, target_pseudo) == 0) {
                    target_index = i;
                    break;
                }        
            }
            pthread_mutex_unlock(&mutex);
            printf("Message privé à %s dont l'id est %d\n", target_pseudo, target_index);
            return target_index; //on renvoie si truc => index sinon renvoie -3 le pseudo n'existe pas
        }
    }
    return -1; // pas une commande
}

int idSalonDemande(char * nomSalonDemande){
    printf("Nom du salon demandé : %s\n", nomSalonDemande);
    
    size_t len = strlen(nomSalonDemande);
    int j = 0;
    int isDigit = 1;
    while (j < len && isDigit) { // pour tout les caractères de la chaine
        if (!isdigit(nomSalonDemande[j])) { // si ce caractère est pas un nombre
            isDigit = 0; // on sort de la boucle, ce n'est pas un indice mais le nom du salon 
        }
        j++;
    }
    if (isDigit){ // C'est l'indice du salon
        printf("C'est un indice %d\n", atoi(nomSalonDemande));
        return atoi(nomSalonDemande);
    }

    int i = 0;
    while (i < nbSalons){
        if (strncmp(tabSalons[i].nomSalon, nomSalonDemande, strlen(nomSalonDemande)) == 0){
            printf("Salon trouvé\n");
            return i;
        }
        i++;
    }
    return -1;
}

void* communication(void* arg){

    thread_args *args = (thread_args *) arg;
    long index = args->index;

    int sortie = 0;
    while (sortie == 0){
        
        // Reception de la réponse client Writer: 
        int recvTaille = recv(tabClientStruct[index].dSC, &tailleBufferReception, sizeof(int), 0);

        if (recvTaille == 0) {deconnecterClient(index);break;}
        if (recvTaille == -1){perror("Taille non reçue\n");break;}

        printf("la taille %d\n", tailleBufferReception);
        

        // Reception message
        char* message1 = malloc(tailleBufferReception*sizeof(char));

        int recvMessage = recv(tabClientStruct[index].dSC, message1, tailleBufferReception, 0);

        if(recvMessage == 0){deconnecterClient(index);break;}
        if(recvMessage == -1){perror("Réponse non reçue");break;}

        printf("Réponse reçue : %s\n", message1);

        long i = 0;
        while (i < lengthTabClient && sortie == 0){
            if (i != index){
                // Recuperation du code commande
                int cmd = commande(message1);
                printf("cmd == %d\n",cmd);

                if (cmd == -1){ // On envoie le message à tout le monde
                    pthread_mutex_lock(&mutex);
                    if (tabClientStruct[i].idSalon == tabClientStruct[index].idSalon && tabClientStruct[i].dSC != -1) {
                        printf("Envoi du message à %s\n", tabClientStruct[i].pseudo);
                        int sended = send_message(tabClientStruct[i].dSC, message1, tailleBufferReception);
                        if (sended == -1){
                            printf("Erreur lors de l'envoi du message à %s\n", tabClientStruct[i].pseudo);
                        }
                    }
                    pthread_mutex_unlock(&mutex);
                    i++;
                    continue;
                }
                else if (cmd == -2) { // Déconnecter l'utilisateur actuel uniquement
                    printf("Déconnexion du client %ld\n", index);
                    printf("Client %s déconnecté (fin) \n", tabClientStruct[index].pseudo);
                    sortie = 1;
                    break;
                }

                else if (cmd == -3) { // Pas de client trouvé pour ce pseudo
                    pthread_mutex_lock(&mutex);
                    char* message = "Pas de client trouvé pour ce pseudo";
                    int sended = send_message(tabClientStruct[index].dSC, message, strlen(message));
                    if (sended == -1){
                        printf("Erreur lors de l'envoi du message à %s\n", tabClientStruct[i].pseudo);
                    }
                    pthread_mutex_unlock(&mutex);
                    break;
                }
                else if (cmd == -4){ // On demande la liste des utilisateurs
                    pthread_mutex_lock(&mutex);
                    printf("\nListe des utilisateurs connectés :\n");
                    char* message = malloc(4096*sizeof(char));
                    strcat(message, "Liste des utilisateurs connectés :\n");
                    for (int j = 0; j < lengthTabClient; ++j) {
                        if (strncmp(tabClientStruct[j].pseudo, "__DÉCONNECTÉ__", 14) != 0) {
                            printf("Client %d : %s\n", j ,tabClientStruct[j].pseudo);
                            strcat(message, tabClientStruct[j].pseudo);
                            strcat(message, " dans le salon :");
                            char idSalon[2];
                            sprintf(idSalon, "%d", tabClientStruct[j].idSalon);
                            strcat(message, idSalon);
                            strcat(message, "\n");
                        }
                    }
                    int sended = send_message(tabClientStruct[index].dSC, message, strlen(message));
                    if (sended == -1){
                        printf("Erreur lors de l'envoi du message à %s\n", tabClientStruct[index].pseudo);
                    }
                    pthread_mutex_unlock(&mutex);
                    break;
                }
                else if (cmd == -5){ // On envoie le contenue du manuel.txt
                    FILE *fichier = fopen("./manuel.txt", "r");
                    if (fichier == NULL){
                        perror("Erreur lors de l'ouverture du fichier");
                        break;
                    }
                    char* manuel = malloc(4096*sizeof(char));
                    char* ligne = malloc(4096*sizeof(char));
                    while (fgets(ligne, 4096, fichier) != NULL){
                        printf("%s", manuel);
                        strcat(manuel, ligne);
                    }
                    fclose(fichier);
                    pthread_mutex_lock(&mutex);
                    int sended = send_message(tabClientStruct[index].dSC, manuel, strlen(manuel));
                    if (sended == -1){
                        printf("Erreur lors de l'envoi du message à %s\n", tabClientStruct[index].pseudo);
                    }
                    pthread_mutex_unlock(&mutex);
                    break;
                }
                else if (cmd == -6){ // on receptionne le fichier client uploader
                    printf("zone de reception fichier\n");
                    recevoirFichier(index);
                    break;
                }
                else if (cmd == -7){ // on envoi la liste des fichiers du repertoire serveur au client
                   sendFileList(index);
                }
                else if (cmd == -8){ // envoi d'un fichier au client
                    sendFileList(index);
                    printf("Avant d'entré\n");
                    getFile(index);
                    printf("Après être entré\n");
                    break;
                }
                else if (cmd == -9){ // @goTo 2
                
                    printf("Commande goTo lancée \n");

                    pthread_mutex_lock(&mutex);

                    char* nomSalonDemande = malloc(TAILLE_NOM_SALON*sizeof(char));
                    strcpy(nomSalonDemande, message1);
                    printf("strcpy fait, nomSalonDemande = %s \n", nomSalonDemande);

                    if (nomSalonDemande[strlen(nomSalonDemande)-1] == '\n') nomSalonDemande[strlen(nomSalonDemande)-1] == '\0';

                    printf("nom du salon %s \n", nomSalonDemande);

                    nomSalonDemande = strtok(nomSalonDemande, " ");
                    nomSalonDemande = strtok(NULL, " ");

                    for (int i = 0; i < strlen(nomSalonDemande); i++) {
                        if (nomSalonDemande[i] == '\n') {
                            nomSalonDemande[i] = '\0';
                        }
                    }

                    printf("nom du salon après strtok %s \n", nomSalonDemande);
                    
                    int idSalon = idSalonDemande(nomSalonDemande);
                    printf("idSalon = %d\n", idSalon);
                    if (idSalon == -1) {
                        printf("Salon non trouvé\n");
                        send_message(tabClientStruct[index].dSC, "Salon non trouvé", strlen("Salon non trouvé"));
                        pthread_mutex_unlock(&mutex);
                        break;
                    }
                    tabClientStruct[index].idSalon = idSalon;

                    char* newMessage = malloc(4096*sizeof(char));

                    strcat(newMessage, "@idSalon ");

                    char idSalonToChar[2];
                    sprintf(idSalonToChar, "%d", idSalon);
                    strcat(newMessage, idSalonToChar);

                    strcat(newMessage, "\n");

                    printf("Nouveau message : %s\n", newMessage);

                    int sended = send_message(tabClientStruct[index].dSC, newMessage, tailleBufferReception);
                    if (sended == -1){
                        printf("Erreur lors de l'envoi du message de changement de salon à %s\n", tabClientStruct[index].pseudo);
                    }
                    pthread_mutex_unlock(&mutex);
                    printf("Changement de salon effectué\n");
                }
                else if (cmd ==-10){ // on envoi la liste des salons disponibles 
                    printf("\nListe des salons disponibles :\n");
                    char* message = malloc(4096*sizeof(char));
                    strcat(message, "Liste des salons disponibles :\n");
                    for (int j = 0; j < nbSalons; j++) {
                        
                        printf("Salon %d : %s , description: %s\n", j ,tabSalons[j].nomSalon, tabSalons[j].descriptionSalon);
                        char idSalon[2];
                        sprintf(idSalon, "%d", j);
                        strcat(message, idSalon);
                        strcat(message, " Nom: ");
                        strcat(message,tabSalons[j].nomSalon);
                        strcat(message, " Descritpion: ");
                        strcat(message,tabSalons[j].descriptionSalon);
                        strcat(message, "\n");
                        
                    }
                    int sended = send_message(tabClientStruct[index].dSC, message, strlen(message));
                    if (sended == -1){
                        printf("Erreur lors de l'envoi du message à %s\n", tabClientStruct[index].pseudo);
                    }
                }
                else if (cmd == i){ // On envoie le message privé
                    pthread_mutex_lock(&mutex);
                    int sended = send_message(tabClientStruct[i].dSC, message1, tailleBufferReception);
                    if (sended == -1){
                        printf("Erreur lors de l'envoi du message à %s\n", tabClientStruct[i].pseudo);
                    }
                    pthread_mutex_unlock(&mutex);
                }
            }
            i+=1;
        }
    }
    
    printf("Sortie, déconnexion du client %s %ld\n", tabClientStruct[index].pseudo ,index);
    deconnecterClient(index);
    printf("Client %s déconnecté (fin) \n", tabClientStruct[index].pseudo);

    sem_post(&semaphore);
    free(args);
    pthread_exit(NULL);
}

//  fonction pseudo déjà utilisé
int pseudoDejaUtilise(char* pseudo){
    
    for (int i = 0; i < lengthTabClient; i++){
        pthread_mutex_lock(&mutex);
        if (strcmp(pseudo, tabClientStruct[i].pseudo) == 0){
            pthread_mutex_unlock(&mutex);
            return 1;
        } 
        pthread_mutex_unlock(&mutex);
    }
    return 0;
}

int main(int argc, char *argv[])
{
    // Enregistrement du gestionnaire de signaux
	if(signal(SIGINT, sig_handler) == SIG_ERR){
		puts("Erreur à l'enregistrement du gestionnaire de signaux !");
	}

    if (argc != 4)
    {
        perror("./serveur port nbcli nbsalon");
        exit(1);
    }

    printf("Début programme\n");

    int dS = socket(PF_INET, SOCK_STREAM, 0);
    if (dS == -1)
    {
        perror(" Socket serveur non créé");
        exit(1);
    }
    dSF = socket(PF_INET, SOCK_STREAM, 0);
    if (dSF == -1)
    {
        perror(" Socket sendFile non créé");
        exit(1);
    }
    dSlistF = socket(PF_INET, SOCK_STREAM, 0);
    if (dSlistF == -1)
    {
        perror(" Socket listFile non créé");
        exit(1);
    }
    printf("Sockets Créés\n");

    //socket serveur
    struct sockaddr_in ad;
    //socket sendFile
    struct sockaddr_in adFile;
    //socket listFile
    struct sockaddr_in adListFile;


    ad.sin_family = AF_INET;
    adFile.sin_family = AF_INET;
    adListFile.sin_family = AF_INET;

    ad.sin_addr.s_addr = INADDR_ANY;
    adFile.sin_addr.s_addr = INADDR_ANY;
    adListFile.sin_addr.s_addr = INADDR_ANY;

    ad.sin_port = htons(atoi(argv[1]));
    adFile.sin_port = htons(atoi(argv[1])+260); // port du socket sendFile
    adListFile.sin_port = htons(atoi(argv[1])+300); // port du socket sendFile

    if (bind(dS, (struct sockaddr *)&ad, sizeof(ad)) == -1)
    {
        perror("Socket serveur pas nommée");
        exit(1);
    }
    if (bind(dSF, (struct sockaddr *)&adFile, sizeof(adFile)) == -1)
    {
        perror("Socket sendFile pas nommée");
        exit(1);
    }
    if (bind(dSlistF, (struct sockaddr *)&adListFile, sizeof(adListFile)) == -1)
    {
        perror("Socket sendFile pas nommée");
        exit(1);
    }


    printf("Sockets Nommés\n");

    if (listen(dS, 7) == -1)
    {
        perror("Mode écoute serveur non activé");
        exit(1);
    }
    if (listen(dSF, 7) == -1)
    {
        perror("Mode écoute sendFile non activé");
        exit(1);
    }
    if (listen(dSlistF, 7) == -1)
    {
        perror("Mode écoute sendFile non activé");
        exit(1);
    }
    printf("Modes écoutes\n");


    int nbcli = atoi(argv[2]);
    pthread_t* thread = (pthread_t*) malloc(nbcli*sizeof(int));
    tabClientStruct = malloc(nbcli * sizeof(dSClient));
    nbSalons = atoi(argv[3]);
    tabSalons = malloc(nbSalons * sizeof(salon));
    // Salon 1
    strcpy(tabSalons[0].nomSalon, "General");
    strcpy(tabSalons[0].descriptionSalon, "Le salon general commun.");
    
    // Salon 2
    strcpy(tabSalons[1].nomSalon, "Secondaire");
    strcpy(tabSalons[1].descriptionSalon, "Le salon secondaire pas commun lol.");


    lengthTabClient = nbcli;

    // Ajout du sémaphore
    sem_init(&semaphore, 0, nbcli);

    //Ajout du mutex
    pthread_mutex_init(&mutex, NULL);

    pthread_mutex_lock(&mutex);
    for (int i = 0; i < nbcli; i++){
        tabClientStruct[i].dSC = -1;
        strcpy(tabClientStruct[i].pseudo, "__DÉCONNECTÉ__");
        tabClientStruct[i].idSalon = 0;
    }
    pthread_mutex_unlock(&mutex);
    // Le code:
    while (1){
        
        // On verifie que le client peut rentrer soit qu'il y ait des jetons
        sem_wait(&semaphore);
        printf("Client connecté\n");
        // Recherche d'un emplacement vide dans le tableau tabClientStruct
        int index = trouverIndexDisponible();

        if (index != -1) {

            // Clients :
            struct sockaddr_in aC;
            socklen_t lg = sizeof(struct sockaddr_in);
            int dSC = accept(dS, (struct sockaddr *)&aC, &lg);
            if (dSC == -1)
            {
                printf("Client %d non connecté \n", index);
                continue;
            }
            printf("Client %d Connecté \n", index);

            char ack[] = "ACK";
            send(dSC, ack, strlen(ack) + 1, 0);

            // Reception pseudo
            char* pseudo = malloc(sizeof(char)*TAILLE_PSEUDO);
            int recvPseudo = recv(dSC, pseudo, TAILLE_PSEUDO, 0);
            if(recvPseudo == 0){
                printf("Client %d déconnecté \n", index);
                continue;
            }
            if(recvPseudo == -1){
                perror("Pseudo non reçue");
                continue;
            }
            printf("Pseudo client %d : %s\n", dSC, pseudo);

            if (pseudo[strlen(pseudo)-1] == '\n') 
            pseudo[strlen(pseudo)-1] = '\0';

            if (strncmp(pseudo, "__DÉCONNECTÉ__", 14) == 0|| strncmp(pseudo, "getFile", 7) == 0 || strncmp(pseudo, "fichierServ", 11) == 0 || strncmp(pseudo, "sendFile", 8) == 0 || strncmp(pseudo, "@fin", 4) == 0 ||strncmp(pseudo, "list", 4) == 0 ||  strncmp(pseudo, "help", 4) == 0 ||   strncmp(pseudo, "fin", 3) == 0 || pseudoDejaUtilise(pseudo) == 1){
                
                char* messagePseudo = "Pseudo déjà utilisé ou pseudo invalide";
                send_message(dSC, messagePseudo, strlen(messagePseudo));

                // envoyer @fin au client pour qu'il se déconnecte (avec la taille)
                char* messageFin = "@fin";
                send_message(dSC, messageFin, strlen(messageFin));
                sem_post(&semaphore);        
                continue;
            }
    
            pthread_mutex_lock(&mutex);
            
            tabClientStruct[index].dSC = dSC;
            strncpy(tabClientStruct[index].pseudo, pseudo, TAILLE_PSEUDO);
            
            pthread_mutex_unlock(&mutex);

            // Ajout de l'attente du sémaphore et la création du thread avec les arguments
            thread_args *args = (thread_args *) malloc(sizeof(thread_args));
            args->index = index;
            pthread_create(&thread[index], NULL, communication, (void *)args);
        }
    }
    pthread_mutex_destroy(&mutex);
    shutdown(dS, 2);
    sem_destroy(&semaphore);
    printf("Fin du programme\n");
}

