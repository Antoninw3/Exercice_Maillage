#include "allegement.hpp"

#include "triangulation.hpp"

#include <OpenMesh/Core/Mesh/TriMesh_ArrayKernelT.hh>
#include <OpenMesh/Tools/Decimater/DecimaterT.hh>
#include <OpenMesh/Tools/Decimater/ModBaseT.hh>

#include <algorithm>
#include <cmath>
#include <vector>

namespace {

typedef OpenMesh::TriMesh_ArrayKernelT<> Triangles;
typedef OpenMesh::Vec3d Vecteur;

const double ECART_PLAN = 1e-5;

const double ANGLE_MINIMAL = 2.0 * M_PI / 180.0;

Vecteur enDouble(const Triangles::Point& p) {
    return Vecteur(p[0], p[1], p[2]);
}

double angleMinimal(const Vecteur& a, const Vecteur& b, const Vecteur& c) {
    double ab = (b - a).norm();
    double bc = (c - b).norm();
    double ca = (a - c).norm();
    if (ab == 0 || bc == 0 || ca == 0) {
        return 0;
    }
    double aire2 = OpenMesh::cross(b - a, c - a).norm();
    double s = std::min({aire2 / (ab * ca), aire2 / (ab * bc), aire2 / (bc * ca)});
    return std::asin(std::min(1.0, s));
}

template <class MeshT>
class ModSansPerteT : public OpenMesh::Decimater::ModBaseT<MeshT> {
public:
    DECIMATING_MODULE(ModSansPerteT, MeshT, SansPerte);

    explicit ModSansPerteT(MeshT& maillage) : Base(maillage, false) {}

    float collapse_priority(const CollapseInfo& ci) override {
        const MeshT& m = Base::mesh();
        Vecteur p1 = enDouble(ci.p1);
        for (typename MeshT::ConstVertexFaceIter vf = m.cvf_iter(ci.v0); vf.is_valid(); ++vf) {
            if (*vf == ci.fl || *vf == ci.fr) {
                continue;
            }
            Vecteur ancien[3];
            Vecteur nouveau[3];
            int k = 0;
            for (typename MeshT::ConstFaceVertexIter fv = m.cfv_iter(*vf); fv.is_valid() && k < 3; ++fv, ++k) {
                ancien[k] = enDouble(m.point(*fv));
                nouveau[k] = *fv == ci.v0 ? p1 : ancien[k];
            }
            Vecteur normale = OpenMesh::cross(ancien[1] - ancien[0], ancien[2] - ancien[0]);
            Vecteur normaleNouvelle = OpenMesh::cross(nouveau[1] - nouveau[0], nouveau[2] - nouveau[0]);
            double longueur = normale.norm();
            if (longueur == 0) {
                return Base::ILLEGAL_COLLAPSE;
            }
            if (std::fabs(OpenMesh::dot(p1 - ancien[0], normale)) / longueur > ECART_PLAN) {
                return Base::ILLEGAL_COLLAPSE;
            }
            if (OpenMesh::dot(normale, normaleNouvelle) <= 0) {
                return Base::ILLEGAL_COLLAPSE;
            }
            double avant = angleMinimal(ancien[0], ancien[1], ancien[2]);
            double apres = angleMinimal(nouveau[0], nouveau[1], nouveau[2]);
            if (apres < std::min(ANGLE_MINIMAL, avant)) {
                return Base::ILLEGAL_COLLAPSE;
            }
        }
        return (ci.p1 - ci.p0).sqrnorm();
    }
};

Triangles versTriangles(const Maillage& solide) {
    Triangles resultat;
    std::vector<Triangles::VertexHandle> sommets;
    sommets.reserve(solide.n_vertices());
    for (Sommet s : solide.vertices()) {
        sommets.push_back(resultat.add_vertex(solide.point(s)));
    }
    for (Face f : solide.faces()) {
        std::vector<Triangles::VertexHandle> coins;
        for (Sommet s : solide.fv_range(f)) {
            coins.push_back(sommets[s.idx()]);
        }
        if (coins.size() == 3) {
            resultat.add_face(coins[0], coins[1], coins[2]);
            continue;
        }
        std::vector<Point2D> polygone;
        Vecteur normale(0, 0, 0);
        for (size_t k = 0; k < coins.size(); k++) {
            Vecteur a = enDouble(resultat.point(coins[k]));
            Vecteur b = enDouble(resultat.point(coins[(k + 1) % coins.size()]));
            normale += OpenMesh::cross(a, b);
        }
        int dominant = 0;
        for (int k = 1; k < 3; k++) {
            if (std::fabs(normale[k]) > std::fabs(normale[dominant])) {
                dominant = k;
            }
        }
        for (Triangles::VertexHandle s : coins) {
            Triangles::Point p = resultat.point(s);
            polygone.push_back({p[(dominant + 1) % 3], p[(dominant + 2) % 3]});
        }
        std::vector<int> triangles = trianguler(polygone);
        for (size_t k = 0; k + 2 < triangles.size(); k += 3) {
            resultat.add_face(coins[triangles[k]], coins[triangles[k + 1]], coins[triangles[k + 2]]);
        }
    }
    return resultat;
}

Maillage versMaillage(const Triangles& triangles) {
    Maillage resultat;
    std::vector<Sommet> sommets(triangles.n_vertices());
    for (Triangles::VertexHandle s : triangles.vertices()) {
        sommets[s.idx()] = resultat.add_vertex(triangles.point(s));
    }
    for (Triangles::FaceHandle f : triangles.faces()) {
        std::vector<Sommet> coins;
        for (Triangles::VertexHandle s : triangles.fv_range(f)) {
            coins.push_back(sommets[s.idx()]);
        }
        resultat.add_face(coins);
    }
    return resultat;
}

}  // namespace

Maillage alleger(const Maillage& solide, int& nbFusions) {
    Triangles triangles = versTriangles(solide);
    OpenMesh::Decimater::DecimaterT<Triangles> decimateur(triangles);
    ModSansPerteT<Triangles>::Handle sansPerte;
    decimateur.add(sansPerte);
    decimateur.initialize();
    nbFusions = decimateur.decimate();
    triangles.garbage_collection();
    return versMaillage(triangles);
}
