# Validation beta.4

Corrections issues de la vidéo MedalTVGeometryDash20261025165726788.mp4 : pages suivantes bloquées sur « Une autre requete est en cours » et onglet STARTPOS remplacé par le raccourci de recherche demandé. L'interface française, les 75 niveaux et le format de sauvegarde sont conservés.

## Contrôles automatisés

- Parcours : 75 identifiants uniques, ordre critique inchangé, Nullscapes dernier.
- Sauvegardes C++ : aller-retour, schéma invalide, limites, doublons, runs invalides et rejet sans écrasement.
- Dimensions : séparation titre / onglets / recherche / contenu / pied de page aux quatre tailles d'écran existantes.
- Routage : délégué précédent non nul, changement de délégué par un autre mod, réponse annulée, réponses hors ordre, ancienne réponse après remplacement et nouvelle demande depuis un callback.
- Recherche : Acu, Forest Temple tronqué comme dans la vidéo et nom long.
- Compilation et paquet Windows avec Geode 5.11.2 dans GitHub Actions.

## À valider dans Geometry Dash 2.2081

Ces tests ne sont pas exécutables dans cet environnement. Ne pas fusionner dans main avant validation.

- Parcourir les huit pages de PARCOURS avec les mêmes mods que dans la vidéo. Les cellules et VIEW doivent charger après la page 1 sans le message de blocage.
- Ouvrir STARTPOS sans aucun repère enregistré : les 75 niveaux doivent être proposés avec les mêmes cellules natives que PARCOURS.
- STARTPOS → Forest Temple → VIEW : ouvrir les résultats natifs pour « Forest Temple startp », avec les copies d'entraînement. Ne pas ouvrir directement le niveau original ou sélectionner automatiquement une copie.
- Refaire le test avec Acu et un nom long ; la disponibilité et le nommage des copies dépendent des niveaux publiés sur GD.
- Dans PARCOURS, VIEW d'une cellule chargée ouvre toujours la fiche du bon niveau original.
- Revenir des résultats STARTPOS, parcourir une autre page, utiliser VIEW puis changer d'onglet. La recherche, le filtre et la page restent propres à chaque vue.
- Pendant le chargement ou hors connexion, VIEW reste présent : recherche de copies dans STARTPOS, recherche par ID dans PARCOURS.
- Changer rapidement de page, chercher ou filtrer pendant une requête ; une réponse ancienne ne doit pas remplacer la sélection actuelle ou PROGRESSION. La page courante doit ensuite charger automatiquement.
- Fermer le mod pendant une requête, lancer une recherche native différente, puis attendre la réponse précédente. Elle ne doit pas remplacer les résultats natifs ni laisser de pointeur vers le menu fermé.
- Cliquer plusieurs fois sur ACTUALISER, tester une erreur réseau et le délai de 15 secondes, puis réessayer après reconnexion.
- Ouvrir et fermer dix fois ; vérifier les textures, miniatures, contrôles et le bouton i sans chevauchement.
- Ajouter notes, favori, objectif, run et repère manuel ; redémarrer GD et vérifier la conservation des anciennes sauvegardes.
- Exporter, annuler une restauration, restaurer, rejeter un fichier corrompu et vérifier le fichier de secours.
- Confirmer que la progression native est seulement lue et que le meilleur pourcentage local ne diminue pas.
- Vérifier Integrated Demonlist, BetterInfo, Happy Textures, Level Thumbnails et le mod de startpos visible dans la vidéo.

## Fonctions du rapport restant à développer

- Les repères manuels restent des noms / pourcentages. La capture native et le moteur TEST isolé ne sont pas implémentés. Le nouvel onglet STARTPOS utilise les recherches GD demandées par l'utilisateur.
- Statistiques automatiques de session, synchronisation en arrière-plan et dates de runs.
- Le routage est limité aux clés de requêtes RDoD ; il ne remplace pas un broker réseau général entre tous les mods. Les conflits de mods doivent être validés en jeu.
- Les anciennes coches DONE restent sous leurs clés historiques et ne sont pas comptées comme des victoires GD vérifiées.
- Le téléchargement existant de la capture pendant le build reste en place ; une capture personnelle fournie doit le remplacer.
