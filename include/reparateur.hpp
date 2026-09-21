#pragma once

#include <vector>

#include "maillage.hpp"
#include "plan.hpp"

class Reparateur {
public:
    Reparateur(const std::vector<Plan>& plans);

    Maillage reboucher(const Maillage& morceau);

    int nombreBouchons() const;

private:
    std::vector<Plan> plans;
    int bouchons;
};
