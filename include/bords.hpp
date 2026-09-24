#pragma once

#include <functional>
#include <string>
#include <vector>

#include "aretes.hpp"

struct RapportBords {
    int aretesNonManifold = 0;
    int sommetsPinces = 0;
    std::vector<std::vector<int> > boucles;

    bool estFerme() const {
        return boucles.empty() && aretesNonManifold == 0;
    }
};

typedef std::function<bool(const Arete&)> FiltreArete;

std::vector<std::vector<int> > chainesDeBord(const TableAretes& table, const FiltreArete& filtre);
int compterPincements(const TableAretes& table);
RapportBords analyserBords(const TableAretes& table);
std::string decrireBords(const RapportBords& rapport);
