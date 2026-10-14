#include "triangulation.hpp"

#include <cmath>
#include <map>
#include <queue>
#include <utility>

namespace {

double croix(const Point2D& o, const Point2D& a, const Point2D& b) {
    return (a[0] - o[0]) * (b[1] - o[1]) - (a[1] - o[1]) * (b[0] - o[0]);
}

double longueur(const Point2D& a, const Point2D& b) {
    return std::hypot(b[0] - a[0], b[1] - a[1]);
}

double angleMinimal(const Point2D& a, const Point2D& b, const Point2D& c) {
    double ab = longueur(a, b);
    double bc = longueur(b, c);
    double ca = longueur(c, a);
    if (ab == 0 || bc == 0 || ca == 0) {
        return 0;
    }
    double aire2 = std::fabs(croix(a, b, c));
    double sinA = aire2 / (ab * ca);
    double sinB = aire2 / (ab * bc);
    double sinC = aire2 / (bc * ca);
    double s = std::fmin(sinA, std::fmin(sinB, sinC));
    return std::asin(std::fmin(1.0, s));
}

bool dedans(const Point2D& p, const Point2D& a, const Point2D& b, const Point2D& c, double sens) {
    return croix(a, b, p) * sens > 0 && croix(b, c, p) * sens > 0 && croix(c, a, p) * sens > 0;
}

struct Oreille {
    double qualite;
    int sommet;
    int version;

    bool operator<(const Oreille& autre) const {
        return qualite < autre.qualite;
    }
};

}  // namespace

double aireSignee(const std::vector<Point2D>& polygone) {
    double aire = 0;
    for (size_t i = 0; i < polygone.size(); i++) {
        const Point2D& p = polygone[i];
        const Point2D& q = polygone[(i + 1) % polygone.size()];
        aire += p[0] * q[1] - q[0] * p[1];
    }
    return aire / 2;
}

void basculer(const std::vector<Point2D>& p, std::vector<int>& triangles) {
    int nbTriangles = triangles.size() / 3;
    std::map<std::pair<int, int>, int> parArete;
    for (int t = 0; t < nbTriangles; t++) {
        for (int k = 0; k < 3; k++) {
            parArete[{triangles[3 * t + k], triangles[3 * t + (k + 1) % 3]}] = t;
        }
    }
    auto dansLeCercle = [&](int a, int b, int c, int d) {
        double ax = p[a][0] - p[d][0], ay = p[a][1] - p[d][1];
        double bx = p[b][0] - p[d][0], by = p[b][1] - p[d][1];
        double cx = p[c][0] - p[d][0], cy = p[c][1] - p[d][1];
        double det = (ax * ax + ay * ay) * (bx * cy - cx * by) - (bx * bx + by * by) * (ax * cy - cx * ay)
                   + (cx * cx + cy * cy) * (ax * by - bx * ay);
        double echelle = (ax * ax + ay * ay + bx * bx + by * by + cx * cx + cy * cy);
        return det > 1e-9 * echelle * echelle;
    };
    std::vector<std::pair<int, int> > aVoir;
    for (const auto& kv : parArete) {
        aVoir.push_back(kv.first);
    }
    size_t limite = 20 * parArete.size() + 100;
    while (!aVoir.empty() && limite-- > 0) {
        std::pair<int, int> arete = aVoir.back();
        aVoir.pop_back();
        int a = arete.first;
        int b = arete.second;
        std::map<std::pair<int, int>, int>::iterator t1 = parArete.find({a, b});
        std::map<std::pair<int, int>, int>::iterator t2 = parArete.find({b, a});
        if (t1 == parArete.end() || t2 == parArete.end()) {
            continue;
        }
        int u = t1->second;
        int v = t2->second;
        int c = -1;
        int d = -1;
        for (int k = 0; k < 3; k++) {
            if (triangles[3 * u + k] != a && triangles[3 * u + k] != b) c = triangles[3 * u + k];
            if (triangles[3 * v + k] != a && triangles[3 * v + k] != b) d = triangles[3 * v + k];
        }
        if (croix(p[a], p[d], p[c]) <= 0 || croix(p[d], p[b], p[c]) <= 0) {
            continue;
        }
        if (!dansLeCercle(a, b, c, d)) {
            continue;
        }
        for (int k = 0; k < 3; k++) {
            parArete.erase({triangles[3 * u + k], triangles[3 * u + (k + 1) % 3]});
            parArete.erase({triangles[3 * v + k], triangles[3 * v + (k + 1) % 3]});
        }
        triangles[3 * u] = a; triangles[3 * u + 1] = d; triangles[3 * u + 2] = c;
        triangles[3 * v] = d; triangles[3 * v + 1] = b; triangles[3 * v + 2] = c;
        for (int t : {u, v}) {
            for (int k = 0; k < 3; k++) {
                std::pair<int, int> e(triangles[3 * t + k], triangles[3 * t + (k + 1) % 3]);
                parArete[e] = t;
                aVoir.push_back(e);
            }
        }
    }
}

std::vector<int> trianguler(const std::vector<Point2D>& contour) {
    int n = contour.size();
    std::vector<int> triangles;
    if (n < 3) {
        return triangles;
    }
    std::vector<Point2D> polygone = contour;
    if (aireSignee(contour) < 0) {
        for (Point2D& q : polygone) {
            q[0] = -q[0];
        }
    }
    const double sens = 1;

    std::vector<int> precedent(n);
    std::vector<int> suivant(n);
    std::vector<bool> retire(n, false);
    std::vector<int> version(n, 0);
    for (int i = 0; i < n; i++) {
        precedent[i] = (i + n - 1) % n;
        suivant[i] = (i + 1) % n;
    }

    auto qualite = [&](int i) {
        const Point2D& a = polygone[precedent[i]];
        const Point2D& b = polygone[i];
        const Point2D& c = polygone[suivant[i]];
        if (croix(a, b, c) * sens <= 0) {
            return -1.0;
        }
        for (int j = suivant[suivant[i]]; j != precedent[i]; j = suivant[j]) {
            if (dedans(polygone[j], a, b, c, sens)) {
                return -1.0;
            }
        }
        return angleMinimal(a, b, c);
    };

    std::priority_queue<Oreille> oreilles;
    for (int i = 0; i < n; i++) {
        oreilles.push({qualite(i), i, 0});
    }

    int restants = n;
    while (restants > 3) {
        int i;
        if (oreilles.empty()) {
            i = 0;
            while (retire[i]) {
                i++;
            }
        } else {
            Oreille o = oreilles.top();
            oreilles.pop();
            if (retire[o.sommet] || o.version != version[o.sommet]) {
                continue;
            }
            i = o.sommet;
        }
        int a = precedent[i];
        int c = suivant[i];
        triangles.push_back(a);
        triangles.push_back(i);
        triangles.push_back(c);
        retire[i] = true;
        suivant[a] = c;
        precedent[c] = a;
        restants--;
        for (int voisin : {a, c}) {
            version[voisin]++;
            oreilles.push({qualite(voisin), voisin, version[voisin]});
        }
    }
    int i = 0;
    while (retire[i]) {
        i++;
    }
    triangles.push_back(precedent[i]);
    triangles.push_back(i);
    triangles.push_back(suivant[i]);
    basculer(polygone, triangles);
    return triangles;
}
