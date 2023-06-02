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
#define DISPO "DISPONIBLE"

// Structure d'un salons
typedef struct {
    char nomSalon[TAILLE_NOM_SALON];
    char descriptionSalon[TAILLE_DESCRIPTION_SALON];
} salon;


// Structure info Client
typedef struct {
    int idSalon;
    int dSC;
    char pseudo[TAILLE_PSEUDO];
} dSClient;

dSClient* tabClientStruct;
int tailleBufferReception;
int lengthTabClient;

// Structure pour les arguments passés au thread
typedef struct {
    long index;
} thread_args;


// Variable pour les salons
salon* tabSalons; // Tableau de salons
int nbSalons;

// Declaration mutex et semaphore
pthread_mutex_t mutex;
sem_t semaphore;


int dSF;
int dSlistF;
char* tabFichiers[100];


/**
*@brief Fonction utilisée pour déconnecter un client
*@param index Index du client dans le tableau des clients
*/
void deconnecterClient(int index) {
    pthread_mutex_lock(&mutex);
    printf("Client %d déconnecté \n", index);
    // Mise à jour du tableau à l'indice donné
    tabClientStruct[index].dSC = -1;
    strncpy(tabClientStruct[index].pseudo, "__DÉCONNECTÉ__", TAILLE_PSEUDO);
    tabClientStruct[index].idSalon = 0;
    pthread_mutex_unlock(&mutex);
}


/**
*@brief Fonction qui envoi un message à un client
*@param client_fd Adresse socket client
*@param message Message à envoyer
*@param message_size Taille du message à envoyer
*@return 0 si l'envoi s'est effectué, -1 si erreur
*/
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

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                   GESTION DES SIGNAUX                                                    //
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
*@brief Fonction qui gère les signaux 
*@param sig Signal reçu par l'OS
*/
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
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                   FONCTIONS UTILES AUX FICHIERS                                          //
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
*@brief Fonction qui gère l'envoi au client d'un fichier
*@param index Index du client
*/
void getFile(int index){

    // Reception du nom du fichier 
    printf("Reception du nom du fichier\n");

    // Variables utiles
    char* chemin_fichier = (char*)malloc(150*sizeof(char));
    char* nom_fichier = malloc(50*sizeof(char));

    // Réception du nom du fichier
    pthread_mutex_lock(&mutex);
    int recvMessage = recv(tabClientStruct[index].dSC, nom_fichier, 50, 0);
    pthread_mutex_unlock(&mutex);

    if(recvMessage == 0){deconnecterClient(index); free(nom_fichier); return;}
    if(recvMessage == -1){perror("Réponse non reçue"); free(nom_fichier); return;}
    
    // Gestion des caractères
    nom_fichier[strlen(nom_fichier)] = '\0';
    

    if (nom_fichier[strlen(nom_fichier)-1] == '\n') // On enlève le \n de la chaine
        nom_fichier[strlen(nom_fichier)-1] = '\0';
    
    for (int i = 0; i < strlen(nom_fichier); i++) {
        if (nom_fichier[i] == '\n') {
            nom_fichier[i] = '\0';
        }
    }
    


    // Gestion en fonction de si on a envoyé un indice ou le nom du fichier directement
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

    printf("Nom du fichier a envoyer : %s\n", nom_fichier);
    // Creation du chemin du fichier
    strcpy(chemin_fichier,"./filesServeur/");
    strcat(chemin_fichier,nom_fichier);

    // ouverture du fichier
    FILE *fichier = fopen(chemin_fichier, "r");
    if (fichier == NULL){
        perror("Erreur lors de l'ouverture du fichier");
        return;
    }


    // prevenir le client de la reception:
    // envoi code commande fichier au client: (previent le client qu'on veut envoyer un fichier)
    char* messWrite = (char*)malloc(50*sizeof(char));
    strcpy(messWrite,"@writeFile");
    
    //envoi taille de messWrite:
    int a = 25;
    pthread_mutex_lock(&mutex);
    int taille= send(tabClientStruct[index].dSC,&a,sizeof(int),0);
    pthread_mutex_unlock(&mutex);
    if(taille ==-1){
        printf("Erreur lors de l'envoi de la taille de la commande @writeFile au client \n");
    }
    
    //envoi du messWrite
    pthread_mutex_lock(&mutex);
    int envoi = send(tabClientStruct[index].dSC,messWrite, sizeof(char)*25,0);
    pthread_mutex_unlock(&mutex);
    if(envoi == -1){
        printf("Erreur lors de l'envoi de la commande @writeFile au client \n");
    }


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
        printf("Erreur lors de l'envoi au client du nom du fichier\n");
    }


    // calcule de la taille du fichier 
    int fsize;
    fseek(fichier, 0, SEEK_END);

    fsize = ftell(fichier);
    rewind(fichier);// remise du curseur de lecture à 0


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
    }
    close(dSClistF);
    fclose(fichier);
    free(buffer);
    free(messWrite);


}


