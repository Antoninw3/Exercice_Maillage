# Maillage

Outil en C++ / OpenMesh pour analyser, épaissir et découper des maillages OBJ.

## Compilation

```bash
sudo apt install build-essential cmake libopenmesh-dev
cmake -S . -B build
cmake --build build
```

## Commandes

```bash
# Analyse seule : points, faces, arêtes, bords ouverts
./build/objread <fichier.obj>

# Extruder tout le dessus : surface ouverte -> coquille fermée de <hauteur> d'épaisseur
# (la nappe est dupliquée vers le bas, retournée, et les bords sont reliés ;
# le dessous est la nappe décalée de <hauteur> à la verticale, voir « La nappe à la bille »),
# solide fermé -> toutes les faces du dessus montent, reliées aux côtés
./build/objread <fichier.obj> extruder <sortie.obj|sortie.stl> <hauteur> all

# Pattes : coque de <hauteur> qui suit le relief, creux dessous, et sous chaque
# bord une patte de <hauteur> d'épaisseur, côté matière, qui descend jusqu'au
# plan (marge sous le point le plus bas de la coque). Les rainures restent ouvertes.
./build/objread <fichier.obj> extruder <sortie.stl> <hauteur> all cotes
./build/objread <fichier.obj> extruder <sortie.stl> <hauteur> all cotes <z>

# Plein : solide à fond plat, <hauteur> sous le point le plus bas du dessus
# (épaisseur minimale). Les rainures restent ouvertes de haut en bas.
./build/objread <fichier.obj> extruder <sortie.stl> <hauteur> all plein
./build/objread <fichier.obj> extruder <sortie.stl> <hauteur> all plein <z>

# Option marge=<mm>, n'importe où sur la ligne : descente des pattes sous le point
# le plus bas de la coque (2 par défaut, même unité que le fichier)
./build/objread <fichier.obj> extruder <sortie.stl> 0.2 all cotes marge=0.2

# Option poussieres=<N>, n'importe où sur la ligne : avant d'épaissir, supprime les
# morceaux de moins de N faces (pixels isolés, débris)
./build/objread <fichier.obj> extruder <sortie.stl> 2 all cotes poussieres=2000

# Option trous=<N>, n'importe où sur la ligne : avant d'épaissir, bouche les trous
# de la nappe d'au plus N arêtes de tour (micro-trous de quelques pixels, qui
# deviendraient des tubes traversants trop fins pour être imprimés)
./build/objread <fichier.obj> extruder <sortie.stl> 3 all plein poussieres=2000 trous=100

# Option bille=<largeur>, avec all seul : le dessous suit une bille de <hauteur> de
# haut et de <largeur> de rayon à plat (voir « La nappe à la bille ») ; sans elle,
# le dessous est la nappe décalée de <hauteur> à la verticale
./build/objread <fichier.obj> extruder <sortie.stl> 3 all bille=0.8

# Extruder une face : elle monte et reste reliée à ses voisines
./build/objread <fichier.obj> extruder <sortie.obj|sortie.stl> <hauteur> <numero_face>

# Extruder : sans numéro, le programme liste les faces du dessus et demande un choix
./build/objread <fichier.obj> extruder <sortie.obj|sortie.stl> <hauteur>

# Découper en 4 quarts puis reboucher
./build/objread <fichier.obj> decouper <prefixe>

# Alléger sans perte : fusionne les triangles des paliers plats, du fond et des
# murs droits, sans déplacer la surface ; une seconde sortie reçoit le même
# résultat (OBJ et STL en un seul passage)
./build/objread <fichier.obj|fichier.stl> alleger <sortie.obj|sortie.stl> [autre_sortie.obj|autre_sortie.stl]

# Soleil flottant 10 x 20 cm, de 3d-mesh.obj jusqu'aux fichiers à envoyer
outils/flottant.sh samples/3d-mesh.obj samples/soleil_flottant_accentue_10x20cm
```

