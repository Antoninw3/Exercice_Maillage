#pragma once

#include <vector>

#include "maillage.hpp"

struct Arete {
    int a;
    int b;
    int faces[2];
    int nbFaces;

    bool estBord() const {
        return nbFaces == 1;
    }
};

struct TableAretes {
    int nbSommets = 0;
    std::vector<Arete> aretes;
    std::vector<std::vector<int> > coins;

    size_t size() const {
        return aretes.size();
    }
    const Arete* trouver(int a, int b) const;
};

std::vector<int> sommetsDeLaFace(const Maillage& maillage, int numeroFace);
bool faceParcourt(const std::vector<int>& sommets, int a, int b);
TableAretes construireAretes(const Maillage& maillage);
