# Messagerie

## Sprint 1 : Semaine du 10.04. 3 séance encadrée : Rendu le 23/04
### Partie 1 :

### Sujet (séance 1) : Un serveur relaie des messages textuels entre deux clients 
1 programme serveur et 1 programme client, lancé deux fois (deux processus distincts)
1 seul processus/thread serveur pour gérer les 2 clients, qui envoient leurs messages à tour de rôle (client 1 : write puis read, et client 2 : read puis write)
l’échange de messages s’arrête lorsque l’un des clients envoie le message « fin ». Ceci n’arrête pas le serveur, qui peut attendre la connexion d’autres clients.
On utilisera le protocole TCP.

### Sujet (séance 2, 3 et 4) : Un serveur et un client multi-threadés

### v1 :
Utiliser le multi-threading pour gérer l’envoi de messages dans n’importe quel ordre (on ne sera plus contraint par le fait que c’est le premier client qui se connecte qui envoie un message en premier et attend que l’autre lui écrive un message pour pouvoir envoyer un autre message) :
programme client : 1 processus pour la saisie (avec fgets) et l’envoi du message au serveur 
et 1 autre processus pour la réception des messages du serveur et leur affichage (avec puts)
soit le programme principal plus un thread minimum

programme serveur : 1 thread pour relayer des messages du client 1 vers le client 2 et 1 autre pour relayer les messages du client 2 vers le client 1, 
pour faciliter le passage en v2 on pourra appeler la même fonction en créant le thread en passant en argument le descripteur et en interrogeant le tableau des clients

### v2 :
Mise en place d’un serveur qui puisse gérer n clients, pour cela mettre en place un tableau partagé pour stocker leurs identifiants sockets, par défaut, les messages arrivant depuis un client sont relayés à tous les autres présents. ( rappel : les descripteurs clients peuvent être récupérés via la fonction accept )
Côté serveur : 1  thread par client pour écouter les messages qui proviennent du client et les diffuser vers tous les autres clients. 
Côté client : rien ne change par rapport à la première version du client multithreadé. 

La déconnexion sur le client/serveur multithreadé n’est pas obligatoire pour ce sprint, elle le sera pour le prochain