/**
*@brief Fonction qui gère l'envoi au client de la liste des fichiers
*@param index Index du client
*/
void sendFileList(int index) {
    char fileList[500];
    char line[270];

    //Preparation du bloc d'envoi
    strcpy(fileList, "Voici la liste des fichiers du serveur:\n");

    // Ouverture du repertoire
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
    // Envoi de la liste au client
    pthread_mutex_lock(&mutex);
    int sent = send_message(tabClientStruct[index].dSC, fileList, strlen(fileList));
    if (sent == -1) {
        printf("Erreur lors de l'envoi du message au client\n");
    }
    pthread_mutex_unlock(&mutex);
}

/**
*@brief Fonction qui gère la reception de fichier
*@param index Index du client
*/
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

    // Envoi la confirmation de la synchronisation
    char ack[] = "ACK";
    send(dSCF, ack, strlen(ack) + 1, 0);

    // Reception du nom du fichier
    char* nom_fichier = malloc(50*sizeof(char));
    int recvMessage = recv(dSCF, nom_fichier, 50, 0);

    if(recvMessage == 0){close(dSCF); free(nom_fichier); return;}
    if(recvMessage == -1){perror("Réponse non reçue"); free(nom_fichier); return;}
    
    // Gestion des caractères
    for (int i = 0; i < strlen(nom_fichier); i++) {
    if (nom_fichier[i] == '\n') {
        nom_fichier[i] = '\0';
    }
}
    printf("\nFichier a recevoir : %s\n", nom_fichier);

    // Reception de la taille du fichier 
    int tailleBufferReception;
    int recvTaille = recv(dSCF, &tailleBufferReception, sizeof(int), 0);

    if (recvTaille == 0) {close(dSCF); free(nom_fichier); return;}
    if (recvTaille == -1){perror("Taille non reçue\n"); free(nom_fichier); return;}

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
    // Reception du fichier par bouts
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
    }
    close(dSCF);
    fclose(fichier);
    free(nom_fichier);
    printf("\nFichier reçu avec succès !\n");
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                  FIN FONCTIONS UTILES AUX FICHIERS                                       //
//////////////////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                   GESTION DES COMMANDES                                                  //
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
*@brief Fonction qui vérifie si le message est un message de commande
*@param message Message envoyé par le client ou par le serveur
*@return 1 si il s'agit d'une commande, 0 sinon
*/
int verif_commande(char* message) {
    char cmp = '@';
    if (message[0] == cmp) {
        return 1;
    }
    else {
        return 0;
    }
}

