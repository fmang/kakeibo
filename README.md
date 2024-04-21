Kakeibo
=======

Ce projet vise à fournir une application web conviviale pour enregistrer les
dépenses et les revenus de mon foyer.

Compilation
-----------

Dépendances pour le scanneur de reçus :

- CMake 3.23.
- Un compilateur C++20.
- OpenCV 4.

CMake permet de compiler l’exécutable receipt-scanner, requis pour extraire les
informations des photos de reçu. Une fois compilé, il doit être accessible
depuis le PATH de l’application web.

Dépendances côté Python :

- scikit-learn,
- fastapi,
- uvicorn,
- python-multipart.

Données
-------

Pour entrainer le modèle, remplir pour chaque lettre le dossier letters/x (où x
est la lettre à reconnaitre) d’images de cette lettre. Ensuite, lancer la
commande suivante :

	receipt-scanner --compile letters | python -m kakeibo.classifier --train kakeibo/letters.model

Pour détecter les magasins, se référer au module kakeibo.stores.

Application web
---------------

Pour pouvoir utiliser toutes les fonctionnalités JavaScript, il faut un
certificat. Pour ça, créer le dossier ssl/ et y lancer la commande `openssl req
-x509 -newkey rsa:2048 -keyout key.pem -out cert.pem -sha256 -days 365 -nodes`.

Pour lancer l’API en développement, utiliser `python -m kakeibo.api`.
Autrement, lancer le module kakeibo.api avec uvicorn.

L’application peut être montée sur n’importe quel domaine, y compris avec un
chemin relatif.

Gestion des utilisateurs
------------------------

Seuls deux utilisateurs sont admis : riku et anju. Ils peuvent avoir autant de
clés d’API que nécessaire. N’importe quelle chaine peut servir de clé, mais
`python -c 'import secrets; print(secrets.token_urlsafe())'` permet d’en
générer une sûre. Enfin, ajouter au fichier api-keys une ligne avec la clé
suivie du nom d’utilisateur, séparés par un blanc.

Pour utiliser l’application depuis un navigateur, utiliser la route
`/#utilisateur:clé`.

Pour installer l’application web progressive, aller sur la route
`/welcome.html#utilisateur:clé` et installer l’application de là. Sur iOS, il
faut aller dans le menu *Partager* puis *Ajouter à l’écran d’accueil*.
