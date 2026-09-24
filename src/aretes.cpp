#include "aretes.hpp"

#include <algorithm>

std::vector<int> sommetsDeLaFace(const Maillage& maillage, int numeroFace) {
    std::vector<int> sommets;
    for (Sommet s : maillage.fv_range(maillage.face_handle(numeroFace))) {
        sommets.push_back(s.idx());
    }
    return sommets;
}

bool faceParcourt(const std::vector<int>& sommets, int a, int b) {
    int n = sommets.size();
    for (int i = 0; i < n; i++) {
        if (sommets[i] == a && sommets[(i + 1) % n] == b) {
            return true;
        }
    }
    return false;
}

namespace {

struct Brin {
    int a;
    int b;
    int face;
};

bool operator<(const Brin& x, const Brin& y) {
    if (x.a != y.a) {
        return x.a < y.a;
    }
    if (x.b != y.b) {
        return x.b < y.b;
    }
    return x.face < y.face;
}

}  // namespace

TableAretes construireAretes(const Maillage& maillage) {
    TableAretes table;
    table.nbSommets = maillage.n_vertices();
    table.coins.resize(maillage.n_faces());

    std::vector<Brin> brins;
    brins.reserve(3 * maillage.n_faces());
    for (Face face : maillage.faces()) {
        std::vector<int>& c = table.coins[face.idx()];
        c = sommetsDeLaFace(maillage, face.idx());
        int n = c.size();
        for (int i = 0; i < n; i++) {
            int a = c[i];
            int b = c[(i + 1) % n];
            brins.push_back({std::min(a, b), std::max(a, b), face.idx()});
        }
    }
    std::sort(brins.begin(), brins.end());

    for (size_t i = 0; i < brins.size();) {
        Arete arete;
        arete.a = brins[i].a;
        arete.b = brins[i].b;
        arete.nbFaces = 0;
        arete.faces[0] = -1;
        arete.faces[1] = -1;
        size_t j = i;
        while (j < brins.size() && brins[j].a == arete.a && brins[j].b == arete.b) {
            if (arete.nbFaces < 2) {
                arete.faces[arete.nbFaces] = brins[j].face;
            }
            arete.nbFaces++;
            j++;
        }
        table.aretes.push_back(arete);
        i = j;
    }
    return table;
}

const Arete* TableAretes::trouver(int a, int b) const {
    if (a > b) {
        std::swap(a, b);
    }
    size_t bas = 0;
    size_t haut = aretes.size();
    while (bas < haut) {
        size_t milieu = (bas + haut) / 2;
        const Arete& e = aretes[milieu];
        if (e.a < a || (e.a == a && e.b < b)) {
            bas = milieu + 1;
        } else {
            haut = milieu;
        }
    }
    if (bas < aretes.size() && aretes[bas].a == a && aretes[bas].b == b) {
        return &aretes[bas];
    }
    return nullptr;
}
