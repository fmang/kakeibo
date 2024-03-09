Dépendances
-----------

Pour le scanner de reçus :

- CMake 3.23.
- Un compilateur C++20.
- OpenCV 4.

Côté Python :

- scikit-learn
- fastapi
- uvicorn

Compilation
-----------

CMake permet de compiler l’exécutable receipt-scanner, requis pour extraire les
informations des photos de reçu. Une fois complié, il doit être accessible
depuis le PATH de l’application web.

Application web
---------------

Pour pouvoir utiliser toutes les fonctionnalités JavaScript, il faut un
certificat. Pour ça, créer le dossier ssl/ et y lancer la commande `openssl req
-x509 -newkey rsa:2048 -keyout key.pem -out cert.pem -sha256 -days 365 -nodes`.
