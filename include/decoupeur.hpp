#pragma once

#include <vector>

#include "maillage.hpp"
#include "plan.hpp"

class Decoupeur {
public:
    Decoupeur(const Maillage& maillage);

    std::vector<Maillage> quatreQuarts() const;

    std::vector<Plan> plans() const;

    static Maillage couper(const Maillage& maillage, const Plan& plan, bool garderDevant);

private:
    Maillage maillage;
    Plan planX;
    Plan planY;
};
