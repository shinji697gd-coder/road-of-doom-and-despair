# Sauvegardes locales — bêta.6

La bêta.6 conserve exactement les clés, formats et champs sérialisés de bêta.5. Les noms affichés sont résolus depuis le catalogue local des 75 niveaux ; aucun accès réseau ni migration n'est nécessaire pour les anciens records.

## Compatibilité

training-v1 conserve strictement le format RDOD 1 des bêta.1 à bêta.4 : meilleur pourcentage normal local, favori, statut, notes, runs manuels, repères et objectif global. Aucune migration destructive.

training-extra-v1 utilise RDX 1. En-tête : numéro de session suivant, nombre de niveaux, nombre de sessions. Chaque niveau contient copie préférée, record normal de référence, type / bornes / état d'objectif précis et historique daté d'observation. Une session contient son identifiant, les IDs original et joué, début / fin, essais normaux et practice/test, temps actif par mode.

Les deux décodeurs valident dans un candidat temporaire avant affectation. Les nouvelles données sont limitées à 75 originaux connus, 100 observations de record par original (croissantes), 500 sessions, 2 MB de texte, des valeurs finies et des bornes explicites. Un ID de copie ne peut pas être un original du parcours ni être partagé par deux originaux. Le source ID d'une session est conservé même si la copie préférée est modifiée plus tard.

Un format inconnu ou invalide désactive l'écriture de la donnée correspondante sans écraser le texte original. Les anciennes clés completed-ID ne sont pas modifiées. Aucune progression native GD n'est écrite.

## Export et restauration

L'enveloppe JSON 2 contient schema: 2, payload: "<RDOD 1...>", extras: "<RDX 1...>". L'import valide les deux avant mutation. L'enveloppe JSON 1 antérieure reste acceptée : elle remplace payload seulement, les nouvelles données restent intactes.

Avant import, un fichier unique training-before-import-[microsecondes UTC].json conserve les deux anciennes valeurs, y compris le texte brut d'une donnée illisible. Si cette écriture échoue, l'import est annulé. Le meilleur local est fusionné par maximum avec les records actuels et ceux de GD ; il ne diminue pas.

## Versions automatiques

Avant une écriture, une version complète des valeurs persistées précédentes peut être créée dans le sous-dossier auto-backups du dossier Geode du mod. Un maximum de 10 minutes limite la fréquence des nouvelles versions (une version au premier changement, puis au plus une toutes les 10 minutes). Aucune copie invalide ne remplace une version valide.

Chaque version utilise auto-[microsecondes UTC].json, un fichier temporaire puis un renommage. Après écriture réussie, seuls les anciens fichiers réguliers dont le nom correspond exactement au motif géré sont retirés, pour conserver les dix derniers. Aucun effacement récursif ni traitement des fichiers d'un autre nom. OUTILS liste et restaure ces versions après confirmation.

Les sauvegardes restent locales. Aucune connexion à ChatGPT ni autre service n'est ajoutée.
