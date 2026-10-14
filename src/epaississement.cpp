#include "epaississement.hpp"

#include "bords.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <queue>
#include <set>
#include <stdexcept>

namespace {

void ajouterFace(Maillage& maillage, const std::vector<Sommet>& sommets, const std::string& quoi) {
    if (!maillage.add_face(sommets).is_valid()) {
        throw std::runtime_error("impossible d'ajouter " + quoi);
    }
}

Point normaleGlobale(const Maillage& source, const TableAretes& table) {
    Point somme(0, 0, 0);
    for (const std::vector<int>& coins : table.coins) {
        if (coins.size() < 3) {
            continue;
        }
        Point a = source.point(source.vertex_handle(coins[0]));
        Point b = source.point(source.vertex_handle(coins[1]));
        Point c = source.point(source.vertex_handle(coins[2]));
        somme += OpenMesh::cross(b - a, c - a);
    }
    return somme;
}

Point auNiveau(const Point& p, float z) {
    return Point(p[0], p[1], z);
}

Sommet sommetOuCreer(Maillage& resultat, const Maillage& source, std::vector<Sommet>& copies, int s) {
    if (!copies[s].is_valid()) {
        copies[s] = resultat.add_vertex(source.point(source.vertex_handle(s)));
    }
    return copies[s];
}

std::vector<float> distanceAuBord(const Maillage& nappe, const TableAretes& table, float limite) {
    std::vector<std::vector<int> > voisins(table.nbSommets);
    std::vector<float> distance(table.nbSommets, std::numeric_limits<float>::infinity());
    typedef std::pair<float, int> Etape;
    std::priority_queue<Etape, std::vector<Etape>, std::greater<Etape> > file;
    for (const Arete& arete : table.aretes) {
        voisins[arete.a].push_back(arete.b);
        voisins[arete.b].push_back(arete.a);
        if (arete.estBord()) {
            for (int s : {arete.a, arete.b}) {
                if (distance[s] > 0) {
                    distance[s] = 0;
                    file.push({0, s});
                }
            }
        }
    }
    while (!file.empty()) {
        Etape etape = file.top();
        file.pop();
        int s = etape.second;
        if (etape.first > distance[s]) {
            continue;
        }
        Point p = nappe.point(nappe.vertex_handle(s));
        for (int t : voisins[s]) {
            float d = etape.first + (nappe.point(nappe.vertex_handle(t)) - p).norm();
            if (d < distance[t] && d <= limite) {
                distance[t] = d;
                file.push({d, t});
            }
        }
    }
    return distance;
}

std::vector<std::vector<int> > coinsVersLeHaut(const Maillage& nappe, const TableAretes& table) {
    std::vector<std::vector<int> > coins = table.coins;
    if (normaleGlobale(nappe, table)[2] < 0) {
        for (std::vector<int>& face : coins) {
            std::reverse(face.begin(), face.end());
        }
    }
    return coins;
}

int racine(std::vector<int>& parent, int x) {
    while (parent[x] != x) {
        parent[x] = parent[parent[x]];
        x = parent[x];
    }
    return x;
}

struct Eventails {
    std::vector<int> debut;
    std::vector<int> numero;

