#include "bords.hpp"

#include <algorithm>
#include <unordered_map>

namespace {

struct Oriente {
    int depart;
    int arrivee;
};

typedef std::unordered_map<int, std::vector<int> > Sortants;

std::vector<Oriente> orienterBords(const TableAretes& table, const FiltreArete& filtre) {
    std::vector<Oriente> bords;
    for (const Arete& arete : table.aretes) {
        if (!arete.estBord() || !filtre(arete)) {
            continue;
        }
        if (faceParcourt(table.coins[arete.faces[0]], arete.a, arete.b)) {
            bords.push_back({arete.b, arete.a});
        } else {
            bords.push_back({arete.a, arete.b});
        }
    }
    return bords;
}

int prochainNonVisite(const Sortants& sortants, const std::vector<bool>& visite, int sommet) {
    Sortants::const_iterator it = sortants.find(sommet);
    if (it == sortants.end()) {
        return -1;
    }
    for (int candidat : it->second) {
        if (!visite[candidat]) {
            return candidat;
        }
    }
    return -1;
}

std::vector<int> suivre(const std::vector<Oriente>& bords, const Sortants& sortants, std::vector<bool>& visite, int premier) {
    std::vector<int> chaine;
    int courant = premier;
    int arrivee = bords[premier].depart;
    while (courant != -1) {
        visite[courant] = true;
        chaine.push_back(bords[courant].depart);
        arrivee = bords[courant].arrivee;
        courant = prochainNonVisite(sortants, visite, arrivee);
    }
    if (arrivee != chaine.front()) {
        chaine.push_back(arrivee);
    }
    return chaine;
}

}  // namespace

std::vector<std::vector<int> > chainesDeBord(const TableAretes& table, const FiltreArete& filtre) {
    std::vector<Oriente> bords = orienterBords(table, filtre);
    Sortants sortants;
    std::unordered_map<int, int> entrants;
    for (int i = 0; i < (int)bords.size(); i++) {
        sortants[bords[i].depart].push_back(i);
        entrants[bords[i].arrivee]++;
    }

    std::vector<bool> visite(bords.size(), false);
    std::vector<std::vector<int> > chaines;
    for (int i = 0; i < (int)bords.size(); i++) {
        int depart = bords[i].depart;
        if (!visite[i] && (int)sortants[depart].size() != entrants[depart]) {
            chaines.push_back(suivre(bords, sortants, visite, i));
        }
    }
    for (int i = 0; i < (int)bords.size(); i++) {
        if (!visite[i]) {
            chaines.push_back(suivre(bords, sortants, visite, i));
        }
    }
    return chaines;
}

int compterPincements(const TableAretes& table) {
    std::unordered_map<int, int> degre;
    for (const Arete& arete : table.aretes) {
        if (arete.estBord()) {
            degre[arete.a]++;
            degre[arete.b]++;
        }
    }
    int pinces = 0;
    for (const auto& kv : degre) {
        if (kv.second != 2) {
            pinces++;
        }
    }
    return pinces;
}

RapportBords analyserBords(const TableAretes& table) {
    RapportBords rapport;
    for (const Arete& arete : table.aretes) {
        if (arete.nbFaces > 2) {
            rapport.aretesNonManifold++;
        }
    }
    rapport.sommetsPinces = compterPincements(table);
    rapport.boucles = chainesDeBord(table, [](const Arete&) { return true; });
    return rapport;
}

std::string decrireBords(const RapportBords& rapport) {
    std::string texte;
    if (rapport.aretesNonManifold > 0) {
        texte += "NON-MANIFOLD : " + std::to_string(rapport.aretesNonManifold) + " arete(s) avec plus de 2 faces. ";
    }
    if (rapport.sommetsPinces > 0) {
        texte += "PINCEMENT : " + std::to_string(rapport.sommetsPinces) + " sommet(s) de bord ambigu(s). ";
    }
    if (rapport.boucles.empty()) {
        texte += "Surface fermee, aucun bord.";
        return texte;
    }

    texte += "Surface ouverte : " + std::to_string(rapport.boucles.size()) + " bord(s)";
    const int maxDetail = 10;
    if ((int)rapport.boucles.size() <= maxDetail) {
        texte += " (";
        for (int i = 0; i < (int)rapport.boucles.size(); i++) {
            if (i > 0) {
                texte += ", ";
            }
            texte += std::to_string(rapport.boucles[i].size()) + " aretes";
        }
        texte += ")";
    } else {
        size_t total = 0;
        size_t plusGrand = 0;
        int petits = 0;
        for (const std::vector<int>& boucle : rapport.boucles) {
            total += boucle.size();
            plusGrand = std::max(plusGrand, boucle.size());
            if (boucle.size() <= 4) {
                petits++;
            }
        }
        texte += ", " + std::to_string(total) + " aretes de bord au total, plus grand bord : "
               + std::to_string(plusGrand) + " aretes, " + std::to_string(petits)
               + " bord(s) de 4 aretes ou moins";
    }
    if (rapport.boucles.size() == 1 && rapport.aretesNonManifold == 0 && rapport.sommetsPinces == 0) {
        texte += ", chemin unique.";
    }
    return texte;
}
