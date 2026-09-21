#include "decoupeur.hpp"

#include <algorithm>
#include <map>
#include <stdexcept>

#include "bords.hpp"

Decoupeur::Decoupeur(const Maillage& m) : maillage(m) {
    Point minimum(0, 0, 0);
    Point maximum(0, 0, 0);
    if (maillage.n_vertices() > 0) {
        minimum = maillage.point(maillage.vertex_handle(0));
        maximum = minimum;
    }
    for (Sommet s : maillage.vertices()) {
        minimum.minimize(maillage.point(s));
        maximum.maximize(maillage.point(s));
    }

    Point centre = (minimum + maximum) * 0.5f;
    float tolerance = std::max((maximum - minimum).norm() * 1e-5f, 1e-7f);

    planX.origine = centre;
    planX.normale = Point(1, 0, 0);
    planX.tolerance = tolerance;

    planY.origine = centre;
    planY.normale = Point(0, 1, 0);
    planY.tolerance = tolerance;
}

std::vector<Plan> Decoupeur::plans() const {
    std::vector<Plan> liste;
    liste.push_back(planX);
    liste.push_back(planY);
    return liste;
}

Maillage Decoupeur::couper(const Maillage& maillage, const Plan& plan, bool garderDevant) {
    int nb = maillage.n_vertices();

    std::vector<float> distances(nb);
    for (Sommet s : maillage.vertices()) {
        Point p = maillage.point(s);
        if (plan.contient(p)) {
            distances[s.idx()] = 0;
        } else {
            distances[s.idx()] = plan.distance(p);
        }
    }

    Maillage resultat;
    std::vector<Sommet> copies(nb);
    std::map<std::pair<int, int>, Sommet> intersections;

    for (Face face : maillage.faces()) {
        std::vector<int> sommets;
        for (Sommet s : maillage.fv_range(face)) {
            sommets.push_back(s.idx());
        }

        std::vector<Sommet> polygone;
        int taille = sommets.size();
        for (int i = 0; i < taille; i++) {
            int courant = sommets[i];
            int suivant = sommets[(i + 1) % taille];

            bool dedans;
            if (garderDevant) {
                dedans = distances[courant] >= 0;
            } else {
                dedans = distances[courant] <= 0;
            }

            if (dedans) {
                if (!copies[courant].is_valid()) {
                    copies[courant] = resultat.add_vertex(maillage.point(maillage.vertex_handle(courant)));
                }
                polygone.push_back(copies[courant]);
            }

            bool traverse = distances[courant] * distances[suivant] < 0;
            if (traverse) {
                std::pair<int, int> cle = cleArete(courant, suivant);
                if (intersections.count(cle) == 0) {
                    Point a = maillage.point(maillage.vertex_handle(courant));
                    Point b = maillage.point(maillage.vertex_handle(suivant));
                    float t = distances[courant] / (distances[courant] - distances[suivant]);
                    intersections[cle] = resultat.add_vertex(a + (b - a) * t);
                }
                polygone.push_back(intersections[cle]);
            }
        }

        if (polygone.size() < 3) {
            continue;
        }
        if (!resultat.add_face(polygone).is_valid()) {
            throw std::runtime_error("decoupe : impossible d'ajouter une face coupee");
        }
    }

    return resultat;
}

std::vector<Maillage> Decoupeur::quatreQuarts() const {
    Maillage devant = couper(maillage, planX, true);
    Maillage derriere = couper(maillage, planX, false);

    std::vector<Maillage> quarts;
    quarts.push_back(couper(devant, planY, true));
    quarts.push_back(couper(devant, planY, false));
    quarts.push_back(couper(derriere, planY, true));
    quarts.push_back(couper(derriere, planY, false));
    return quarts;
}