    int coin(int face, int position) const {
        return numero[debut[face] + position];
    }
};

Eventails eventailsParClasse(const TableAretes& table, const std::vector<bool>& classe) {
    Eventails resultat;
    int nbCoins = 0;
    for (const std::vector<int>& coins : table.coins) {
        resultat.debut.push_back(nbCoins);
        nbCoins += coins.size();
    }
    std::vector<int> parent(nbCoins);
    for (int c = 0; c < nbCoins; c++) {
        parent[c] = c;
    }
    auto position = [&](int face, int sommet) {
        const std::vector<int>& coins = table.coins[face];
        for (int k = 0; k < (int)coins.size(); k++) {
            if (coins[k] == sommet) {
                return resultat.debut[face] + k;
            }
        }
        return -1;
    };
    for (const Arete& arete : table.aretes) {
        if (arete.nbFaces != 2 || classe[arete.faces[0]] != classe[arete.faces[1]]) {
            continue;
        }
        for (int s : {arete.a, arete.b}) {
            int c = position(arete.faces[0], s);
            int d = position(arete.faces[1], s);
            parent[racine(parent, c)] = racine(parent, d);
        }
    }
    resultat.numero.resize(nbCoins);
    for (int c = 0; c < nbCoins; c++) {
        resultat.numero[c] = racine(parent, c);
    }
    return resultat;
}

std::vector<float> profondeursALaBille(const Maillage& nappe, const TableAretes& table, float hauteur, float largeur,
                                       const Point& direction) {
    Point d = direction / direction.norm();
    Point u = OpenMesh::cross(d, std::fabs(d[0]) < 0.9f ? Point(1, 0, 0) : Point(0, 1, 0));
    u /= u.norm();
    Point v = OpenMesh::cross(d, u);

    struct Projete {
        float a, b, w;
    };
    auto projeter = [&](const Point& p) {
        return Projete{OpenMesh::dot(p, u), OpenMesh::dot(p, v), OpenMesh::dot(p, d)};
    };
    auto point = [&](int s) {
        return nappe.point(nappe.vertex_handle(s));
    };

    std::vector<bool> porte(table.nbSommets, false);
    for (const std::vector<int>& coins : table.coins) {
        for (int s : coins) {
            porte[s] = true;
        }
    }
    std::vector<Projete> points;
    for (int s = 0; s < table.nbSommets; s++) {
        if (porte[s]) {
            points.push_back(projeter(point(s)));
        }
    }
    const float petit = std::min(hauteur, largeur), grand = std::max(hauteur, largeur);
    const float pas = petit * std::sqrt(8.0f / 1000) * std::sqrt(petit / grand);
    for (const Arete& arete : table.aretes) {
        Point a = point(arete.a), b = point(arete.b);
        int n = (int)std::ceil((b - a).norm() / pas);
        for (int k = 1; k < n; k++) {
            points.push_back(projeter(a + (b - a) * ((float)k / n)));
        }
    }
    float aMin = std::numeric_limits<float>::max(), bMin = aMin, aMax = -aMin, bMax = -aMin;
    for (const Projete& q : points) {
        aMin = std::min(aMin, q.a);
        aMax = std::max(aMax, q.a);
        bMin = std::min(bMin, q.b);
        bMax = std::max(bMax, q.b);
    }

    double cote = largeur / 4.0;
    long nbA = (long)((aMax - aMin) / cote) + 1, nbB = (long)((bMax - bMin) / cote) + 1;
    while ((double)nbA * nbB > 5e7) {
        cote *= 2;
        nbA = (long)((aMax - aMin) / cote) + 1;
        nbB = (long)((bMax - bMin) / cote) + 1;
    }
    auto indices = [&](const Projete& q) {
        return std::make_pair(std::min((long)((q.a - aMin) / cote), nbA - 1), std::min((long)((q.b - bMin) / cote), nbB - 1));
    };
    std::vector<long> debut(nbA * nbB + 1, 0);
    for (const Projete& q : points) {
        auto [ia, ib] = indices(q);
        debut[ia * nbB + ib + 1]++;
    }
    for (long k = 0; k < nbA * nbB; k++) {
        debut[k + 1] += debut[k];
    }
    std::vector<Projete> rangee(points.size());
    std::vector<long> suivant(debut.begin(), debut.end() - 1);
    std::vector<float> plusBas(nbA * nbB, -std::numeric_limits<float>::max());
    for (const Projete& q : points) {
        auto [ia, ib] = indices(q);
        long k = ia * nbB + ib;
        rangee[suivant[k]++] = q;
        plusBas[k] = std::max(plusBas[k], q.w);
    }
    points.clear();
    points.shrink_to_fit();

    const double l2 = (double)largeur * largeur;
    const double rapport = (double)hauteur / largeur;
    const long portee = (long)std::ceil(largeur / cote);
    std::vector<float> profondeur(table.nbSommets, hauteur);
    for (int s = 0; s < table.nbSommets; s++) {
        if (!porte[s]) {
            continue;
        }
        const Projete p = projeter(point(s));
        auto [ia, ib] = indices(p);
        double meilleur = hauteur;
        for (long ja = std::max(0L, ia - portee); ja <= std::min(nbA - 1, ia + portee); ja++) {
            for (long jb = std::max(0L, ib - portee); jb <= std::min(nbB - 1, ib + portee); jb++) {
                long k = ja * nbB + jb;
                if (debut[k] == debut[k + 1]) {
                    continue;
                }
                double da = std::max({0.0, aMin + ja * cote - p.a, p.a - (aMin + (ja + 1) * cote)});
                double db = std::max({0.0, bMin + jb * cote - p.b, p.b - (bMin + (jb + 1) * cote)});
                double dd = da * da + db * db;
                if (dd > l2 || plusBas[k] - p.w + rapport * std::sqrt(l2 - dd) <= meilleur) {
                    continue;
                }
                for (long i = debut[k]; i < debut[k + 1]; i++) {
                    const Projete& q = rangee[i];
                    double r2 = (double)(q.a - p.a) * (q.a - p.a) + (double)(q.b - p.b) * (q.b - p.b);
                    if (r2 <= l2) {
                        meilleur = std::max(meilleur, q.w - p.w + rapport * std::sqrt(l2 - r2));
                    }
                }
            }
        }
        profondeur[s] = meilleur;
    }
    return profondeur;
}

}  // namespace

