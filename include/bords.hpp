#pragma once

#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "maillage.hpp"

typedef std::map<std::pair<int, int>, std::vector<int> > TableAretes;
typedef std::map<int, std::vector<int> > GrapheBords;

struct RapportBords {
    int aretesNonManifold = 0;
    int sommetsPinces = 0;
    std::vector<std::vector<int> > boucles;

    bool estFerme() const {
        return boucles.empty() && aretesNonManifold == 0;
    }
};

std::pair<int, int> cleArete(int a, int b);

std::vector<int> sommetsDeLaFace(const Maillage& maillage, int numeroFace);

bool faceParcourt(const std::vector<int>& sommets, int a, int b);

TableAretes construireAretes(const Maillage& maillage);

std::vector<int> suivreBoucle(const GrapheBords& graphe, int depart, std::set<int>& visites);

RapportBords analyserBords(const TableAretes& aretes);

std::string decrireBords(const RapportBords& rapport);
