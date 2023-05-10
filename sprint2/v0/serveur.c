#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <pthread.h>
#include <semaphore.h>
#include <signal.h>

#define TAILLE_PSEUDO 32

typedef struct {
    int dSC;
    char pseudo[TAILLE_PSEUDO];
} dSClient;

dSClient* tabClientStruct;
int tailleBufferReception;
int lengthTabClient;

typedef struct {
    long index;
} thread_args;

pthread_mutex_t mutex;
sem_t semaphore;

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

        printf(" strcmp fin : %d \n", strncmp(cmd,"fin", 3));
        printf(" strcmp list : %d \n", strncmp(cmd,"list", 4));
        printf(" strcmp help : %d \n", strncmp(cmd,"help", 4));
        if (strncmp(cmd,"fin", 3) == 0) return -2; //on renvoie -2 si truc == fin
        else if (strncmp(cmd, "list", 4) == 0) return -4; //on renvoie -4 si truc == list
        else if (strncmp(cmd, "help", 4) == 0) return -5; //on renvoie -5 si truc == help
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


void* communication(void* arg){

    thread_args *args = (thread_args *) arg;
    long index = args->index;

    int sortie = 0;
    while (sortie == 0){
        
        // Reception de la réponse client Writer: 
        int recvTaille = recv(tabClientStruct[index].dSC, &tailleBufferReception, sizeof(int), 0);
        if (recvTaille == 0) {
            pthread_mutex_lock(&mutex);
            printf("Client %ld déconnecté (recvTaille =0) \n", index);
            tabClientStruct[index].dSC = -1;
            strncpy(tabClientStruct[index].pseudo, "__DÉCONNECTÉ__", TAILLE_PSEUDO);
            pthread_mutex_unlock(&mutex);
            break;
        }
        if (recvTaille == -1){
            perror("Taille non reçue\n");
            break;
        }
        printf("la taille %d\n", tailleBufferReception);
        

        // Reception message
        char* message1 = malloc(tailleBufferReception*sizeof(char));
        int recvMessage = recv(tabClientStruct[index].dSC, message1, tailleBufferReception, 0);
        if(recvMessage == 0){
            pthread_mutex_lock(&mutex);
            printf("Client %ld déconnecté (recvMessage = 0) \n", index);
            tabClientStruct[index].dSC = -1;
            strncpy(tabClientStruct[index].pseudo, "__DÉCONNECTÉ__", TAILLE_PSEUDO);
            pthread_mutex_unlock(&mutex);
            break;
        }
        if(recvMessage == -1){
            perror("Réponse non reçue");
            break;
        }
        printf("Réponse reçue : %s\n", message1);

        long i = 0;
        while (i < lengthTabClient && sortie == 0){
            if (i != index){
                int cmd = commande(message1);
                if (cmd == -1){ // On envoie le message à tout le monde
                    pthread_mutex_lock(&mutex);
                    if (tabClientStruct[i].dSC != -1) {
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
    printf("Sortie : %d\n", sortie);
    printf("Sortie, déconnexion du client %s %ld\n", tabClientStruct[index].pseudo ,index);
    pthread_mutex_lock(&mutex);
    close(tabClientStruct[index].dSC);
    strncpy(tabClientStruct[index].pseudo, "__DÉCONNECTÉ__", TAILLE_PSEUDO);
    tabClientStruct[index].dSC = -1;
    pthread_mutex_unlock(&mutex);
            

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

    if (argc != 3)
    {
        perror("./serveur port nbcli");
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


    int nbcli = atoi(argv[2]);
    pthread_t* thread = (pthread_t*) malloc(nbcli*sizeof(int));
    tabClientStruct = malloc(nbcli * sizeof(dSClient));

    lengthTabClient = nbcli;

    // Ajout du sémaphore
    sem_init(&semaphore, 0, nbcli);

    //Ajout du mutex
    pthread_mutex_init(&mutex, NULL);

    pthread_mutex_lock(&mutex);
    for (int i = 0; i < nbcli; i++){
        tabClientStruct[i].dSC = -1;
        strcpy(tabClientStruct[i].pseudo, "__DÉCONNECTÉ__");
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
                printf("Client %ld déconnecté \n", index);
                continue;
            }
            if(recvPseudo == -1){
                perror("Pseudo non reçue");
                continue;
            }
            printf("Pseudo client %ld : %s\n", dSC, pseudo);

            if (pseudo[strlen(pseudo)-1] == '\n') 
            pseudo[strlen(pseudo)-1] = '\0';

            if (strncmp(pseudo, "__DÉCONNECTÉ__", 14) == 0 ||strncmp(pseudo, "@fin", 4) == 0 ||strncmp(pseudo, "list", 4) == 0 ||  strncmp(pseudo, "help", 4) == 0 ||   strncmp(pseudo, "fin", 3) == 0 || pseudoDejaUtilise(pseudo) == 1){
                char* messagePseudo = "Pseudo déjà utilisé ou pseudo invalide";
                send_message(dSC, messagePseudo, strlen(messagePseudo));

                // envoyer @fin au client pour qu'il se déconnecte (avec la taille)
                char* messageFin = "@fin";
                send_message(dSC, messageFin, strlen(messageFin));

                sem_post(&semaphore);
                               
                continue;
            }

            printf("je passe ici \n");
    
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
