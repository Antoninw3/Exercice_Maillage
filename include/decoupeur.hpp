#pragma once

#include <vector>

#include "maillage.hpp"
#include "plan.hpp"

std::vector<Plan> plansMedians(const Maillage& maillage);
Maillage couper(const Maillage& maillage, const Plan& plan, bool garderDevant);
std::vector<Maillage> quatreQuarts(const Maillage& maillage, const std::vector<Plan>& plans);