Maillage epaissir(const Maillage& nappe, const TableAretes& table, float hauteur, float largeurBille,
                  const Point& direction) {
    Maillage solide;
    bool copieAuDessus = OpenMesh::dot(normaleGlobale(nappe, table), direction) >= 0;
    const std::vector<float> profondeur = largeurBille > 0
                                              ? profondeursALaBille(nappe, table, hauteur, largeurBille, direction)
                                              : std::vector<float>(table.nbSommets, hauteur);

    std::vector<Sommet> origine(table.nbSommets);
    std::vector<Sommet> copie(table.nbSommets);
    for (const std::vector<int>& coins : table.coins) {
        for (int s : coins) {
            if (origine[s].is_valid()) {
                continue;
            }
            Point p = nappe.point(nappe.vertex_handle(s));
            origine[s] = solide.add_vertex(p);
            copie[s] = solide.add_vertex(p + direction * profondeur[s]);
        }
    }
    const std::vector<Sommet>& dessus = copieAuDessus ? copie : origine;
    const std::vector<Sommet>& dessous = copieAuDessus ? origine : copie;

    for (const std::vector<int>& coins : table.coins) {
        std::vector<Sommet> faceHaut;
        std::vector<Sommet> faceBas;
        for (int s : coins) {
            faceHaut.push_back(dessus[s]);
        }
        for (int i = coins.size() - 1; i >= 0; i--) {
            faceBas.push_back(dessous[coins[i]]);
        }
        ajouterFace(solide, faceHaut, "une face du dessus");
        ajouterFace(solide, faceBas, "une face du dessous");
    }

    for (const Arete& arete : table.aretes) {
        if (!arete.estBord()) {
            continue;
        }
        int a = arete.a;
        int b = arete.b;
        if (!faceParcourt(table.coins[arete.faces[0]], a, b)) {
            std::swap(a, b);
        }
        std::vector<Sommet> paroi = {dessus[b], dessus[a], dessous[a], dessous[b]};
        ajouterFace(solide, paroi, "un quad de bord");
    }
    return solide;
}

