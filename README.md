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
# (la nappe est dupliquée vers le bas, retournée, et les bords sont reliés),
# solide fermé -> toutes les faces du dessus montent, reliées aux côtés
./build/objread <fichier.obj> extruder <sortie.obj|sortie.stl> <hauteur> all

# Même chose avec un plancher : le dessous ne descend jamais sous z = <plancher>
# et devient plat à ces endroits (appuis plans pour poser la pièce)
./build/objread <fichier.obj> extruder <sortie.obj|sortie.stl> <hauteur> all <plancher>

# Option poussieres=<N>, n'importe où sur la ligne : avant d'épaissir, supprime les
# morceaux de moins de N faces (pixels isolés, débris)
./build/objread <fichier.obj> extruder <sortie.stl> 2 all -1 poussieres=2000

# Extruder une face : elle monte et reste reliée à ses voisines
./build/objread <fichier.obj> extruder <sortie.obj|sortie.stl> <hauteur> <numero_face>

# Extruder : sans numéro, le programme liste les faces du dessus et demande un choix
./build/objread <fichier.obj> extruder <sortie.obj|sortie.stl> <hauteur>

# Découper en 4 quarts puis reboucher
./build/objread <fichier.obj> decouper <prefixe>
```

La découpe écrit pour chaque quart :

- `<prefixe>_quartN_decoupe.obj` : après la coupe, encore ouvert
- `<prefixe>_quartN_referme.obj` : après rebouchage
- `<prefixe>_quartN_referme.stl` : après rebouchage, en STL
