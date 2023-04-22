# Sprint 1


## 1. Le protocole de communication entre les clients et le serveur (diagramme de séquence UML).

![diagramme de séquence UML]()

---

## 2. L’architecture de l’application.

---

## 3. Les difficultés rencontrées, les choix faits pour les surmonter, et les problèmes qui persistent.

---

## 4. La répartition du travail entre les membres du groupe.

Nous nous sommes organisés de la manière suivante :

- v0 : Travail collaboratif, en simultané.
- v1 : Compréhension des threads puis conception du code 
- v2 : 

---

## 5. Comment compiler et exécuter le code.

Grâce à un Makefile, le code se compilera et s'executera en fonction des paramètres utilisés de la manière suivante :

```bash
make serveur VERSION= PORT=
make client VERSION= ADRESSEIP= PORT=
```

### Sujet 1 :

#### V0 :
Lancez d'abord le serveur :
```bash
make serveur VERSION=v0 PORT=3000
```
Puis lancez les deux clients sur des ternimaux différents :
```bash
make client VERSION=v0 ADRESSEIP=127.0.0.1 PORT=3000 ORDRE=0
make client VERSION=v0 ADRESSEIP=127.0.0.1 PORT=3000 ORDRE=1
```
>Le client 0 envera d'abord son message puis recevra le message du client 1.  
Le client 1 recevra le message du client 0 et envera le sien.  
Leur communication se terminera si l'un des deux envoie "fin". 

#### V1 :
Lancez d'abord le serveur :
```bash
make serveur VERSION=v1 PORT=3000
```
Puis lancez les deux clients sur des ternimaux différents avec sur chacun des terminaux :
```bash
make client VERSION=v1 ADRESSEIP=127.0.0.1 PORT=3000
```
>Le serveur ne se fermera pas et attendera de nouvelle connection.  
Les clients pourront échanger des messages sans contrainte d'ordre.  
Leur communication se terminera si l'un des deux envoie "fin". 

#### V2 :
Lancez d'abord le serveur :
```bash
make serveur VERSION=v2 PORT=3000 NBCLI=2
```
Puis lancez les n clients sur des ternimaux différents avec sur chacun des terminaux :
```bash
make client VERSION=v2 ADRESSEIP=127.0.0.1 PORT=3000
```
> Le serveur attendra les connections des n clients avant de relier les messages.  
Les clients pourront échanger des messages sans contrainte d'ordre.  
La communication se terminera si l'un des deux envoie "fin". 

---

Co-authored-by: HarumiaStar <HarumiaStar@users.noreply.github.com>  
Co-authored-by: charleneMrcp <charleneMrcp@users.noreply.github.com> 