# Bomberman - Programmation Reseau PR6
Projet de Programmation Reseau - Bomberman.

## DESCRIPTION
Jeu de Bomberman en réseau local jouable à 4 avec gestions des clients et serveurs. Voir [projet.pdf](./projet.pdf) pour plus d'informations.

## POUR TESTER
### COMPILATION
Pour compiler le projet :
```bash
make
```

Cela va produire deux executables  `global_server` ainsi que `client` au répertoire courant.

### EXÉCUTION
Pour exécuter le projet, il faut lancer 4 clients avec la syntaxe suivante :
```bash
./client "<port>"
```
`<port>` étant le port de connexion.
Et il faut lancer le serveur en le lancant avec la syntaxe suivante : 
```bash
./global_server "<port>"
```
