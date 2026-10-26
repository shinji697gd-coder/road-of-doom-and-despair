# Validation bêta.5

## Automatisé

- 75 niveaux et IDs uniques, ordre critique préservé, Nullscapes dernier.
- Ancien format RDOD 1 et sauvegardes des bêta précédentes inchangés.
- Nouveau format : aller-retour, séparation copie/original, unicité des associations, rejet non destructif de schémas inconnus, mauvais IDs, objectifs invalides et texte superflu.
- Premier record observé sans date historique inventée ; augmentation seulement ; objectif de run couvert et non couvert.
- Compteur : un seul départ par tentative, deux callbacks de mort/reset sans double compte, practice séparé, pause et longue interruption exclues.
- Snapshot : dix versions conservées, doublon de nom refusé, nom invalide refusé, fichier personnel non supprimé.
- Tests existants de dimensions, routage réseau et recherches STARTPOS.
- Compilation Windows x64 avec Geode 5.11.2 pour GD 2.2081.

## À tester dans GD 2.2081

L'environnement de build ne permet pas d'exécuter le jeu. Ne pas considérer les hooks ou les nouveaux écrans comme validés en jeu après une simple compilation.

### Non-régression

- Ouvrir / fermer dix fois et parcourir les huit pages avec les mods de la vidéo.
- PARCOURS conserve ses cellules et VIEW natifs. STARTPOS sans copie conserve la recherche native validée en bêta.4.
- Tester navigation rapide, erreurs réseau, délai de 15 secondes, fermeture pendant chargement et retour depuis les résultats.
- Les onglets ne recouvrent ni la liste ni les boutons, aux petites résolutions et avec les textures / miniatures installées.

### Copies préférées

- STARTPOS → Forest Temple → VIEW → copie → Voir seulement : aucune association.
- Même parcours → Choisir : revenir puis VIEW ouvre directement la fiche de cette copie.
- i → STARTPOS : vérifier l'ID, le changer, le retirer, relancer la recherche et redémarrer GD.
- Un ID original ou une copie déjà liée ailleurs est refusé. Un ID inconnu / supprimé échoue proprement au chargement ; l'utilisateur peut le remplacer.
- Les recherches natives ouvertes hors RDoD n'affichent pas le choix de copie.
- REPERES MANUELS conserve les anciens noms / pourcentages, avec ajout, modification et suppression.

### Sessions et historique

- Faire un nombre connu d'essais normaux sur un original, quitter et comparer aux stats.
- Alterner normal, practice et startpos/test ; vérifier les colonnes et l'absence de double compte après restart, mort dual et changement de mode.
- Laisser le jeu en pause une minute : ce temps ne doit pas être ajouté ; vérifier aussi pause par perte de focus, resume, completion et abandon d'une tentative vivante.
- Jouer la copie liée : elle apparaît dans COPIES et n'augmente pas les stats de l'original dans ORIGINAL.
- Tester un niveau hors parcours, un replay, l'éditeur et un platformer : aucun suivi.
- Vérifier les comptes avec FPS élevé, speedhack, noclip et le mod de startpos installé ; ce suivi n'est pas une validation anti-triche.
- Faire un nouveau record normal puis revenir : une observation datée apparaît, pas de régression après un essai inférieur. Les anciens records ne reçoivent pas de fausse date.
- Désactiver Stats automatiques de session, rouvrir un niveau et confirmer l'arrêt du suivi.
- Vérifier la sauvegarde après pause / quitter et après crash simulé (perte possible de moins de 30 secondes non sauvegardées).

### Objectifs

- Choisir 70 % en normal ; il est atteint si le meilleur normal lu atteint 70.
- Choisir 40–100 ; ajouter 41–100 ne suffit pas, 40–100 ou 39–100 suffit.
- Les runs sont manuels : ne pas annoncer de reconnaissance automatique d'un 40–100 joué.
- Modifier / retirer l'objectif et redémarrer ; le choix du niveau objectif global reste indépendant.

### Sauvegardes

- Installer sur la bêta.4 : notes, favoris, runs, repères et progression restent présents.
- Exporter, restaurer l'export complet, annuler une restauration et importer un ancien export schéma 1 (nouvelles données conservées).
- Vérifier les versions automatiques après modifications puis après dix minutes de jeu ; restaurer une ancienne date après confirmation.
- Rejeter des données tronquées ou corrompues sans modifier les données actuelles.
- Vérifier le fichier unique de secours avant chaque restauration, y compris avec une valeur Geode initialement illisible.
- Une erreur d'écriture de la copie de secours annule l'import. Les meilleurs normaux ne diminuent jamais.

## Toujours hors périmètre

Connexion à ChatGPT, suivi externe, capture native / moteur TEST isolé, détection automatique des segments de run, récupération rétroactive des anciens temps et tentatives, broker général entre tous les mods et remplacement de la capture téléchargée pendant le build.
