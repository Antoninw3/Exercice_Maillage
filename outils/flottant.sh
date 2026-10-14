#!/bin/bash
set -euo pipefail

if [ $# -ne 2 ] && [ $# -ne 5 ] && [ $# -ne 7 ]; then
    cat <<'FIN'
Soleil flottant 10 x 20 cm, de 3d-mesh.obj jusqu'aux fichiers a envoyer :
  <prefixe>.obj et .stl               entier, allege, ferme
  <prefixe>_quart1..4.obj et .stl     quarts refermes, alleges
Les fichiers intermediaires (grille, STL brut, quarts avant allegement) sont
faits dans un dossier de travail efface a la fin.

Usage : outils/flottant.sh <3d-mesh.obj> <prefixe> [socle bord emax|fixe [retrait angle]]
  par defaut : version accentuee (socle 3, bord 0.5, biseau adaptatif jusqu'a 3 mm)
  version d'origine : outils/flottant.sh samples/3d-mesh.obj <prefixe> 2 0.2 fixe
  biseau uniforme 0,5 mm a 70 deg : outils/flottant.sh samples/3d-mesh.obj <prefixe> 3 0.5 fixe 0.5 70
Environ 12 minutes, 5 Go de RAM au plus (etapes l'une apres l'autre), 5 Go de
disque pour le dossier de travail.
FIN
    exit 1
fi
ENTREE=$1
PREFIXE=$2
shift 2

RACINE=$(cd "$(dirname "$0")/.." && pwd)
OBJREAD=$RACINE/build/objread
GRILLE=$RACINE/build/grille
FLOTTANT=$RACINE/build/flottant
for outil in "$OBJREAD" "$GRILLE" "$FLOTTANT"; do
    if [ ! -x "$outil" ]; then
        echo "Erreur : $outil absent, compiler d'abord : cmake -S . -B build && cmake --build build" >&2
        exit 1
    fi
done
if [ ! -f "$ENTREE" ]; then
    echo "Erreur : '$ENTREE' introuvable" >&2
    exit 1
fi

mkdir -p "$(dirname "$PREFIXE")"
TRAVAIL=$(mktemp -d "$(dirname "$PREFIXE")/travail_flottant_XXXXXX")
trap 'rm -rf "$TRAVAIL"' EXIT

etape() {
    echo
    echo "=== $1 ($((SECONDS / 60)) min $((SECONDS % 60)) s)"
}

etape "1/5 grille des hauteurs"
"$GRILLE" "$ENTREE" "$TRAVAIL"

etape "2/5 construction du solide flottant"
"$FLOTTANT" "$TRAVAIL/z.f32" "$TRAVAIL/tri.u8" "$TRAVAIL/brut.stl" "$@"
rm "$TRAVAIL/z.f32" "$TRAVAIL/tri.u8"

etape "3/5 entier allege"
"$OBJREAD" "$TRAVAIL/brut.stl" alleger "$PREFIXE.obj" "$PREFIXE.stl"

etape "4/5 decoupe en 4 et rebouchage"
"$OBJREAD" "$TRAVAIL/brut.stl" decouper "$TRAVAIL/q"
rm "$TRAVAIL/brut.stl"

for i in 1 2 3 4; do
    etape "5/5 quart $i allege"
    if [ ! -f "$TRAVAIL/q_quart${i}_referme.obj" ]; then
        echo "quart $i vide, ignore"
        continue
    fi
    "$OBJREAD" "$TRAVAIL/q_quart${i}_referme.obj" alleger "${PREFIXE}_quart${i}.obj" "${PREFIXE}_quart${i}.stl"
done

etape "termine"
ls -lh "$PREFIXE".obj "$PREFIXE".stl "${PREFIXE}"_quart?.obj "${PREFIXE}"_quart?.stl
