#include "bords.hpp"

std::pair<int, int> cleArete(int a, int b) {
    if (a < b) {
        return std::make_pair(a, b);
    }
    return std::make_pair(b, a);
}

std::vector<int> sommetsDeLaFace(const Maillage& maillage, int numeroFace) {
    std::vector<int> sommets;
    for (Sommet s : maillage.fv_range(maillage.face_handle(numeroFace))) {
        sommets.push_back(s.idx());
    }
    return sommets;
}

bool faceParcourt(const std::vector<int>& sommets, int a, int b) {
    int nb = sommets.size();
    for (int i = 0; i < nb; i++) {
        if (sommets[i] == a && sommets[(i + 1) % nb] == b) {
            return true;
        }
    }
    return false;
}

TableAretes construireAretes(const Maillage& maillage) {
    TableAretes aretes;

    for (Face face : maillage.faces()) {
        std::vector<int> sommets = sommetsDeLaFace(maillage, face.idx());
        int nb = sommets.size();
        for (int i = 0; i < nb; i++) {
            int a = sommets[i];
            int b = sommets[(i + 1) % nb];
            aretes[cleArete(a, b)].push_back(face.idx());
        }
    }
    return aretes;
}

std::vector<int> suivreBoucle(const GrapheBords& graphe, int depart, std::set<int>& visites) {
    std::vector<int> boucle;
    int precedent = -1;
    int courant = depart;

    while ((int)boucle.size() <= (int)graphe.size()) {
        boucle.push_back(courant);
        visites.insert(courant);

        const std::vector<int>& voisins = graphe.at(courant);
        int suivant = -1;
        for (int k = 0; k < (int)voisins.size(); k++) {
            if (voisins[k] != precedent) {
                suivant = voisins[k];
                break;
            }
        }
        if (suivant == -1 || visites.count(suivant) > 0) {
            break;
        }
        precedent = courant;
        courant = suivant;
    }
    return boucle;
}

RapportBords analyserBords(const TableAretes& aretes) {
    RapportBords rapport;
    GrapheBords graphe;

    for (TableAretes::const_iterator it = aretes.begin(); it != aretes.end(); ++it) {
        int a = it->first.first;
        int b = it->first.second;
        int nbFaces = it->second.size();

        if (nbFaces > 2) {
            rapport.aretesNonManifold++;
        }
        if (nbFaces == 1) {
            graphe[a].push_back(b);
            graphe[b].push_back(a);
        }
    }

    for (GrapheBords::iterator it = graphe.begin(); it != graphe.end(); ++it) {
        if (it->second.size() != 2) {
            rapport.sommetsPinces++;
        }
    }

    std::set<int> visites;
    for (GrapheBords::iterator it = graphe.begin(); it != graphe.end(); ++it) {
        int sommet = it->first;
        if (visites.count(sommet) == 0) {
            rapport.boucles.push_back(suivreBoucle(graphe, sommet, visites));
        }
    }
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

    texte += "Surface ouverte : " + std::to_string(rapport.boucles.size()) + " bord(s) (";
    for (int i = 0; i < (int)rapport.boucles.size(); i++) {
        if (i > 0) {
            texte += ", ";
        }
        texte += std::to_string(rapport.boucles[i].size()) + " aretes";
    }
    texte += ")";

    if (rapport.boucles.size() == 1 && rapport.aretesNonManifold == 0 && rapport.sommetsPinces == 0) {
        texte += ", chemin unique.";
    }
    return texte;
}