/**
*@brief Fonction qui indique quelle commande est contenue dans le message
*@param mess Message envoyé par le client ou par le serveur
*@return -1 si il ne s'agit pas d'une commande, 
        -2 si il s'agit de la commande de déconnection fin
        -3 si il s'agit de la commande de message privé
        -4 si il s'agit de l'envoi de la liste des clients du serveur
        -5 si il s'agit de l'envoi du manuel help
        -6 si on s'apprête à recupérer un fichier depuis le client
        -7 si on doit envoyer la liste des fichiers du repertoire serv au client
        -8 si il s'agit de la commande d'envoi de fichier au client
        -9 si il s'agit de la commande pour le changement de salon
        -10 si on doit envoyer la liste des salons disponibles
        -11 si on doit créer un nouveau salon de discussion
        -12 si on veut supprimer un salon de discussion

*/
int commande(char* mess){
    if (verif_commande(mess) == 1){
        char* message = (char*) malloc(strlen(mess) * sizeof(char));
        strcpy(message, mess); // on copie le message pour pouvoir le modifier
        char* cmd = strtok(message, " "); // on isole l'élément après l'@ 
        cmd = strtok(cmd, "@"); // on récup l'élément après l'@ (le nom de la commande)
        

        if (strncmp(cmd,"fin", 3) == 0) return -2; 
        else if (strncmp(cmd, "list", 4) == 0) return -4; 
        else if (strncmp(cmd, "help", 4) == 0) return -5; 
        else if (strncmp(cmd,"file", 4)==0) return -6 ; 
        else if (strncmp(cmd,"fichierServ",11) == 0) return -7; 
        else if ( strncmp(cmd,"getFile", 7) == 0) return -8; 
        else if (strncmp(cmd,"goTo", 4) == 0) return -9; 
        else if (strncmp(cmd,"getSalons", 9) == 0) return -10; 
        else if (strncmp(cmd, "addSalon", 8) == 0) return -11; 
        else if (strncmp(cmd, "suppSalon", 9) == 0) return -12; 
        else {
            
            char target_pseudo[TAILLE_PSEUDO]; 

            if (sscanf(message, "@%s\n", target_pseudo) != 1) {
                printf("Erreur de lecture du pseudo\n");
            }
            pthread_mutex_lock(&mutex);
            int target_index = -3;
            for (int i = 0; i < lengthTabClient; ++i) {
                if (strcmp(tabClientStruct[i].pseudo, target_pseudo) == 0) {
                    target_index = i;
                    break;
                }        
            }
            pthread_mutex_unlock(&mutex);
            return target_index; 
        }
    }
    return -1; 
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                   FIN GESTION DES COMMANDES                                              //
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                   FONCTION DE RECHERCHE DANS LES TABLEAUX                                //
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
* @brief Fonction qui permet de récupérer le premier indice disponible à la création d'un client
* @return int Indice dans le tableau des clients
*/
int trouverIndexDisponible() {
    pthread_mutex_lock(&mutex);
    for (int j = 0; j < lengthTabClient; j++) {
        if (tabClientStruct[j].dSC == -1) {
            pthread_mutex_unlock(&mutex);
            return j;
        }
    }
    pthread_mutex_unlock(&mutex);
    return -1;
}

/**
* @brief Fonction qui permet de récupérer le premier salon disponible à la création
* @return int Indice de la position trouvée
*/
int getPositionFreeSalon(){
    int i = 0;
    int pos = 0;
    while (i < nbSalons && pos == 0){
        int changement = strncmp(tabSalons[i].nomSalon, DISPO, TAILLE_NOM_SALON);
        
        if (changement == 0){
            return i;
        }
        i++;
    }
    return -1;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                   FONCTIONS RELATIVES AUX SALONS                                         //
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
* @brief Fonction qui permet de récupérer l'id du salon demandé
* @param nomSalonDemande : nom du salon demande
* @return int Id du salon demande
*/
int idSalonDemande(char * nomSalonDemande){
    printf("\nNom du salon demandé : %s\n", nomSalonDemande);
    
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
        return atoi(nomSalonDemande);
    }

    int i = 0;
    while (i < nbSalons){
        if (strncmp(tabSalons[i].nomSalon, nomSalonDemande, strlen(nomSalonDemande)) == 0){
            return i;
        }
        i++;
    }
    return -1;
}


/**
* @brief Fonction qui permet de créer un nouveau salon de discussion et de l'ajouter au tableau des salons
* @param index : index du client qui a demandé la création du salon
* @param nomSalon : nom du salon
* @param descriptionSalon : description du salon
* @return void
*/
void addNewSalon(int index, char* nomSalon, char* descriptionSalon){
    printf("\nCréation d'un nouveau salon\n");
    // On vérifie que le nombre de salons n'est pas dépassé
    int posDispo = getPositionFreeSalon();
    if (posDispo == -1) {
        pthread_mutex_lock(&mutex);
        char* messageMaxSalon = "Nombre de salons dépassé";
        send_message(tabClientStruct[index].dSC, messageMaxSalon, strlen(messageMaxSalon));
        pthread_mutex_unlock(&mutex);
        return;
    }
    // On vérifie que le nom du salon n'est pas déjà utilisé
    for (int i = 0; i < nbSalons; ++i) {
        if (strcmp(tabSalons[i].nomSalon, nomSalon) == 0) {
            pthread_mutex_lock(&mutex);
            char* messageNomIndispo = "Le nom du salon est déjà utilisé";
            printf("%s\n", messageNomIndispo);
            send_message(tabClientStruct[index].dSC, messageNomIndispo, strlen(messageNomIndispo));
            pthread_mutex_unlock(&mutex);
            return;
        }
    }
    // On ajoute le salon au tableau des salons
    pthread_mutex_lock(&mutex);
    strcpy(tabSalons[posDispo].nomSalon, nomSalon);
    strcpy(tabSalons[posDispo].descriptionSalon, descriptionSalon);

    // On ajoute le salon au fichier config.txt
    FILE *fichier = fopen("./config.txt", "a");
    if (fichier == NULL){
        perror("Erreur lors de l'ouverture du fichier");
        return;
    }
    char* ligne = malloc(4096*sizeof(char));
    sprintf(ligne, "%d-%s-%s\n", posDispo, nomSalon, descriptionSalon);
    fputs(ligne, fichier);
    fclose(fichier);

    // On envoie un message au client pour lui dire que le salon a été ajouté
    char* message2 = "Salon ajouté";
    printf("%s\n", message2);
    send_message(tabClientStruct[index].dSC, message2, strlen(message2));
    pthread_mutex_unlock(&mutex);
}

/**
* @brief Fonction qui permet de supprimer un salon de discussion et de le supprimer au tableau des salons
* @param index : index du client qui a demandé la création du salon
* @param idSalon : nom du salon
* @return void
*/
void suppOldSalon(int index, int idSalon){
    printf("\nSuppression du salon %d\n", idSalon);

    if (idSalon == 0){
        pthread_mutex_lock(&mutex);
        char* messageMaxSalon = "Impossible de supprimer le salon Général";
        printf("%s\n", messageMaxSalon);
        send_message(tabClientStruct[index].dSC, messageMaxSalon, strlen(messageMaxSalon));
        pthread_mutex_unlock(&mutex);
        return;
    }

    // On déplace tout les clients dans le salon général
    pthread_mutex_lock(&mutex);
    for (int i = 0; i < lengthTabClient; ++i) {
        if (tabClientStruct[i].idSalon == idSalon) {
            tabClientStruct[i].idSalon = 0;
            // On envoie un message au client pour lui dire qu'il a été déplacé dans le salon général
            char* message = "Vous avez été déplacé dans le salon général car le salon dans lequel vous étiez a été supprimé";
            send_message(tabClientStruct[i].dSC, message, strlen(message));
        }
    }
    pthread_mutex_unlock(&mutex);

    pthread_mutex_lock(&mutex);
    strcpy(tabSalons[idSalon].nomSalon, DISPO);
    strcpy(tabSalons[idSalon].descriptionSalon, "");

    // On supprime le salon au fichier config.txt
    FILE *fichier = fopen("./config.txt", "r");
    FILE *tmp = fopen("./tmpConfig.txt", "a");
    if (fichier == NULL){
        perror("Erreur lors de l'ouverture du fichier");
        return;
    }
    while (feof(fichier) == 0){
        // lecture d'une ligne
        char* ligne = malloc(4096*sizeof(char));
        fgets(ligne, 4096, fichier);
        if (ligne[0] == '\n') continue;

        // On recopie ligne dans id
        char* id = malloc(256*sizeof(char));
        strcpy(id, ligne);
        strtok(id, "-");
        
        if (atoi(id) != idSalon) {
            fputs(ligne, tmp);

        }
        free(ligne);
    };
    fclose(fichier);
    fclose(tmp);
    remove("./config.txt");
    rename("./tmpConfig.txt", "./config.txt");

    // On envoie un message au client pour lui dire que le salon a été supprimé
    char* message2 = "Salon supprimé";
    printf("%s\n", message2);
    send_message(tabClientStruct[index].dSC, message2, strlen(message2));
    pthread_mutex_unlock(&mutex);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                  FIN FONCTIONS RELATIVES AUX SALONS                                      //
//////////////////////////////////////////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                    FONCTION UTILISE PAR LE THREAD                                        //
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
*@brief Fonction utilisée pour la communication avec les clients
*@param args 
*/
void* communication(void* arg){

    thread_args *args = (thread_args *) arg;
    long index = args->index;

    int sortie = 0;
    while (sortie == 0){
        
        // Reception de la réponse client Writer: 
        int recvTaille = recv(tabClientStruct[index].dSC, &tailleBufferReception, sizeof(int), 0);

        if (recvTaille == 0) {deconnecterClient(index);break;}
        if (recvTaille == -1){perror("Taille non reçue\n");break;}

        

        // Reception message
        char* message1 = malloc(tailleBufferReception*sizeof(char));

        int recvMessage = recv(tabClientStruct[index].dSC, message1, tailleBufferReception, 0);

        if(recvMessage == 0){deconnecterClient(index);break;}
        if(recvMessage == -1){perror("Réponse non reçue");break;}

        printf("\nMessage reçue \n");

        long i = 0;
        while (i < lengthTabClient && sortie == 0){
            if (i != index){
                // Recuperation du code commande
                int cmd = commande(message1);

                if (cmd == -1){ // On envoie le message à tout le monde
                    pthread_mutex_lock(&mutex);
                    if (tabClientStruct[i].idSalon == tabClientStruct[index].idSalon && tabClientStruct[i].dSC != -1) {
                        
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
                    printf("\nEnvoi liste des utilisateurs connectés \n");
                    char* message = malloc(4096*sizeof(char));
                    strcat(message, "Liste des utilisateurs connectés :\n");
                    for (int j = 0; j < lengthTabClient; ++j) {
                        if (strncmp(tabClientStruct[j].pseudo, "__DÉCONNECTÉ__", 14) != 0) {
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
                    printf("\nReception fichier\n");
                    recevoirFichier(index);
                    break;
                }
                else if (cmd == -7){ // on envoi la liste des fichiers du repertoire serveur au client
                   sendFileList(index);
                   break;
                }
                else if (cmd == -8){ // envoi d'un fichier au client
                    sendFileList(index);
                    getFile(index);
                    break;
                }
                else if (cmd == -9){ // @goTo 2
                
                    printf("Changement de salon d'un utilisateur \n");

                    pthread_mutex_lock(&mutex);

                    char* nomSalonDemande = malloc(TAILLE_NOM_SALON*sizeof(char));
                    strcpy(nomSalonDemande, message1);

                    if (nomSalonDemande[strlen(nomSalonDemande)-1] == '\n') nomSalonDemande[strlen(nomSalonDemande)-1] == '\0';

                    nomSalonDemande = strtok(nomSalonDemande, " ");
                    nomSalonDemande = strtok(NULL, " ");

                    for (int i = 0; i < strlen(nomSalonDemande); i++) {
                        if (nomSalonDemande[i] == '\n') {
                            nomSalonDemande[i] = '\0';
                        }
                    }

                    printf("nom du salon: %s \n", nomSalonDemande);
                    
                    int idSalon = idSalonDemande(nomSalonDemande);

                    if (idSalon == -1) {
                        printf("Salon non trouvé\n");
                        send_message(tabClientStruct[index].dSC, "Salon non trouvé", strlen("Salon non trouvé"));
                        pthread_mutex_unlock(&mutex);
                        break;
                    }
                    tabClientStruct[index].idSalon = idSalon;

                    char* newMessage = malloc(4096*sizeof(char));
                    sprintf(newMessage,"%s", "@idSalon ");

                    char idSalonToChar[2];
                    sprintf(idSalonToChar, "%d", idSalon);
                    strcat(newMessage, idSalonToChar);

                    strcat(newMessage, "\n");

                    int sended = send_message(tabClientStruct[index].dSC, newMessage, tailleBufferReception);
                    if (sended == -1){
                        printf("Erreur lors de l'envoi du message de changement de salon à %s\n", tabClientStruct[index].pseudo);
                    }
                    pthread_mutex_unlock(&mutex);
                    printf("Changement de salon effectué\n");
                    break;
                }
                else if (cmd ==-10){ // on envoi la liste des salons disponibles 
                    printf("\nListe des salons disponibles :\n");
                    char* message = malloc(4096*sizeof(char));
                    strcat(message, "\nListe des salons disponibles :\n");
                    for (int j = 0; j < nbSalons; j++) {
                        
                        printf("Salon %d : %s , description: %s\n", j ,tabSalons[j].nomSalon, tabSalons[j].descriptionSalon);
                        char idSalon[2];
                        strcat(message, "Salon ");
                        sprintf(idSalon, "%d", j);
                        strcat(message, idSalon);
                        strcat(message, " : Nom: ");
                        strcat(message,tabSalons[j].nomSalon);
                        strcat(message, " , Descritpion: ");
                        strcat(message,tabSalons[j].descriptionSalon);
                        strcat(message, "\n");
                        
                    }
                    int sended = send_message(tabClientStruct[index].dSC, message, strlen(message));
                    if (sended == -1){
                        printf("Erreur lors de l'envoi du message à %s\n", tabClientStruct[index].pseudo);
                    }
                    break;
                }
                else if (cmd == -11){ // Création d'un nouveau salon
                    // @addSalon nomSalon -descriptionSalon - envoyé par ...
                    // On récupère le nom du salon
                    char* nomSalon = malloc(TAILLE_NOM_SALON*sizeof(char));
                    strcpy(nomSalon, message1);
                    nomSalon = strtok(nomSalon, " ");
                    nomSalon = strtok(NULL, " ");

                    // On récupère la description du salon
                    char* descriptionSalon = malloc(TAILLE_DESCRIPTION_SALON*sizeof(char));
                    strcpy(descriptionSalon, message1);
                    descriptionSalon = strtok(descriptionSalon, "-");
                    descriptionSalon = strtok(NULL, "-");

                    // On enlève les \n en trop à la fin des chaines
                    for (int i = 0; i < strlen(nomSalon); i++) {
                        if (nomSalon[i] == '\n') {
                            nomSalon[i] = '\0';
                        }
                    }
                    for (int i = 0; i < strlen(descriptionSalon); i++) {
                        if (descriptionSalon[i] == '\n') {
                            descriptionSalon[i] = '\0';
                        }
                    }

                    // On vérifie qu'il y a bien un nom et une description
                    printf("Nom du salon : %s\n", nomSalon);
                    printf("Description du salon : %s\n", descriptionSalon);

                    if (strncmp(nomSalon, DISPO, strlen(DISPO)) == 0){
                        pthread_mutex_lock(&mutex);
                        char* message = "Le nom du salon ne peut pas être 'DISPONIBLE'";
                        int sended = send_message(tabClientStruct[index].dSC, message, strlen(message));
                        if (sended == -1){
                            printf("Erreur lors de l'envoi du message à %s\n", tabClientStruct[index].pseudo);
                        }
                        pthread_mutex_unlock(&mutex);
                        break;
                    }

                    if (nomSalon == "-" || descriptionSalon[0] == ' ') {
                        pthread_mutex_lock(&mutex);
                        char* message = "Veuillez entrer un nom et une description pour le salon";
                        int sended = send_message(tabClientStruct[index].dSC, message, strlen(message));
                        if (sended == -1){
                            printf("Erreur lors de l'envoi du message à %s\n", tabClientStruct[index].pseudo);
                        }
                        pthread_mutex_unlock(&mutex);
                        break;
                    }

                    addNewSalon(index, nomSalon, descriptionSalon);
                    break;
                }
                else if (cmd == -12){ // On supprime un salon
                    // @suppSalon nomSalon/idSalon - envoyé par ...
                    // On récupère le nom du salon
                    printf("On va supprimer un salon \n");
                    char* nomSalon = malloc(TAILLE_NOM_SALON*sizeof(char));
                    strcpy(nomSalon, message1);
                    nomSalon = strtok(nomSalon, " ");
                    nomSalon = strtok(NULL, " ");

                    // On enlève les \n en trop à la fin des chaines
                    for (int i = 0; i < strlen(nomSalon); i++) {
                        if (nomSalon[i] == '\n') {
                            nomSalon[i] = '\0';
                        }
                    }

                    // On vérifie qu'il y a bien un nom et une description
                    printf("Nom du salon : %s\n", nomSalon);

                    if (nomSalon == "-") {
                        pthread_mutex_lock(&mutex);
                        char* message = "Veuillez entrer le nom ou l'identifiant du salon";
                        int sended = send_message(tabClientStruct[index].dSC, message, strlen(message));
                        if (sended == -1){
                            printf("Erreur lors de l'envoi du message à %s\n", tabClientStruct[index].pseudo);
                        }
                        pthread_mutex_unlock(&mutex);
                        break;
                    }

                    int idSalonD = idSalonDemande(nomSalon);

                    suppOldSalon(index, idSalonD);
                    break;
                }
                else if (cmd == i){ // On envoie le message privé
                    printf("message privé\n");
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
    
    printf("Sortie, déconnexion du client %s\n", tabClientStruct[index].pseudo);
    deconnecterClient(index);

    sem_post(&semaphore);
    free(args);
    pthread_exit(NULL);
}

//  fonction pseudo déjà utilisé
/**
* @brief Fonction qui permet de dire si un pseudo est déjà utilisé
* @param pseudo Le pseudo recherché 
* @return 0 si pas déjà utilisé, sinon 1
*/
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


//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                 MAIN                                                     //
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

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

    //Ajout du mutex
    pthread_mutex_init(&mutex, NULL);

    // initialisation des salons
    pthread_mutex_lock(&mutex);
    int z = 0;
    while(z < nbSalons ){
        if (z == 0){
            strcpy(tabSalons[z].nomSalon, "General");
            strcpy(tabSalons[z].descriptionSalon, "Le salon general commun.");
        }
        else{
            strcpy(tabSalons[z].nomSalon, DISPO);
        }
        z++;
    }
    pthread_mutex_unlock(&mutex);
    
    

    // on ouvre le fichier

    FILE* config =  fopen("./config.txt", "r");
    if (config == NULL){
        perror("Erreur lors de l'ouverture du fichier");
        exit(1);
    }

    // on calcul la taille
    int fsize;
    fseek(config, 0, SEEK_END);

    fsize = ftell(config);
    rewind(config);// remise du curseur de lecture à 0


    // si fichier vide on passe
    if (fsize != 0){
        // on récupère chaque ligne de fichier qu'on implémente dans le tableau initialisé de base
        int id;
        char* nom;
        char* description;

        char* ligne = malloc(256*sizeof(char));
        while (fgets(ligne, 256, config) != 0){
            id = atoi(strtok(ligne, "-"));
            nom = strtok(NULL, "-");
            description = strtok(NULL, "-");

            if (description[strlen(description-1)] == '\n') description[strlen(description-1)] = '\0';

            pthread_mutex_lock(&mutex);
            strcpy(tabSalons[id].nomSalon, nom);
            strcpy(tabSalons[id].descriptionSalon, description);
            pthread_mutex_unlock(&mutex);
        }
        

    }

    // on ferme le fichier 
    fclose(config);
       
    lengthTabClient = nbcli;

    // Ajout du sémaphore
    sem_init(&semaphore, 0, nbcli);


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
            printf("Pseudo client %d : %s\n", index, pseudo);

            for (int i = 0; i < strlen(pseudo); i++) {
                if (pseudo[i] == '\n') {
                    pseudo[i] = '\0';
                }
            }

            if (strncmp(pseudo, "__DÉCONNECTÉ__", 14) == 0 || strncmp(pseudo, "goTo", 4) == 0 || strncmp(pseudo, "getSalons", 9) == 0 || strncmp(pseudo, "suppSalon", 9) == 0  || strncmp(pseudo, "addSalon", 8) == 0 || strncmp(pseudo, "getFile", 7) == 0 || strncmp(pseudo, "fichierServ", 11) == 0 || strncmp(pseudo, "sendFile", 8) == 0 || strncmp(pseudo, "@fin", 4) == 0 ||strncmp(pseudo, "list", 4) == 0 ||  strncmp(pseudo, "help", 4) == 0 ||   strncmp(pseudo, "fin", 3) == 0 || pseudoDejaUtilise(pseudo) == 1){
                
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
