# DAAP-Smartcard
Lecture/Ecriture de carte SLE4442 pour le distributeur automatique d'argent de poche

Projet relatif au [Distributeur Automatique d'Argent de Poche](https://github.com/richard-fagot/distributeur-argent-de-poche) permettant d'écrire sur une carte à puce SLE4442 les données nécessaires au bon fonctionnement du distributeur :
 - Nom de l'enfant ;
 - Code secret de l'enfant ;
 - Montant de l'argent de poche à distribué (en centime).

 **Attention**, les cartes SLE4442 sont des cartes protégées en écriture par un code pin. Le sketch développé ici suppose que le code pin de la carte est le code par défaut (0xFFFFFF).