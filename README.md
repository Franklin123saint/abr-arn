# abr·arn : regarder un arbre se rééquilibrer

Une application web qui montre, étape par étape, ce qui se passe dans un **arbre binaire de recherche (ABR)** et dans un **arbre rouge-noir (ARN)** quand on insère, supprime ou cherche une valeur. Une version de référence en **C** accompagne l’application, pour relier l’animation au code que l’on écrit en cours.

---

## 1. L’idée en trois phrases

1. Un ABR est un jeu de « plus petit / plus grand » : à chaque nœud, on choisit gauche ou droite. C’est rapide tant que l’arbre reste équilibré.
2. Avec des valeurs déjà triées, l’ABR dégénère en liste et chaque opération coûte **O(n)**.
3. L’ARN ajoute une couleur à chaque nœud et quelques règles. Quand une règle est violée, il recolorie et fait pivoter des nœuds pour se rééquilibrer. Résultat : **O(log n)**, toujours.

## 2. Lancer le projet

| Ce que vous voulez | Comment |
|---|---|
| Voir l’application | Ouvrir `index.html` dans un navigateur. Aucun serveur, aucune installation. |
| Voir le code C tourner | `gcc -Wall -Wextra -o abr_arn abr_arn.c && ./abr_arn` |

## 3. Carte des fichiers

```
index.html   l’application (HTML + CSS + JavaScript, un seul fichier)
abr_arn.c              les mêmes algorithmes en C, avec un main de test
README.md              ce document
```

Dans `index.html`, le fichier se lit de haut en bas :

| Zone | Contenu |
|---|---|
| `<style>` | L’apparence : thème clair et sombre, barre de commandes, points de navigation. |
| `<body>` | La page : sélecteur ABR/ARN, zone de l’arbre, phrase d’explication, barre de commandes. |
| `<script>` | **Toute la logique.** C’est ici que se trouvent les opérations. |

---

## 4. Un nœud, quatre champs

```js
{ k: 40, c: 'R', l: …, r: …, p: … }      // JavaScript
```
```c
typedef struct Noeud {                    /* C */
    int k, c;                             /* clé, couleur */
    struct Noeud *g, *d, *p;              /* gauche, droite, parent */
} Noeud;
```

- `k` est la clé. `l`/`r` (JavaScript) ou `g`/`d` (C) sont les deux fils. `p` est le **parent** : les rotations et la suppression en ont besoin pour « remonter ».
- `c` est la couleur. **Attention aux lettres** : en JavaScript, `'R'` = rouge, `'B'` = noir (*black*), et `'N'` = « neutre », utilisé uniquement en mode ABR. En C, on utilise les constantes `ROUGE` et `NOIR`.

Un pointeur `NULL` (`null` en JavaScript) représente une **feuille vide**. Par convention, une feuille vide compte comme **noire**.

---

## 5. Le moteur du « film » : l’arbre ne se dessine jamais lui-même

C’est la seule idée d’architecture à comprendre avant de lire le code.

Les fonctions `ins`, `del`, `find`… **ne dessinent rien**. Elles font le travail sur l’arbre et, aux moments importants, elles appellent :

```js
msg("Comparer 25 avec 30 : plus petit, on va à gauche.", [30]);
```

`msg` fait deux choses : il **photographie l’arbre** (`ser` en fait une copie sans les parents) et il range cette photo avec la phrase et la liste des nœuds à surligner dans le tableau `frames`.

```
opération ──msg──▶ frames = [ photo0, photo1, photo2, … ]
                                    │
                    show(i) dessine la photo i en SVG
                    play() avance de photo en photo toutes les 1,1 s
```

Pourquoi ce choix ? Parce qu’il sépare le **calcul** (les algorithmes) de l’**affichage** (le SVG). Les algorithmes restent lisibles : on peut les lire sans connaître le dessin. Et revenir en arrière est gratuit : une étape précédente est juste une photo plus ancienne.

| Fonction | Rôle |
|---|---|
| `msg(texte, surlignés)` | Enregistre une étape. |
| `ser(n)` | Copie l’arbre pour la photo. |
| `lay(t)` | Calcule la position de chaque nœud : abscisse = rang dans le parcours infixe, ordonnée = profondeur. |
| `show(i)` | Dessine la photo `i`. |
| `run(fn)` | Vide `frames`, lance l’opération, puis lit le film. |
| `quiet(fn)` | Construit un arbre **sans** enregistrer d’étapes (boutons « aléatoire » et « 10 à 70 »). |

