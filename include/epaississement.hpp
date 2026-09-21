#pragma once

#include <string>
#include <vector>

#include "maillage.hpp"

Maillage epaissir(const Maillage& surface, float hauteur, const Point& direction);

Maillage extruderFaces(const Maillage& maillage, const std::vector<int>& numerosFaces, float hauteur, const Point& direction);

std::vector<int> facesDuDessus(const Maillage& maillage, const Point& haut);

std::string formaterPlages(const std::vector<int>& numeros);
