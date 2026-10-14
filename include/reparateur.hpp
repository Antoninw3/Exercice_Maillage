#pragma once

#include <vector>

#include "aretes.hpp"
#include "maillage.hpp"
#include "plan.hpp"

Maillage separerPincements(const Maillage& maillage, const TableAretes& table);
std::vector<int> morceauDeChaqueFace(const TableAretes& table, int& nbMorceaux);
Maillage supprimerPoussieres(const Maillage& maillage, const TableAretes& table, int seuilFaces);
Maillage boucherPetitsTrous(const Maillage& nappe, const TableAretes& table, int maxAretes, int& nbBouches);
Maillage reboucher(const Maillage& morceau, const std::vector<Plan>& plans);
