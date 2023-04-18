# Sprint 1

## 1. Le protocole de communication entre les clients et le serveur (diagramme de séquence UML).

![diagramme de séquence UML]()

## 2. L’architecture de l’application.

## 3. Les difficultés rencontrées, les choix faits pour les surmonter, et les problèmes qui persistent.

## 4. La répartition du travail entre les membres du groupe.

Nous nous sommes organisés de la manière suivante :

- v0 : Travail collaboratif, en simultané.
- v1 : Compréhension des threads puis conception du code 

## 5. Comment compiler et exécuter le code.

Le code se compile de la manière suivante :

```bash
gcc -o /out/[votre_version]/serveur /sprint1/[votre_version]/serveur.c
gcc -o /out/[votre_version]/client /sprint1/[votre_version]/client.c
```
### Sujet 1 :

#### V0 :
Ensuite, lancez d'abord le serveur :
```bash
./out/v0/serveur le_port_souhaité
```
Puis lancez les deux clients sur des ternimaux différents :
```bash
./out/v0/client adresse_ip_du_serveur le_port 0
./out/v0/client adresse_ip_du_serveur le_port 1
```
Le client 0 envera d'abord son message puis recevra le message du client 1.\
Le client 1 recevra le message du client 0 et envera le sien.

Co-authored-by: HarumiaStar <HarumiaStar@users.noreply.github.com>
Co-authored-by: charleneMrcp <charleneMrcp@users.noreply.github.com> 