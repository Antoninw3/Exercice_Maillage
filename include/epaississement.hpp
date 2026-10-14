#pragma once

#include <limits>
#include <string>
#include <vector>

#include "aretes.hpp"
#include "maillage.hpp"

const float SANS_SOCLE = std::numeric_limits<float>::quiet_NaN();

Maillage epaissir(const Maillage& nappe, const TableAretes& table, float hauteur, float largeurBille,
                  const Point& direction);

Maillage epaissirAvecPattes(const Maillage& nappe, const TableAretes& table, float hauteur, float socle);

Maillage remplirJusquAuSol(const Maillage& nappe, const TableAretes& table, float socle);

float zMinimum(const Maillage& maillage, const TableAretes& table);

Maillage extruderFaces(const Maillage& maillage, const TableAretes& table, const std::vector<int>& numerosFaces,
                       float hauteur, const Point& direction);

std::vector<int> facesDuDessus(const Maillage& maillage, const Point& haut);

std::string formaterPlages(const std::vector<int>& numeros);
