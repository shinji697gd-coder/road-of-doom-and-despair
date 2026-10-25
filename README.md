# The Road of Doom and Despair — bêta.3

Mod pour Windows, Geometry Dash 2.2081 et Geode 5.11.2.

L'interface est en français. Les termes de jeu comme startpos, runs, grind et VIEW ainsi que les noms des niveaux sont conservés. Les libellés en jeu utilisent des caractères simples compatibles avec la police de GD.

Les 75 niveaux du parcours et leurs boutons VIEW natifs sont conservés. Les onglets PARCOURS, STARTPOS et PROGRESSION se trouvent sous le titre. La pagination et le bouton ACTUALISER sont en bas de la fenêtre.

Le bouton **i** à côté d'un niveau ouvre sa fiche :

- **APERCU** : record normal, statut, favori, objectif et notes. Les notes sont enregistrées en changeant de page ou en fermant la fiche.
- **RUNS** : ajoute un segment, sélectionne une ligne pour le modifier ou supprime le run sélectionné.
- **STARTPOS** : enregistre un nom et un pourcentage de départ. Sélectionne une ligne pour la modifier. NOUVEAU vide le formulaire et SUPPRIMER retire le startpos sélectionné.

L'onglet STARTPOS affiche les niveaux ayant des repères enregistrés. CHOISIR UN NIVEAU ou le filtre TOUS permet d'en ajouter un premier. Ces repères contiennent un pourcentage ; le lancement du niveau depuis un startpos reste à développer.

**OUTILS** regroupe l'export, la restauration et les paramètres du mod. L'export crée training-backup.json dans le dossier de sauvegarde du mod. La restauration valide le fichier, demande confirmation et conserve une copie de secours des données actuelles. Le mode sécurisé est réservé au futur moteur expérimental ; cette version n'ajoute aucun hook expérimental de gameplay.

La progression est lue depuis les niveaux sauvegardés de GD et les objets de niveau chargés. Le meilleur pourcentage local ne diminue pas. Les anciennes coches manuelles DONE restent conservées mais ne comptent pas comme des victoires natives. La recherche, le filtre et la page sont conservés séparément pour chaque vue.

## Installation et sauvegardes

Remplace l'ancien fichier .geode par la bêta.3. Les sauvegardes des bêta.1 et bêta.2 restent compatibles : aucun changement du format ni des identifiants de stockage.

## Vérification

    python scripts/check_roadmap.py
    c++ -std=c++20 tests/training_data.cpp -o training_data && ./training_data
    c++ -std=c++20 tests/layout.cpp -o layout_tests && ./layout_tests

Les contrôles de dimensions vérifient la séparation des zones d'interface et leur adaptation à la taille d'écran. Ils ne remplacent pas un test de rendu dans GD avec les mods et textures installés. La compilation Windows s'exécute dans GitHub Actions.

Voir les [tests en jeu et fonctions restantes](docs/BETA_TEST_MATRIX.md) et le [format de sauvegarde](docs/SAVE_SCHEMA.md). Ne pas fusionner dans main avant validation en jeu.

## Crédits

L'interface et la recherche s'inspirent d'Integrated Demonlist (MIT) ; voir LICENSE-IntegratedDemonlist. Le build existant télécharge une capture de Nullscapes via scripts/prepare_logo.py. Le jeu, le niveau et l'image appartiennent à leurs titulaires respectifs.