---

## 6. L’ABR : trois gestes

### Descendre (chercher et insérer)

À chaque nœud, on compare et on part à gauche ou à droite. C’est `find` en JavaScript, `chercher` en C :

```c
Noeud *chercher(Noeud *x, int k) {
    while (x && k != x->k) x = (k < x->k) ? x->g : x->d;
    return x;
}
```

### Accrocher (insérer)

On descend en gardant le parent `p`. Quand on tombe sur une place vide, on y accroche le nouveau nœud (`ins` / `inserer`). Un doublon est **accepté** : à l’égalité, on part à droite (voir plus bas).

### Recoller (supprimer)

Supprimer un nœud `z` laisse un trou. Trois situations (`del` / `supprimer`) :

| Situation | Geste |
|---|---|
| Pas de fils gauche | Le fils droit prend la place de `z`. |
| Pas de fils droit | Le fils gauche prend la place de `z`. |
| Deux fils | On prend le **successeur** : le minimum du sous-arbre droit. Il prend la place de `z`. |

Le « geste de recollage » s’appelle `transplanter(u, v)` (`tr` en JavaScript) : il remplace `u` par `v` chez le parent de `u`.

### Les doublons

Les clés peuvent apparaître plusieurs fois. Chaque doublon est un **nœud à part entière**.

- **À l’insertion**, quand `k == x->k`, on descend à droite : `(k < x->k) ? x->g : x->d`.
- **L’invariant** devient : sous-arbre gauche ≤ nœud ≤ sous-arbre droit. Dans l’ARN, une rotation peut envoyer un doublon à gauche, et l’ordre reste valide.
- **La recherche** s’arrête sur le premier nœud égal rencontré en descendant, et **la suppression** retire un seul nœud portant cette valeur.
- **Le parcours infixe** reste trié, au sens large : 5, 5, 5, 8…

### Les parcours

Infixe (gauche, nœud, droite), préfixe (nœud, gauche, droite), postfixe (gauche, droite, nœud). Le parcours **infixe d’un ABR donne les valeurs triées** : c’est un bon test pour vérifier votre arbre.

---

## 7. L’ARN : ce que les couleurs ajoutent

### Les règles

1. Chaque nœud est rouge ou noir.
2. La racine est noire.
3. Un nœud rouge n’a pas de fils rouge.
4. Tous les chemins d’un nœud vers ses feuilles vides contiennent le même nombre de nœuds noirs.

Conséquence : le chemin le plus long est au plus deux fois plus long que le plus court, donc la hauteur est au plus **2·log₂(n+1)**.

### La rotation : l’outil de base

Une rotation change la forme de l’arbre sans changer l’ordre des clés. Rotation **gauche** autour de `x` :

```
     x                  y
    / \                / \
   A   y      ───▶    x   C
      / \            / \
     B   C          A   B
```

`rot(x, d)` en JavaScript et `rotation(x, sens)` en C font ce travail. La rotation droite est son miroir.

| Sens | JavaScript | C |
|---|---|---|
| Gauche | `'l'` | `'G'` |
| Droite | `'r'` | `'D'` |

### Insertion : « lire l’oncle »

Un nouveau nœud est **rouge**. Seule la règle 3 peut casser, si son parent est rouge aussi. On regarde alors l’**oncle** (le frère du parent) :

| Ce qu’on voit | Cas | Action |
|---|---|---|
| Oncle rouge | 1 | Parent et oncle deviennent noirs, le grand-parent devient rouge. Le problème **remonte** au grand-parent. |
| Oncle noir, le nœud est un « coude » | 2 | Rotation autour du parent pour aligner les trois nœuds. On passe au cas 3. |
| Oncle noir, le nœud est en « ligne » | 3 | Parent noir, grand-parent rouge, rotation autour du grand-parent. **Terminé.** |

À la fin, on force la racine en noir. Une insertion fait au plus 2 rotations.

### Suppression : le « noir en trop »

Supprimer un nœud **rouge** ne casse rien. Supprimer un nœud **noir** fait manquer un noir sur un chemin (règle 4). On imagine que le nœud `x` qui a pris sa place porte un **noir en trop**, et on essaie de s’en débarrasser en regardant son **frère** `w` :

