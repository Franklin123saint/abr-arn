/* abr_arn.c : ABR et arbre rouge-noir, version de référence en C.
 * Mêmes algorithmes que l'application web (mêmes noms de cas).
 * Compilation : gcc -Wall -Wextra -o abr_arn abr_arn.c && ./abr_arn
 */
#include <stdio.h>
#include <stdlib.h>

enum { ROUGE, NOIR };

typedef struct Noeud {
    int k;                    /* clé */
    int c;                    /* couleur (ignorée en mode ABR) */
    struct Noeud *g, *d, *p;  /* fils gauche, fils droit, parent */
} Noeud;

static Noeud *racine = NULL;
static int ARN = 1;           /* 1 : arbre rouge-noir, 0 : ABR simple */

static Noeud *creer(int k, int c) {
    Noeud *n = calloc(1, sizeof *n);
    n->k = k; n->c = c;
    return n;
}

static Noeud *minimum(Noeud *x) { while (x->g) x = x->g; return x; }

static Noeud *chercher(Noeud *x, int k) {
    while (x && k != x->k) x = (k < x->k) ? x->g : x->d;
    return x;
}

/* 'G' : rotation gauche autour de x, 'D' : rotation droite. */
static void rotation(Noeud *x, char sens) {
    Noeud *y = (sens == 'G') ? x->d : x->g;      /* y remonte à la place de x */
    if (sens == 'G') { x->d = y->g; if (y->g) y->g->p = x; y->g = x; }
    else             { x->g = y->d; if (y->d) y->d->p = x; y->d = x; }
    y->p = x->p;
    if (!x->p) racine = y;
    else if (x == x->p->g) x->p->g = y;
    else x->p->d = y;
    x->p = y;
}

/* ---------- Insertion ---------- */
static void fixInsert(Noeud *z) {
    while (z->p && z->p->c == ROUGE) {           /* père rouge : règle violée */
        Noeud *p = z->p, *g = p->p;
        int gauche = (p == g->g);
        Noeud *u = gauche ? g->d : g->g;         /* oncle */
        if (u && u->c == ROUGE) {                /* cas 1 : oncle rouge */
            p->c = u->c = NOIR; g->c = ROUGE;
            z = g; continue;                     /* le problème remonte */
        }
        if (z == (gauche ? p->d : p->g)) {       /* cas 2 : coude */
            rotation(p, gauche ? 'G' : 'D');
            z = p; p = z->p;
        }
        p->c = NOIR; g->c = ROUGE;               /* cas 3 : ligne */
        rotation(g, gauche ? 'D' : 'G');
        break;
    }
    racine->c = NOIR;
}

static void inserer(int k) {
    Noeud *x = racine, *p = NULL;
    while (x) {
        if (k == x->k) return;                   /* doublon refusé */
        p = x;
        x = (k < x->k) ? x->g : x->d;
    }
    Noeud *z = creer(k, ROUGE);                  /* nouveau nœud : rouge */
    z->p = p;
    if (!p) racine = z;
    else if (k < p->k) p->g = z;
    else p->d = z;
    if (ARN) fixInsert(z);
}

/* ---------- Suppression ---------- */
static void transplanter(Noeud *u, Noeud *v) {   /* remplace u par v */
    if (!u->p) racine = v;
    else if (u == u->p->g) u->p->g = v;
    else u->p->d = v;
    if (v) v->p = u->p;
}

/* x peut être NULL (feuille vide) : on garde donc son parent xp à part. */
static void fixSupp(Noeud *x, Noeud *xp) {
    while (x != racine && (!x || x->c == NOIR)) {
        int gauche = (x == xp->g);
        Noeud *w = gauche ? xp->d : xp->g;       /* frère */
        if (w->c == ROUGE) {                     /* cas 1 : frère rouge */
            w->c = NOIR; xp->c = ROUGE;
            rotation(xp, gauche ? 'G' : 'D');
            w = gauche ? xp->d : xp->g;
        }
        if ((!w->g || w->g->c == NOIR) && (!w->d || w->d->c == NOIR)) {
            w->c = ROUGE; x = xp; xp = x->p;     /* cas 2 : on remonte */
        } else {
            Noeud *loin   = gauche ? w->d : w->g;
            Noeud *proche = gauche ? w->g : w->d;
            if (!loin || loin->c == NOIR) {      /* cas 3 : neveu proche rouge */
                if (proche) proche->c = NOIR;
                w->c = ROUGE;
                rotation(w, gauche ? 'D' : 'G');
                w = gauche ? xp->d : xp->g;
                loin = gauche ? w->d : w->g;
            }
            w->c = xp->c; xp->c = NOIR;          /* cas 4 : neveu éloigné rouge */
            if (loin) loin->c = NOIR;
            rotation(xp, gauche ? 'G' : 'D');
            x = racine; break;
        }
    }
    if (x) x->c = NOIR;
}

static void supprimer(int k) {
    Noeud *z = chercher(racine, k);
    if (!z) return;
    Noeud *y = z, *x, *xp;
    int couleur_y = y->c;
    if (!z->g)      { x = z->d; xp = z->p; transplanter(z, z->d); }
    else if (!z->d) { x = z->g; xp = z->p; transplanter(z, z->g); }
    else {                                       /* deux fils : successeur */
        y = minimum(z->d); couleur_y = y->c; x = y->d;
        if (y->p == z) xp = y;
        else { xp = y->p; transplanter(y, y->d); y->d = z->d; y->d->p = y; }
        transplanter(z, y);
        y->g = z->g; y->g->p = y; y->c = z->c;
    }
    if (ARN && couleur_y == NOIR) fixSupp(x, xp);
    free(z);
}

/* ---------- Outils de test ---------- */
static int hauteur(Noeud *x) {
    if (!x) return 0;
    int a = hauteur(x->g), b = hauteur(x->d);
    return 1 + (a > b ? a : b);
}

/* Nombre de nœuds noirs par chemin, ou -1 si une règle est violée. */
static int noirs(Noeud *x) {
    if (!x) return 1;
    int a = noirs(x->g), b = noirs(x->d);
    if (a < 0 || b < 0 || a != b) return -1;
    if (x->c == ROUGE && ((x->g && x->g->c == ROUGE) || (x->d && x->d->c == ROUGE)))
        return -1;
    return a + (x->c == NOIR);
}

static void afficher(Noeud *x) {                 /* ex. 20N(10N,40R(...)) */
    if (!x) return;
    printf("%d", x->k);
    if (ARN) putchar(x->c == ROUGE ? 'R' : 'N');
    if (x->g || x->d) {
        putchar('('); afficher(x->g); putchar(','); afficher(x->d); putchar(')');
    }
}

static void liberer(Noeud *x) {
    if (!x) return;
    liberer(x->g); liberer(x->d); free(x);
}

static void etat(const char *titre) {
    printf("%-22s ", titre); afficher(racine);
    printf("\n%-22s hauteur = %d", "", hauteur(racine));
    if (ARN) printf(", règles %s", (racine && racine->c == NOIR && noirs(racine) > 0) ? "OK" : "VIOLÉES");
    printf("\n\n");
}

int main(void) {
    for (ARN = 1; ARN >= 0; ARN--) {
        printf("===== %s =====\n", ARN ? "Arbre rouge-noir" : "ABR simple");
        racine = NULL;
        for (int k = 10; k <= 70; k += 10) inserer(k);
        etat("après 10, 20, ..., 70 :");
        supprimer(10);
        etat("après suppression de 10 :");
        liberer(racine);
    }
    return 0;
}
