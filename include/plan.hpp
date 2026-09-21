#pragma once

#include <cmath>

#include "maillage.hpp"

struct Plan {
    Point origine;
    Point normale;
    float tolerance;

    float distance(const Point& p) const {
        return OpenMesh::dot(p - origine, normale);
    }

    bool contient(const Point& p) const {
        return std::fabs(distance(p)) <= tolerance;
    }
};
