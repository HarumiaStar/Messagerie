# Messagerie

#Sprint 1 : Semaine du 10.04. 3 séance encadrée : Rendu le 23/04
    #Partie 1 :

Sujet (séance 1) : Un serveur relaie des messages textuels entre deux clients 
1 programme serveur et 1 programme client, lancé deux fois (deux processus distincts)
1 seul processus/thread serveur pour gérer les 2 clients, qui envoient leurs messages à tour de rôle (client 1 : write puis read, et client 2 : read puis write)
l’échange de messages s’arrête lorsque l’un des clients envoie le message « fin ». Ceci n’arrête pas le serveur, qui peut attendre la connexion d’autres clients.
On utilisera le protocole TCP.