La découpe écrit pour chaque quart :

- `<prefixe>_quartN_decoupe.obj` : après la coupe, encore ouvert
- `<prefixe>_quartN_referme.obj` : après rebouchage
- `<prefixe>_quartN_referme.stl` : après rebouchage, en STL

## Le soleil flottant

`outils/flottant.sh <3d-mesh.obj> <prefixe> [socle bord emax|fixe [retrait angle]]` enchaîne :

1. `build/grille` : hauteur de chaque sommet de la grille de 6240 × 4160 et
   cases portées, lues par `mmap` (quelques secondes pour 2,2 Go) ;
2. `build/flottant` (`outils/flottant.cpp`) : relief flouté (sigma 2,5 pixels,
   contre le tramage de la photo), contour lissé (fermeture puis ouverture 3 × 3),
   agrandi à 10 × 20 cm ;
   dessus arrondi (rayon 0,8 mm) le long des contours, dos plat en retrait
   raccordé au bord par un biseau autoportant (45° ou plus), mur vertical d'au moins
   `bord` entre les deux ;
3. `objread alleger` sur l'entier, puis `objread decouper` et `alleger` sur
   chaque quart refermé.

Sorties : `<prefixe>.obj` et `.stl`, `<prefixe>_quart1..4.obj` et `.stl`, tous
fermés et allégés. Par défaut, version accentuée : socle 3 mm, bord 0,5 mm,
biseau adaptatif jusqu'à 3 mm (chaque point du contour prend le plus grand
retrait qui garde le biseau `bord` sous le dessus). La version d'origine :
`outils/flottant.sh samples/3d-mesh.obj <prefixe> 2 0.2 fixe` (biseau de 1 mm).

Biseau uniforme (même retrait et même angle partout, demandé par l'imprimeur) :
`outils/flottant.sh samples/3d-mesh.obj <prefixe> 3 0.5 fixe 0.5 70` (retrait
0,5 mm à 70°, hauteur 1,37 mm). L'angle se compte depuis l'horizontale, de 45°
(limite autoportante) à 90° exclu. Un ruban n'a de dos plat au milieu que s'il
est plus large que deux fois le retrait : le plus étroit de la spirale centrale
fait 1,64 mm, d'où un retrait de 0,5 mm au plus pour y garder un plan.

Environ 12 minutes et 5 Go de RAM au plus : les étapes tournent l'une après
l'autre, le STL est écrit par blocs et `alleger` / `decouper` libèrent la table
des arêtes dont ils ne se servent pas.

## Les côtés (pattes)

Avec `all cotes`, la coque est la nappe plus sa copie `<hauteur>` plus bas, et
les pattes sont l'extrusion vers le sol de la bande du dessous située à moins de
`<hauteur>` du bord (distance mesurée le long des arêtes). Dans cette bande, les
faces de la copie sont remplacées par leur projection sur le plan du sol ; un
mur vertical descend du bord du dessus jusqu'au sol, et un autre relie la copie
au sol le long de la limite intérieure de la bande. Tout reste sur la grille de
la nappe : pas de faces confondues, et une bande de plaque plus étroite que deux
fois l'épaisseur est pleine d'elle-même.

```
   dessus  o──────────────
           |
   copie   |      o───────
           |      |
           |      |          patte : <hauteur> de large
   sol     o──────o
```

Les bouchons de la découpe sont triangulés par écoupage d'oreilles
(`src/triangulation.cpp`, sans bibliothèque) : une oreille est un sommet convexe
dont le triangle ne contient aucun autre sommet, et à chaque étape on coupe
l'oreille la mieux formée (plus grand angle minimal). Une passe de bascule
d'arêtes termine (critère du cercle circonscrit, comme une triangulation de
Delaunay). Sur une section de coque fine qui suit les marches du relief, chaque
sommet du dessus est ainsi relié au sommet du dessous en face, sans longue
aiguille en diagonale. Un seul polygone
par section n'est pas possible : la section d'une coque à pattes est en U, et un
polygone serait triangulé en éventail à l'écriture STL, avec des triangles hors
de la section.

