#include "decoupeur.hpp"

#include <algorithm>
#include <map>
#include <stdexcept>

#include "aretes.hpp"

std::vector<Plan> plansMedians(const Maillage& maillage) {
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

    std::vector<Plan> plans;
    plans.push_back({centre, Point(1, 0, 0), tolerance});
    plans.push_back({centre, Point(0, 1, 0), tolerance});
    return plans;
}

Maillage couper(const Maillage& maillage, const Plan& plan, bool garderDevant) {
    int nb = maillage.n_vertices();
    std::vector<float> distances(nb);
    for (Sommet s : maillage.vertices()) {
        Point p = maillage.point(s);
        distances[s.idx()] = plan.contient(p) ? 0 : plan.distance(p);
    }

    Maillage resultat;
    std::vector<Sommet> copies(nb);
    std::map<std::pair<int, int>, Sommet> intersections;

    for (Face face : maillage.faces()) {
        std::vector<int> sommets = sommetsDeLaFace(maillage, face.idx());
        std::vector<Sommet> polygone;
        int taille = sommets.size();
        for (int i = 0; i < taille; i++) {
            int courant = sommets[i];
            int suivant = sommets[(i + 1) % taille];
            bool dedans = garderDevant ? distances[courant] >= 0 : distances[courant] <= 0;

            if (dedans) {
                if (!copies[courant].is_valid()) {
                    copies[courant] = resultat.add_vertex(maillage.point(maillage.vertex_handle(courant)));
                }
                polygone.push_back(copies[courant]);
            }

            if (distances[courant] * distances[suivant] < 0) {
                std::pair<int, int> cle(std::min(courant, suivant), std::max(courant, suivant));
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

std::vector<Maillage> quatreQuarts(const Maillage& maillage, const std::vector<Plan>& plans) {
    Maillage devant = couper(maillage, plans[0], true);
    Maillage derriere = couper(maillage, plans[0], false);

    std::vector<Maillage> quarts;
    quarts.push_back(couper(devant, plans[1], true));
    quarts.push_back(couper(devant, plans[1], false));
    quarts.push_back(couper(derriere, plans[1], true));
    quarts.push_back(couper(derriere, plans[1], false));
    return quarts;
}
