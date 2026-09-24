#include "reparateur.hpp"

#include <stdexcept>

#include "bords.hpp"

namespace {

int racine(std::vector<int>& parent, int x) {
    while (parent[x] != x) {
        parent[x] = parent[parent[x]];
        x = parent[x];
    }
    return x;
}

void reunir(std::vector<int>& parent, int a, int b) {
    parent[racine(parent, a)] = racine(parent, b);
}

int coinDuSommet(const std::vector<int>& coins, int sommet) {
    for (int k = 0; k < (int)coins.size(); k++) {
        if (coins[k] == sommet) {
            return k;
        }
    }
    return -1;
}

struct Eventails {
    std::vector<std::vector<int> > numero;
    std::vector<int> parSommet;
    int pinces = 0;
};

Eventails calculerEventails(const TableAretes& table) {
    int nbFaces = table.coins.size();
    std::vector<int> debut(nbFaces);
    std::vector<std::vector<int> > facesDuSommet(table.nbSommets);
    int nbCoins = 0;
    for (int f = 0; f < nbFaces; f++) {
        debut[f] = nbCoins;
        nbCoins += table.coins[f].size();
        for (int s : table.coins[f]) {
            facesDuSommet[s].push_back(f);
        }
    }

    std::vector<int> parent(nbCoins);
    for (int c = 0; c < nbCoins; c++) {
        parent[c] = c;
    }
    for (const Arete& arete : table.aretes) {
        if (arete.nbFaces != 2) {
            continue;
        }
        int f = arete.faces[0];
        int g = arete.faces[1];
        int extremites[2] = {arete.a, arete.b};
        for (int s : extremites) {
            reunir(parent, debut[f] + coinDuSommet(table.coins[f], s), debut[g] + coinDuSommet(table.coins[g], s));
        }
    }

    Eventails e;
    e.numero.resize(nbFaces);
    for (int f = 0; f < nbFaces; f++) {
        e.numero[f].assign(table.coins[f].size(), 0);
    }
    e.parSommet.assign(table.nbSommets, 0);
    for (int s = 0; s < table.nbSommets; s++) {
        std::vector<int> racinesVues;
        for (int f : facesDuSommet[s]) {
            int k = coinDuSommet(table.coins[f], s);
            int r = racine(parent, debut[f] + k);
            int numero = -1;
            for (int j = 0; j < (int)racinesVues.size(); j++) {
                if (racinesVues[j] == r) {
                    numero = j;
                }
            }
            if (numero == -1) {
                numero = racinesVues.size();
                racinesVues.push_back(r);
            }
            e.numero[f][k] = numero;
        }
        e.parSommet[s] = racinesVues.size();
        if (racinesVues.size() > 1) {
            e.pinces++;
        }
    }
    return e;
}

}  // namespace

Maillage separerPincements(const Maillage& maillage, const TableAretes& table) {
    Eventails ev = calculerEventails(table);
    if (ev.pinces == 0) {
        return maillage;
    }

    Maillage resultat;
    std::vector<int> premiere(table.nbSommets, -1);
    std::vector<Sommet> copies;
    for (int s = 0; s < table.nbSommets; s++) {
        if (ev.parSommet[s] == 0) {
            continue;
        }
        premiere[s] = copies.size();
        for (int k = 0; k < ev.parSommet[s]; k++) {
            copies.push_back(resultat.add_vertex(maillage.point(maillage.vertex_handle(s))));
        }
    }

    for (int f = 0; f < (int)table.coins.size(); f++) {
        std::vector<Sommet> sommets;
        for (int k = 0; k < (int)table.coins[f].size(); k++) {
            sommets.push_back(copies[premiere[table.coins[f][k]] + ev.numero[f][k]]);
        }
        if (!resultat.add_face(sommets).is_valid()) {
            throw std::runtime_error("separation des pincements : impossible d'ajouter une face");
        }
    }
    return resultat;
}

std::vector<int> morceauDeChaqueFace(const TableAretes& table, int& nbMorceaux) {
    std::vector<int> parent(table.nbSommets);
    for (int s = 0; s < table.nbSommets; s++) {
        parent[s] = s;
    }
    for (const std::vector<int>& coins : table.coins) {
        for (size_t k = 1; k < coins.size(); k++) {
            reunir(parent, coins[0], coins[k]);
        }
    }

    std::vector<int> numero(table.nbSommets, -1);
    std::vector<int> morceau(table.coins.size());
    nbMorceaux = 0;
    for (size_t f = 0; f < table.coins.size(); f++) {
        int r = racine(parent, table.coins[f][0]);
        if (numero[r] == -1) {
            numero[r] = nbMorceaux++;
        }
        morceau[f] = numero[r];
    }
    return morceau;
}

Maillage supprimerPoussieres(const Maillage& maillage, const TableAretes& table, int seuilFaces) {
    int nbMorceaux = 0;
    std::vector<int> morceau = morceauDeChaqueFace(table, nbMorceaux);
    std::vector<int> taille(nbMorceaux, 0);
    for (int m : morceau) {
        taille[m]++;
    }
    bool rienASupprimer = true;
    for (int t : taille) {
        if (t < seuilFaces) {
            rienASupprimer = false;
        }
    }
    if (rienASupprimer) {
        return maillage;
    }

    Maillage resultat;
    std::vector<Sommet> copies(table.nbSommets);
    for (size_t f = 0; f < table.coins.size(); f++) {
        if (taille[morceau[f]] < seuilFaces) {
            continue;
        }
        std::vector<Sommet> sommets;
        for (int s : table.coins[f]) {
            if (!copies[s].is_valid()) {
                copies[s] = resultat.add_vertex(maillage.point(maillage.vertex_handle(s)));
            }
            sommets.push_back(copies[s]);
        }
        if (!resultat.add_face(sommets).is_valid()) {
            throw std::runtime_error("suppression des poussieres : impossible d'ajouter une face");
        }
    }
    return resultat;
}

Maillage reboucher(const Maillage& morceau, const std::vector<Plan>& plans) {
    Maillage resultat = morceau;
    for (const Plan& plan : plans) {
        TableAretes table = construireAretes(resultat);
        FiltreArete surLePlan = [&](const Arete& arete) {
            return plan.contient(resultat.point(resultat.vertex_handle(arete.a)))
                && plan.contient(resultat.point(resultat.vertex_handle(arete.b)));
        };
        std::vector<std::vector<int> > chaines = chainesDeBord(table, surLePlan);

        for (const std::vector<int>& chaine : chaines) {
            if (chaine.size() < 3) {
                continue;
            }
            std::vector<Sommet> sommets;
            for (int s : chaine) {
                sommets.push_back(resultat.vertex_handle(s));
            }
            if (!resultat.add_face(sommets).is_valid()) {
                throw std::runtime_error("reparation : impossible d'ajouter un bouchon");
            }
        }
    }
    return resultat;
}
