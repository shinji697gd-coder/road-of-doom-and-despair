# Road of Doom and Despair

Mod [Geode](https://geode-sdk.org/) pour **Geometry Dash 2.2081** (Windows), avec une roadmap personnalisée de **75 niveaux**.

## Bêta v1.0.0-beta.1

La bêta conserve l'interface native et les boutons **VIEW** fonctionnels de la v0.5.6.

- **75 niveaux** dans l'ordre défini par `src/level_data.hpp`, jusqu'à **Nullscapes** (ID `109780665`).
- **Recherche** par nom ou identifiant de niveau.
- **Pagination** (10 niveaux par page) et boutons première/dernière page.
- **MARK / DONE** : chaque niveau peut être marqué comme terminé.
- **Progression sauvegardée localement** via les données du mod Geode, indexées par ID de niveau.
- **Filtre** en bas de l'écran : `ALL` → `TO DO` → `DONE`. La recherche et le filtre fonctionnent ensemble.
- **PROGRESS** en haut : nombre de niveaux marqués terminés sur 75.
- Si les serveurs de Geometry Dash ne répondent pas, les noms et IDs restent visibles et le bouton d'actualisation permet de réessayer.

### Utilisation

1. Installer Geode **5.11.2** avec Geometry Dash **2.2081** sous Windows.
2. Installer le fichier `.geode` issu de la compilation GitHub Actions.
3. Ouvrir la recherche de niveaux de Geometry Dash, puis le bouton étoile ajouté aux filtres.
4. Utiliser **VIEW** pour accéder au niveau, **MARK** pour enregistrer sa progression et **FILTER** pour sélectionner les niveaux affichés.

Les niveaux marqués **DONE** ne sont **pas** détectés automatiquement à partir de la progression Geometry Dash : ce sont des coches manuelles, conservées localement. La progression n'est pas synchronisée entre appareils.

### Compilation et validation

Le workflow `.github/workflows/build.yml` compile le mod Windows avec Geode SDK 5.11.2. Il s'exécute sur `main`, sur `beta/**` et sur les pull requests vers `main`.

La commande suivante vérifie la cohérence des données avant compilation :

```bash
python scripts/check_roadmap.py
```

### Checklist de test bêta

- [ ] Ouvrir la roadmap depuis la recherche de niveaux sans plantage.
- [ ] Vérifier l'affichage des 75 niveaux et la navigation jusqu'à Nullscapes.
- [ ] Ouvrir plusieurs niveaux avec leurs boutons VIEW.
- [ ] Marquer un niveau DONE, fermer et rouvrir le jeu, vérifier la sauvegarde.
- [ ] Vérifier les filtres ALL, TO DO, DONE et leur combinaison avec la recherche.
- [ ] Tester la recherche par ID, les résultats vides et le retour arrière.
- [ ] Vérifier l'actualisation et le comportement sans réseau.
- [ ] Tester la navigation clavier gauche/droite et Entrée.

**Statut : bêta à tester en jeu.** La compilation automatique ne remplace pas les tests dans Geometry Dash.

## Icône

Le script `scripts/prepare_logo.py` télécharge une capture réelle du gameplay de **Nullscapes**, niveau créé par ItzKiba/Kiba, ID `109780665`, puis la recadre en `logo.png`.

Source de la capture : https://geometry-dash-fan.fandom.com/wiki/Nullscapes

Ce n'est **pas** une image générée par IA. Les droits sur le jeu, le niveau et les captures appartiennent à leurs titulaires respectifs. Obtenir les autorisations appropriées avant toute redistribution publique. La compilation dépend de la disponibilité de cette source externe.

## Crédits

L'approche de l'interface et de la recherche s'inspire du projet **Integrated Demonlist** de hiimjasmine00 (MIT). Geometry Dash et ses niveaux appartiennent à leurs créateurs et ayants droit respectifs.
