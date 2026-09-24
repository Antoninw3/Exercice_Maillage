#include "epaississement.hpp"

#include <algorithm>
#include <cmath>
#include <set>
#include <stdexcept>

namespace {

void ajouterFace(Maillage& maillage, const std::vector<Sommet>& sommets, const std::string& quoi) {
    if (!maillage.add_face(sommets).is_valid()) {
        throw std::runtime_error("impossible d'ajouter " + quoi);
    }
}

Point normaleGlobale(const Maillage& source, const TableAretes& table) {
    Point somme(0, 0, 0);
    for (const std::vector<int>& coins : table.coins) {
        if (coins.size() < 3) {
            continue;
        }
        Point a = source.point(source.vertex_handle(coins[0]));
        Point b = source.point(source.vertex_handle(coins[1]));
        Point c = source.point(source.vertex_handle(coins[2]));
        somme += OpenMesh::cross(b - a, c - a);
    }
    return somme;
}

Point deplacer(const Point& p, float hauteur, const Point& direction, float plancher) {
    Point resultat = p + direction * hauteur;
    if (std::isnan(plancher)) {
        return resultat;
    }
    if (direction[2] < 0) {
        resultat[2] = std::max(resultat[2], plancher);
    } else if (direction[2] > 0) {
        resultat[2] = std::min(resultat[2], plancher);
    }
    return resultat;
}

Sommet sommetOuCreer(Maillage& resultat, const Maillage& source, std::vector<Sommet>& copies, int s) {
    if (!copies[s].is_valid()) {
        copies[s] = resultat.add_vertex(source.point(source.vertex_handle(s)));
    }
    return copies[s];
}

}  // namespace

Maillage epaissir(const Maillage& nappe, const TableAretes& table, float hauteur, const Point& direction,
                  float plancher) {
    Maillage solide;
    bool copieAuDessus = OpenMesh::dot(normaleGlobale(nappe, table), direction) >= 0;

    std::vector<Sommet> origine(table.nbSommets);
    std::vector<Sommet> copie(table.nbSommets);
    for (const std::vector<int>& coins : table.coins) {
        for (int s : coins) {
            if (origine[s].is_valid()) {
                continue;
            }
            Point p = nappe.point(nappe.vertex_handle(s));
            origine[s] = solide.add_vertex(p);
            copie[s] = solide.add_vertex(deplacer(p, hauteur, direction, plancher));
        }
    }
    const std::vector<Sommet>& dessus = copieAuDessus ? copie : origine;
    const std::vector<Sommet>& dessous = copieAuDessus ? origine : copie;

    for (const std::vector<int>& coins : table.coins) {
        std::vector<Sommet> faceHaut;
        std::vector<Sommet> faceBas;
        for (int s : coins) {
            faceHaut.push_back(dessus[s]);
        }
        for (int i = coins.size() - 1; i >= 0; i--) {
            faceBas.push_back(dessous[coins[i]]);
        }
        ajouterFace(solide, faceHaut, "une face du dessus");
        ajouterFace(solide, faceBas, "une face du dessous");
    }

    for (const Arete& arete : table.aretes) {
        if (!arete.estBord()) {
            continue;
        }
        int a = arete.a;
        int b = arete.b;
        if (!faceParcourt(table.coins[arete.faces[0]], a, b)) {
            std::swap(a, b);
        }
        std::vector<Sommet> paroi = {dessus[b], dessus[a], dessous[a], dessous[b]};
        ajouterFace(solide, paroi, "un quad de bord");
    }
    return solide;
}

Maillage extruderFaces(const Maillage& maillage, const TableAretes& table, const std::vector<int>& numerosFaces,
                       float hauteur, const Point& direction) {
    std::set<int> selection(numerosFaces.begin(), numerosFaces.end());
    Maillage resultat;
    std::vector<Sommet> originaux(table.nbSommets);
    std::vector<Sommet> deplaces(table.nbSommets);

    for (int f = 0; f < (int)table.coins.size(); f++) {
        bool choisie = selection.count(f) > 0;
        std::vector<Sommet> sommets;
        for (int s : table.coins[f]) {
            if (choisie) {
                if (!deplaces[s].is_valid()) {
                    deplaces[s] = resultat.add_vertex(maillage.point(maillage.vertex_handle(s)) + direction * hauteur);
                }
                sommets.push_back(deplaces[s]);
            } else {
                sommets.push_back(sommetOuCreer(resultat, maillage, originaux, s));
            }
        }
        ajouterFace(resultat, sommets, "une face");
    }

    for (const Arete& arete : table.aretes) {
        int nbChoisies = 0;
        int faceChoisie = -1;
        for (int k = 0; k < arete.nbFaces && k < 2; k++) {
            if (selection.count(arete.faces[k]) > 0) {
                nbChoisies++;
                faceChoisie = arete.faces[k];
            }
        }
        if (nbChoisies != 1) {
            continue;
        }
        int a = arete.a;
        int b = arete.b;
        if (!faceParcourt(table.coins[faceChoisie], a, b)) {
            std::swap(a, b);
        }
        std::vector<Sommet> cote = {deplaces[b], deplaces[a],
                                    sommetOuCreer(resultat, maillage, originaux, a),
                                    sommetOuCreer(resultat, maillage, originaux, b)};
        ajouterFace(resultat, cote, "un quad de cote");
    }
    return resultat;
}

std::vector<int> facesDuDessus(const Maillage& maillage, const Point& haut) {
    std::vector<int> resultat;
    for (Face face : maillage.faces()) {
        std::vector<int> coins = sommetsDeLaFace(maillage, face.idx());
        if (coins.size() < 3) {
            continue;
        }
        Point a = maillage.point(maillage.vertex_handle(coins[0]));
        Point b = maillage.point(maillage.vertex_handle(coins[1]));
        Point c = maillage.point(maillage.vertex_handle(coins[2]));
        if (OpenMesh::dot(OpenMesh::cross(b - a, c - a), haut) > 0) {
            resultat.push_back(face.idx());
        }
    }
    return resultat;
}

std::string formaterPlages(const std::vector<int>& numeros) {
    std::string texte;
    int i = 0;
    while (i < (int)numeros.size()) {
        int j = i;
        while (j + 1 < (int)numeros.size() && numeros[j + 1] == numeros[j] + 1) {
            j++;
        }
        if (!texte.empty()) {
            texte += ", ";
        }
        texte += std::to_string(numeros[i]);
        if (j > i) {
            texte += "-" + std::to_string(numeros[j]);
        }
        i = j + 1;
    }
    return texte;
}
