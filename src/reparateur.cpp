#include "reparateur.hpp"

#include <algorithm>
#include <stdexcept>

#include "bords.hpp"

Reparateur::Reparateur(const std::vector<Plan>& p) : plans(p), bouchons(0) {}

int Reparateur::nombreBouchons() const {
    return bouchons;
}

GrapheBords grapheDesBordsSurLePlan(const Maillage& maillage, const TableAretes& aretes, const Plan& plan) {
    GrapheBords graphe;
    for (TableAretes::const_iterator it = aretes.begin(); it != aretes.end(); ++it) {
        if (it->second.size() != 1) {
            continue;
        }
        int a = it->first.first;
        int b = it->first.second;
        Point pointA = maillage.point(maillage.vertex_handle(a));
        Point pointB = maillage.point(maillage.vertex_handle(b));
        if (plan.contient(pointA) && plan.contient(pointB)) {
            graphe[a].push_back(b);
            graphe[b].push_back(a);
        }
    }
    return graphe;
}

std::vector<std::vector<int> > chainesDuGraphe(const GrapheBords& graphe) {
    std::vector<std::vector<int> > chaines;
    std::set<int> visites;

    for (GrapheBords::const_iterator it = graphe.begin(); it != graphe.end(); ++it) {
        if (it->second.size() == 1 && visites.count(it->first) == 0) {
            chaines.push_back(suivreBoucle(graphe, it->first, visites));
        }
    }
    for (GrapheBords::const_iterator it = graphe.begin(); it != graphe.end(); ++it) {
        if (visites.count(it->first) == 0) {
            chaines.push_back(suivreBoucle(graphe, it->first, visites));
        }
    }
    return chaines;
}

void orienterChaine(const Maillage& maillage, const TableAretes& aretes, std::vector<int>& chaine) {
    int a = chaine[0];
    int b = chaine[1];
    int faceVoisine = aretes.at(cleArete(a, b))[0];
    if (faceParcourt(sommetsDeLaFace(maillage, faceVoisine), a, b)) {
        std::reverse(chaine.begin(), chaine.end());
    }
}

Maillage Reparateur::reboucher(const Maillage& morceau) {
    Maillage resultat = morceau;
    bouchons = 0;

    for (int p = 0; p < (int)plans.size(); p++) {
        TableAretes aretes = construireAretes(resultat);
        GrapheBords graphe = grapheDesBordsSurLePlan(resultat, aretes, plans[p]);
        std::vector<std::vector<int> > chaines = chainesDuGraphe(graphe);

        for (int c = 0; c < (int)chaines.size(); c++) {
            if (chaines[c].size() < 3) {
                continue;
            }
            orienterChaine(resultat, aretes, chaines[c]);

            std::vector<Sommet> sommets;
            for (int i = 0; i < (int)chaines[c].size(); i++) {
                sommets.push_back(resultat.vertex_handle(chaines[c][i]));
            }
            if (!resultat.add_face(sommets).is_valid()) {
                throw std::runtime_error("reparation : impossible d'ajouter un bouchon");
            }
            bouchons++;
        }
    }
    return resultat;
}