| Ce qu’on voit | Cas | Action |
|---|---|---|
| Frère rouge | 1 | On le noircit, on rougit le parent, rotation. On retombe sur un des cas suivants. |
| Frère noir, deux neveux noirs | 2 | On rougit le frère et le noir en trop **monte** d’un cran. |
| Frère noir, neveu proche rouge, neveu éloigné noir | 3 | Rotation autour du frère pour se ramener au cas 4. |
| Frère noir, neveu éloigné rouge | 4 | Le frère prend la couleur du parent, rotation autour du parent. **Terminé.** |

Une suppression fait au plus 3 rotations.

---

## 8. Du message affiché à la fonction

Quand l’application affiche une phrase, voici où se trouve le code correspondant.

| Phrase affichée | JavaScript | C |
|---|---|---|
| « Comparer 25 avec 30… » | boucle de `find` et `ins` | boucle de `chercher` et `inserer` |
| « Parent et oncle sont rouges… » | `fixIns`, cas 1 | `fixInsert`, cas 1 |
| « Cas coude… » / « Cas ligne… » | `fixIns`, cas 2 et 3 | `fixInsert`, cas 2 et 3 |
| « …a deux enfants : successeur… » | `del` | `supprimer` |
| « Frère rouge… » (cas 1 à 4) | `fixDel` | `fixSupp` |
| « La racine doit être noire… » | fin de `fixIns` | dernière ligne de `fixInsert` |

---

## 9. Trois expériences à faire

**Expérience 1 : voir pourquoi l’ARN existe.**
Dans l’onglet ABR, cliquez sur « Insérer 10 à 70 dans l’ordre » (menu *Plus*). Résultat : une liste de **7 niveaux**. Faites la même chose dans l’onglet Rouge-noir : l’arbre a **4 niveaux**, avec 20 à la racine. En notation du programme C : `20N(10N,40R(30N,60N(50R,70R)))`.

**Expérience 2 : suivre une suppression complète.**
Dans l’arbre de l’expérience 1 (rouge-noir), supprimez **10**. C’est un nœud noir, donc le rééquilibrage se déclenche. Vous verrez dans l’ordre : un frère rouge (cas 1), puis un frère noir avec deux neveux noirs (cas 2), puis un nœud rouge repassé en noir. Résultat : `40N(20N(,30R),60N(50R,70R))`.

**Expérience 3 : voir une rotation.**
Dans l’onglet Rouge-noir, partez d’un arbre vide et insérez 10, 20, 30. Au troisième ajout, 30 et son parent 20 sont rouges, et l’oncle est vide (donc noir) : c’est le cas « ligne », avec une rotation gauche. 20 devient la racine.

**Expérience 4 : des doublons.**
Dans l’onglet ABR, insérez 5 cinq fois de suite : une liste de 5 niveaux, car chaque doublon part à droite. Dans l’onglet Rouge-noir, insérez 5 sept fois : l’arbre reste équilibré (4 niveaux). En C : `5N(5N,5R(5N,5N(5R,5R)))`.

Le programme C reproduit les expériences 1, 2 et 4 et affiche l’arbre après chaque étape, avec un contrôle automatique des règles (`noirs()`).

---

## 10. Pièges rencontrés

- **`x` peut être vide dans `fixDel` / `fixSupp`.** Quand on supprime une feuille noire, le « nœud qui prend sa place » est `NULL`, et un pointeur vide n’a pas de parent. C’est pourquoi la fonction reçoit aussi `xp`, le parent de `x`, séparément.
- **Gauche ou droite ?** Les cas de rééquilibrage existent en version miroir. Le code évite de les écrire deux fois avec une variable `gauche` et des expressions comme `gauche ? xp->d : xp->g`. Si vous tracez le code à la main, faites-le d’abord pour le cas « gauche ».
- **Les lettres de couleur** diffèrent entre JavaScript (`'R'`, `'B'`, `'N'`) et C (`ROUGE`, `NOIR`). Voir la section 4.
- **Doublons et surlignage.** Deux nœuds de même clé ne doivent pas s’allumer ensemble. Dans l’application, chaque nœud a un identifiant unique (`id`), et `msg` surligne par identifiant, pas par valeur.
- **Le mode ABR réutilise le même code.** `ins` et `del` sont communs : seuls les appels à `fixIns` et `fixDel` sont réservés à l’ARN. L’ARN est un ABR qui se surveille.

## 11. Limites et pistes

- Les clés sont des entiers. Les doublons sont acceptés (section 6).
- L’application n’affiche pas le code à l’écran : le lien avec le code passe par ce document et par le tableau de la section 8.
- Pistes : ajouter une file de commandes (« insérer 5, 3, 8 d’un coup »), un mode côte à côte ABR et ARN, ou un export du film en image.
