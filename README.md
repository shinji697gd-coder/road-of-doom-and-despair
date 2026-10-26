# The Road of Doom and Despair — bêta.6

Windows x64, Geometry Dash 2.2081, Geode 5.11.2. Interface française, termes de jeu et noms des 75 niveaux conservés.

La bêta.6 corrige les vues de stats signalées dans les captures : les noms remplacent les IDs dans les sessions et les records. Le suivi commence lorsque le PlayLayer est réellement prêt, même avec création différée des objets. Un échantillonnage Cocos à 0,1 seconde et les événements de départ / reset / pause / fin remplacent la dépendance au chemin physique postUpdate. Les données et sauvegardes de bêta.5 restent compatibles ; les sessions qui n'avaient pas été enregistrées ne sont pas inventées.

## Copie startpos préférée

Dans STARTPOS, VIEW ouvre toujours la recherche GD tant qu'aucune copie n'est choisie. Dans ces résultats uniquement, VIEW propose « Voir seulement » ou « Choisir ». Choisir associe cette copie au niveau du parcours puis ouvre sa fiche native. Les prochains VIEW de STARTPOS ouvrent directement cette copie après chargement de ses informations.

Le bouton i → STARTPOS permet aussi de saisir l'ID d'une copie, de la retirer ou de relancer la recherche. Un original du parcours ne peut pas être enregistré comme copie ; une copie ne peut pas appartenir à deux originaux. Les anciens noms / pourcentages restent dans REPERES MANUELS. Une recherche de nom reste limitée à 20 caractères comme dans GD.

La disponibilité d'une copie dépend de GD. En cas d'ID supprimé ou d'erreur réseau, le chargement affiche une erreur après 15 secondes ; retire ou remplace alors son ID. « Voir seulement » laisse VIEW en recherche.

## Stats de session et historique

PROGRESSION → STATS affiche toutes les sessions. i → APERCU → STATS limite les données à un niveau.

- ORIGINAL / COPIES sépare les sources par ID. La copie doit être explicitement liée avant de jouer ; sa progression ne devient jamais celle de l'original.
- Les essais normaux et les essais practice/test/startpos sont séparés. Une tentative commence au premier tick actif, pas à chaque appel de reset ou de mort.
- Le temps est du temps actif réel, mesuré avec une horloge monotone. Pauses et interruptions de plus d'une seconde ne sont pas ajoutées. Le suivi exclut éditeur, replays et platformer.
- Le suivi débute avec cette version : aucun ancien temps de jeu ni anciennes tentatives ne sont reconstitués. Les 500 sessions les plus récentes sont conservées.
- RECORDS affiche les augmentations du meilleur pourcentage normal observées après l'activation du suivi. Le premier pourcentage sert de référence sans date inventée. Une date est une date d'observation sur cet appareil, pas une preuve de la date exacte du record.
- Chaque ligne affiche le nom du niveau du parcours. Dans COPIES, la mention « copie startpos » identifie la source ; changer de copie préférée ne déplace pas les anciennes sessions.
- Une vue vide indique maintenant si le suivi est désactivé, en lecture seule ou si une copie doit être liée avant de jouer.
- Les stats sont sauvegardées au début, après la première tentative, toutes les 30 secondes, à la pause, à la fin et en quittant. Un arrêt brutal peut perdre la dernière tranche non sauvegardée.

Les hooks lisent les événements de jeu ; ils n'écrivent ni progression, ni physique, ni compteurs de tentatives de GD. Ce suivi n'est pas un système anti-triche et les interactions avec les autres mods nécessitent une validation en jeu. Le réglage Stats automatiques de session désactive le suivi ; changer ce réglage prend entièrement effet au prochain lancement de niveau.

## Objectifs précis

i → APERCU → OBJECTIF % permet de viser un record normal entier (ex. 70 %) ou un run (ex. 40–100). Le choix du niveau objectif global reste indépendant.

Le record se valide à partir de la progression normale lue. Un objectif de run se valide quand tu ajoutes un run qui le couvre dans RUNS, ou s'il était déjà enregistré. Les runs restent saisis manuellement dans cette bêta ; aucune détection automatique des segments n'est annoncée.

## Sauvegardes

Les sauvegardes existantes training-v1 restent inchangées. Copies, objectifs précis, sessions et historique utilisent une seconde clé indépendante training-extra-v1. Une donnée illisible est conservée et sa réécriture est bloquée.

Les sauvegardes automatiques conservent jusqu'à 10 versions complètes dans auto-backups, avant modification puis au plus une nouvelle version toutes les 10 minutes lorsqu'une écriture a lieu. Elles n'effectuent aucun envoi réseau. OUTILS → SAUVEGARDES AUTO permet de choisir une date, avec confirmation avant restauration.

OUTILS permet toujours d'exporter training-backup.json et de restaurer ce fichier. Les exports de cette version utilisent une enveloppe complète de schéma 2. Les anciens exports de schéma 1 restent importables ; ils remplacent uniquement les anciens champs et conservent les nouvelles données. Chaque restauration crée un fichier training-before-import-[horodatage].json contenant les deux données précédentes, même si une valeur était illisible. Le meilleur pourcentage normal ne diminue pas.

## Vérification

Les tests de parcours, données, dimensions, routage, recherches, sessions et rotation des fichiers tournent dans GitHub Actions avant la compilation Windows. Ils ne remplacent pas les tests dans GD avec les textures et autres mods installés.

Voir [la validation en jeu](docs/BETA_TEST_MATRIX.md) et [les formats de sauvegarde](docs/SAVE_SCHEMA.md). Développement sur beta/v1.0.0-beta.1 ; aucune fusion dans main avant validation de cette nouvelle bêta.

## Crédits

L'interface et la recherche s'inspirent d'Integrated Demonlist (MIT) ; voir LICENSE-IntegratedDemonlist. Le build existant télécharge une capture de Nullscapes via scripts/prepare_logo.py. Le jeu, le niveau et l'image appartiennent à leurs titulaires respectifs.
