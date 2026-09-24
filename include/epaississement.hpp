#pragma once

#include <limits>
#include <string>
#include <vector>

#include "aretes.hpp"
#include "maillage.hpp"

const float SANS_PLANCHER = std::numeric_limits<float>::quiet_NaN();

Maillage epaissir(const Maillage& nappe, const TableAretes& table, float hauteur, const Point& direction,
                  float plancher);

Maillage extruderFaces(const Maillage& maillage, const TableAretes& table, const std::vector<int>& numerosFaces,
                       float hauteur, const Point& direction);

std::vector<int> facesDuDessus(const Maillage& maillage, const Point& haut);

std::string formaterPlages(const std::vector<int>& numeros);