namespace {

Maillage construireAvecBande(const Maillage& nappe, const TableAretes& table, const std::vector<std::vector<int> >& coins,
                             const std::vector<bool>& dansBande, float hauteur, float socle) {
    Eventails eventails = eventailsParClasse(table, dansBande);

    Maillage solide;
    std::vector<Sommet> dessus(table.nbSommets);
    std::vector<Sommet> bas(eventails.numero.size());
    auto sommetDuDessus = [&](int s) {
        if (!dessus[s].is_valid()) {
            dessus[s] = solide.add_vertex(nappe.point(nappe.vertex_handle(s)));
        }
        return dessus[s];
    };
    auto sommetDuBas = [&](int face, int position) {
        int c = eventails.coin(face, position);
        if (!bas[c].is_valid()) {
            Point p = nappe.point(nappe.vertex_handle(coins[face][position]));
            bas[c] = solide.add_vertex(dansBande[face] ? auNiveau(p, socle) : p - HAUT * hauteur);
        }
        return bas[c];
    };
    auto positionDans = [&](int face, int sommet) {
        for (int k = 0; k < (int)coins[face].size(); k++) {
            if (coins[face][k] == sommet) {
                return k;
            }
        }
        throw std::logic_error("sommet absent de la face");
    };

    for (int f = 0; f < (int)coins.size(); f++) {
        std::vector<Sommet> faceHaut;
        std::vector<Sommet> faceBas;
        for (int s : coins[f]) {
            faceHaut.push_back(sommetDuDessus(s));
        }
        for (int i = coins[f].size() - 1; i >= 0; i--) {
            faceBas.push_back(sommetDuBas(f, i));
        }
        ajouterFace(solide, faceHaut, "une face du dessus");
        ajouterFace(solide, faceBas, dansBande[f] ? "une face du sol" : "une face du dessous");
    }

    for (const Arete& arete : table.aretes) {
        int a = arete.a;
        int b = arete.b;
        if (arete.estBord()) {
            int f = arete.faces[0];
            if (!faceParcourt(coins[f], a, b)) {
                std::swap(a, b);
            }
            std::vector<Sommet> mur = {dessus[b], dessus[a], sommetDuBas(f, positionDans(f, a)),
                                       sommetDuBas(f, positionDans(f, b))};
            ajouterFace(solide, mur, "un mur exterieur");
            continue;
        }
        if (arete.nbFaces != 2 || dansBande[arete.faces[0]] == dansBande[arete.faces[1]]) {
            continue;
        }
        int fBande = dansBande[arete.faces[0]] ? arete.faces[0] : arete.faces[1];
        int fCopie = dansBande[arete.faces[0]] ? arete.faces[1] : arete.faces[0];
        if (!faceParcourt(coins[fBande], a, b)) {
            std::swap(a, b);
        }
        std::vector<Sommet> mur = {sommetDuBas(fCopie, positionDans(fCopie, b)), sommetDuBas(fCopie, positionDans(fCopie, a)),
                                   sommetDuBas(fBande, positionDans(fBande, a)), sommetDuBas(fBande, positionDans(fBande, b))};
        ajouterFace(solide, mur, "un mur interieur");
    }

    TableAretes aretesSolide = construireAretes(solide);
    FiltreArete tout = [](const Arete&) { return true; };
    for (const std::vector<int>& fente : chainesDeBord(aretesSolide, tout)) {
        if (fente.size() < 3) {
            continue;
        }
        std::vector<Sommet> sommets;
        for (int s : fente) {
            sommets.push_back(solide.vertex_handle(s));
        }
        ajouterFace(solide, sommets, "un bouchon de pincement de la bande");
    }
    return solide;
}

}  // namespace

Maillage epaissirAvecPattes(const Maillage& nappe, const TableAretes& table, float hauteur, float socle) {
    std::vector<std::vector<int> > coins = coinsVersLeHaut(nappe, table);
    std::vector<float> distance = distanceAuBord(nappe, table, hauteur);

    std::vector<bool> dansBande(coins.size());
    for (int f = 0; f < (int)coins.size(); f++) {
        dansBande[f] = true;
        for (int s : coins[f]) {
            if (distance[s] > hauteur) {
                dansBande[f] = false;
            }
        }
    }
    return construireAvecBande(nappe, table, coins, dansBande, hauteur, socle);
}