Les plans de coupe sont placés à mi-chemin entre les deux colonnes de la grille
qui encadrent le centre : un plan qui passe par des sommets contient des murs
entiers, impossibles à reboucher, et un plan trop près d'une colonne coupe les
pixels en tranches inégales, avec des marches minuscules dans la section et des
aiguilles dans les bouchons.

## La nappe à la bille

Par défaut (sans option), `all` fait la copie verticale : le dessous est la nappe
décalée de `<hauteur>`. Au droit d'une marche du relief, une telle coque ne garde
que `<hauteur>` moins la marche, et quelques centièmes de mm là où la marche
dépasse l'épaisseur (à 0,3 mm, 6,8 % de la surface du soleil tombait sous
0,1 mm ; sur l'éléphant à 3 mm, 0,19 mm au plus mince).

Avec `bille=<largeur>`, on fait rouler sous la nappe une bille de `<hauteur>` de
haut et de `<largeur>` de rayon à plat (ronde si `<largeur>` = `<hauteur>`) :
chaque sommet du dessous descend jusqu'au plus bas de ce que la bille impose
autour de lui, soit `min(z(q) - (hauteur / largeur) racine(largeur² - r²))` sur
les sommets `q` à moins de `<largeur>` en projection. Sur un palier, c'est
`<hauteur>` ; le long d'une marche, la coque descend avec la marche, en mur d'au
moins `<largeur>` d'épais. Les sommets sont rangés dans une grille de cases de
`<largeur>/4`, et une case est sautée quand même son sommet le plus bas ne
pourrait pas faire descendre davantage.

Le prix : une bulle sous chaque pixel en creux du dessus. Un creux de profondeur
`c` fait sur le dos un dôme de rayon `largeur × racine(2c/hauteur − (c/hauteur)²)`.
Sur l'éléphant tramé (creux de 0,11 mm), à 3 mm d'épaisseur : 1,6 mm de large avec
la bille ronde, 0,44 mm avec `bille=0.8`. La largeur règle donc le compromis entre
le mur le plus mince et la taille des bulles.

Mesuré sur une fenêtre du soleil, l'épaisseur la plus faible vaut 0,797 mm pour
0,8 mm demandés : entre deux sommets, le dessous est une corde plate qui rentre
d'au plus un quart de la largeur d'une face (6 à 8 µm ici) dans l'arc de la bille.
C'est sous la précision d'une machine de fusion laser (20 à 50 µm).

Conséquence : plus de matière que surface × épaisseur (+50 % à 0,8 mm sur le
soleil, à cause des murs le long des contours du dessin).

## Le plein et l'allègement

Avec `all plein`, le dessous est la projection de chaque face du dessus sur le
plan du sol, et un mur vertical descend de chaque bord : c'est la construction
des pattes avec une bande qui couvre toute la nappe.

Aux sommets pincés (deux morceaux qui se touchent par un seul coin), chaque
copie avance de 10 % vers le centre de ses propres faces, soit environ 1 µm sur
le soleil. Sans cela, les murs des deux morceaux partageraient une arête
verticale portée par quatre faces, que Magics ou Netfabb signalent comme
non-manifold.

`alleger` utilise le décimateur d'OpenMesh avec un module de priorité maison
(`src/allegement.cpp`) : une arête n'est fusionnée que si chaque face qui reste
autour du sommet supprimé garde son plan (à 10 nm près), son sens, et ne devient
pas plus fine qu'elle ne l'était (angle minimal de 2°). Les modules fournis
(quadriques, Hausdorff) laissent passer des écarts de 20 à 50 µm sur les
contremarches du relief ; celui-ci garde un écart mesuré inférieur à 0,01 µm.
