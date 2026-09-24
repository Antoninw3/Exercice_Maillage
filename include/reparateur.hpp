#pragma once

#include <vector>

#include "aretes.hpp"
#include "maillage.hpp"
#include "plan.hpp"

Maillage separerPincements(const Maillage& maillage, const TableAretes& table);
std::vector<int> morceauDeChaqueFace(const TableAretes& table, int& nbMorceaux);
Maillage supprimerPoussieres(const Maillage& maillage, const TableAretes& table, int seuilFaces);
Maillage reboucher(const Maillage& morceau, const std::vector<Plan>& plans);