Maillage remplirJusquAuSol(const Maillage& nappe, const TableAretes& table, float socle) {
    std::vector<std::vector<int> > coins = coinsVersLeHaut(nappe, table);
    std::vector<bool> dansBande(coins.size(), true);
    return construireAvecBande(nappe, table, coins, dansBande, 0, socle);
}

float zMinimum(const Maillage& maillage, const TableAretes& table) {
    float minimum = std::numeric_limits<float>::infinity();
    for (const std::vector<int>& coins : table.coins) {
        for (int s : coins) {
            minimum = std::min(minimum, maillage.point(maillage.vertex_handle(s))[2]);
        }
    }
    return minimum;
}

Maillage extruderFaces(const Maillage& maillage, const TableAretes& table, const std::vector<int>& numerosFaces,
                       float hauteur, const Point& direction) {
    std::set<int> selection(numerosFaces.begin(), numerosFaces.end());
    Maillage resultat;
    std::vector<Sommet> originaux(table.nbSommets);
    std::vector<Sommet> deplaces(table.nbSommets);

    for (int f = 0; f < (int)table.coins.size(); f++) {
        bool choisie = selection.count(f) > 0;
        std::vector<Sommet> sommets;
        for (int s : table.coins[f]) {
            if (choisie) {
                if (!deplaces[s].is_valid()) {
                    deplaces[s] = resultat.add_vertex(maillage.point(maillage.vertex_handle(s)) + direction * hauteur);
                }
                sommets.push_back(deplaces[s]);
            } else {
                sommets.push_back(sommetOuCreer(resultat, maillage, originaux, s));
            }
        }
        ajouterFace(resultat, sommets, "une face");
    }

    for (const Arete& arete : table.aretes) {
        int nbChoisies = 0;
        int faceChoisie = -1;
        for (int k = 0; k < arete.nbFaces && k < 2; k++) {
            if (selection.count(arete.faces[k]) > 0) {
                nbChoisies++;
                faceChoisie = arete.faces[k];
            }
        }
        if (nbChoisies != 1) {
            continue;
        }
        int a = arete.a;
        int b = arete.b;
        if (!faceParcourt(table.coins[faceChoisie], a, b)) {
            std::swap(a, b);
        }
        std::vector<Sommet> cote = {deplaces[b], deplaces[a],
                                    sommetOuCreer(resultat, maillage, originaux, a),
                                    sommetOuCreer(resultat, maillage, originaux, b)};
        ajouterFace(resultat, cote, "un quad de cote");
    }
    return resultat;
}

std::vector<int> facesDuDessus(const Maillage& maillage, const Point& haut) {
    std::vector<int> resultat;
    for (Face face : maillage.faces()) {
        std::vector<int> coins = sommetsDeLaFace(maillage, face.idx());
        if (coins.size() < 3) {
            continue;
        }
        Point a = maillage.point(maillage.vertex_handle(coins[0]));
        Point b = maillage.point(maillage.vertex_handle(coins[1]));
        Point c = maillage.point(maillage.vertex_handle(coins[2]));
        if (OpenMesh::dot(OpenMesh::cross(b - a, c - a), haut) > 0) {
            resultat.push_back(face.idx());
        }
    }
    return resultat;
}

std::string formaterPlages(const std::vector<int>& numeros) {
    std::string texte;
    int i = 0;
    while (i < (int)numeros.size()) {
        int j = i;
        while (j + 1 < (int)numeros.size() && numeros[j + 1] == numeros[j] + 1) {
            j++;
        }
        if (!texte.empty()) {
            texte += ", ";
        }
        texte += std::to_string(numeros[i]);
        if (j > i) {
            texte += "-" + std::to_string(numeros[j]);
        }
        i = j + 1;
    }
    return texte;
}
