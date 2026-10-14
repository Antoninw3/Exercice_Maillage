#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <string>
#include <vector>

namespace {

const long W = 6240, H = 4160;
const long WC = W - 1, HC = H - 1;
const double F = 2.143852502947797;
const double PX = 140.0 / 6239 * F, PY = 280.0 / 3 / 4159 * F;
const double RT = 0.8;
const double SIGMA = 2.5;
const long POUSSIERE = 1000, PETIT_TROU = 625;
const double INF = std::numeric_limits<double>::infinity();

struct Reglages {
    double socle = 3.0, bord = 0.5, emax = 3.0;
    bool adaptatif = true;
    double retrait = 1.0, angle = 45.0;
    double pente = 1.0;
};

typedef std::vector<uint8_t> Masque;
typedef std::vector<double> Champ;
typedef std::array<int32_t, 3> Triangle;

[[noreturn]] void arreter(const char* message) {
    fprintf(stderr, "%s\n", message);
    exit(1);
}

Masque carre3(const Masque& m, long h, long w, bool dilater) {
    Masque r(m.size());
    for (long i = 0; i < h; i++) {
        for (long j = 0; j < w; j++) {
            bool tous = true, un = false;
            for (long di = -1; di <= 1; di++) {
                for (long dj = -1; dj <= 1; dj++) {
                    long a = i + di, b = j + dj;
                    bool v = a >= 0 && a < h && b >= 0 && b < w && m[a * w + b];
                    tous = tous && v;
                    un = un || v;
                }
            }
            r[i * w + j] = dilater ? un : tous;
        }
    }
    return r;
}

std::vector<int32_t> morceaux(const Masque& m, long h, long w, uint8_t valeur, std::vector<long>& tailles) {
    std::vector<int32_t> num(m.size(), -1);
    std::vector<long> pile;
    tailles.clear();
    for (long depart = 0; depart < h * w; depart++) {
        if (m[depart] != valeur || num[depart] >= 0) {
            continue;
        }
        int32_t n = (int32_t)tailles.size();
        long taille = 0;
        num[depart] = n;
        pile.push_back(depart);
        while (!pile.empty()) {
            long p = pile.back();
            pile.pop_back();
            taille++;
            long i = p / w, j = p % w;
            const long voisins[4][2] = {{i - 1, j}, {i + 1, j}, {i, j - 1}, {i, j + 1}};
            for (const auto& v : voisins) {
                long q = v[0] * w + v[1];
                if (v[0] >= 0 && v[0] < h && v[1] >= 0 && v[1] < w && m[q] == valeur && num[q] < 0) {
                    num[q] = n;
                    pile.push_back(q);
                }
            }
        }
        tailles.push_back(taille);
    }
    return num;
}

void nettoyer(Masque& m) {
    std::vector<long> tailles;
    std::vector<int32_t> num = morceaux(m, HC, WC, 1, tailles);
    for (long p = 0; p < HC * WC; p++) {
        m[p] = m[p] && tailles[num[p]] >= POUSSIERE;
    }
    num = morceaux(m, HC, WC, 0, tailles);
    std::vector<bool> petit(tailles.size());
    for (size_t k = 0; k < tailles.size(); k++) {
        petit[k] = tailles[k] <= PETIT_TROU;
    }
    for (long i = 0; i < HC; i++) {
        for (long j = 0; j < WC; j++) {
            long p = i * WC + j;
            if ((i == 0 || i == HC - 1 || j == 0 || j == WC - 1) && num[p] >= 0) {
                petit[num[p]] = false;
            }
        }
    }
    for (long p = 0; p < HC * WC; p++) {
        if (num[p] >= 0 && petit[num[p]]) {
            m[p] = 1;
        }
    }
}

int leverPincements(Masque& m) {
    int tours = 0;
    std::vector<long> aRemplir;
    while (true) {
        aRemplir.clear();
        for (long i = 0; i + 1 < HC; i++) {
            for (long j = 0; j + 1 < WC; j++) {
                bool a = m[i * WC + j], b = m[i * WC + j + 1];
                bool c = m[(i + 1) * WC + j], d = m[(i + 1) * WC + j + 1];
                if (a && d && !b && !c) {
                    aRemplir.push_back(i * WC + j + 1);
                } else if (b && c && !a && !d) {
                    aRemplir.push_back(i * WC + j);
                }
            }
        }
        if (aRemplir.empty()) {
            return tours;
        }
        for (long p : aRemplir) {
            m[p] = 1;
        }
        tours++;
    }
}

Masque voisinage(const Masque& m) {
    Masque nb(W * H, 0);
    for (long i = 0; i < HC; i++) {
        for (long j = 0; j < WC; j++) {
            if (m[i * WC + j]) {
                nb[i * W + j]++;
                nb[i * W + j + 1]++;
                nb[(i + 1) * W + j]++;
                nb[(i + 1) * W + j + 1]++;
            }
        }
    }
    return nb;
}

std::vector<double> noyauGaussien(double sigma, int rayon) {
    std::vector<double> phi(2 * rayon + 1);
    for (int k = -rayon; k <= rayon; k++) {
        phi[k + rayon] = std::exp(-0.5 / (sigma * sigma) * (double)(k * k));
    }
    double somme;
    size_t n = phi.size();
    if (n < 8) {
        somme = 0;
        for (double v : phi) somme += v;
    } else {
        double r[8];
        for (int k = 0; k < 8; k++) r[k] = phi[k];
        size_t fin = n - n % 8;
        for (size_t i = 8; i < fin; i += 8) {
            for (int k = 0; k < 8; k++) r[k] += phi[i + k];
        }
        somme = ((r[0] + r[1]) + (r[2] + r[3])) + ((r[4] + r[5]) + (r[6] + r[7]));
        for (size_t i = fin; i < n; i++) somme += phi[i];
    }
    for (double& v : phi) v /= somme;
    return phi;
}

void flouLigne(double* debut, long n, long pas, const std::vector<double>& phi, std::vector<double>& tampon) {
    int r = (int)phi.size() / 2;
    tampon.resize(n + 2 * r);
    for (long k = 0; k < n; k++) tampon[r + k] = debut[k * pas];
    for (int k = 1; k <= r; k++) {
        tampon[r - k] = tampon[r + k - 1];
        tampon[r + n - 1 + k] = tampon[r + n - k];
    }
    for (long k = 0; k < n; k++) {
        const double* c = &tampon[r + k];
        double s = c[0] * phi[r];
        for (int d = r; d >= 1; d--) s += (c[-d] + c[d]) * phi[r - d];
        debut[k * pas] = s;
    }
}

void flouGaussien(Champ& g, double sigma) {
    std::vector<double> phi = noyauGaussien(sigma, (int)(4.0 * sigma + 0.5));
    std::vector<double> tampon;
    for (long j = 0; j < W; j++) flouLigne(&g[j], H, W, phi, tampon);
    for (long i = 0; i < H; i++) flouLigne(&g[i * W], W, 1, phi, tampon);
}

struct Voronoi {
    std::vector<int32_t> f0, f1;
    std::vector<long> g;
};

void voronoiLigne(int32_t* r0, int32_t* r1, long len, long pas, int d, long autre, const double s[2], Voronoi& v) {
    v.f0.resize(len);
    v.f1.resize(len);
    v.g.resize(len);
    for (long i = 0; i < len; i++) {
        v.f0[i] = r0[i * pas];
        v.f1[i] = r1[i * pas];
    }
    const int32_t* fd = d == 0 ? v.f0.data() : v.f1.data();
    const int32_t* fa = d == 0 ? v.f1.data() : v.f0.data();
    double sd = s[d], sa = s[1 - d];
    long l = -1;
    for (long i = 0; i < len; i++) {
        if (v.f0[i] < 0) continue;
        double tw = (fa[i] - autre) * sa, wR = tw * tw;
        while (l >= 1) {
            long i1 = v.g[l], i2 = v.g[l - 1];
            double a = (double)(fd[i1] - fd[i2]) * sd, b = (double)(fd[i] - fd[i1]) * sd, c = a + b;
            double tu = (fa[i2] - autre) * sa, tv = (fa[i1] - autre) * sa;
            double uR = tu * tu, vR = tv * tv;
            if (c * vR - b * uR - a * wR - a * b * c <= 0.0) break;
            --l;
        }
        v.g[++l] = i;
    }
    if (l < 0) return;
    long maxl = l;
    l = 0;
    auto carre = [&](long k, long i) {
        double t1 = (double)(fd[k] - i) * sd, t2 = (double)(fa[k] - autre) * sa;
        return d == 0 ? t1 * t1 + t2 * t2 : t2 * t2 + t1 * t1;
    };
    for (long i = 0; i < len; i++) {
        double d1 = carre(v.g[l], i);
        while (l < maxl) {
            double d2 = carre(v.g[l + 1], i);
            if (d1 <= d2) break;
            d1 = d2;
            ++l;
        }
        r0[i * pas] = v.f0[v.g[l]];
        r1[i * pas] = v.f1[v.g[l]];
    }
}

void plusProcheDehors(const Masque& dedans, double sy, double sx, std::vector<int32_t>& ir, std::vector<int32_t>& ic) {
    const double s[2] = {sy, sx};
    ir.assign(W * H, -1);
    ic.assign(W * H, -1);
    for (long p = 0; p < W * H; p++) {
        if (!dedans[p]) {
            ir[p] = (int32_t)(p / W);
            ic[p] = (int32_t)(p % W);
        }
    }
    Voronoi v;
    for (long j = 0; j < W; j++) voronoiLigne(&ir[j], &ic[j], H, W, 0, j, s, v);
    for (long i = 0; i < H; i++) voronoiLigne(&ir[i * W], &ic[i * W], W, 1, 1, i, s, v);
}

Champ reliefFloute(const std::vector<float>& za, const Masque& t, const Masque& utilise) {
    Masque porte(W * H, 0);
    for (long i = 0; i < HC; i++) {
        for (long j = 0; j < WC; j++) {
            if (t[i * WC + j]) {
                porte[i * W + j] = porte[i * W + j + 1] = porte[(i + 1) * W + j] = porte[(i + 1) * W + j + 1] = 1;
            }
        }
    }
    Champ num(W * H), den(W * H);
    for (long p = 0; p < W * H; p++) {
        num[p] = porte[p] ? (double)za[p] : 0.0;
        den[p] = porte[p];
    }
    flouGaussien(num, SIGMA);
    flouGaussien(den, SIGMA);
    Masque loin(W * H);
    bool aCombler = false;
    for (long p = 0; p < W * H; p++) {
        loin[p] = !(den[p] > 1e-3);
        num[p] = loin[p] ? 0.0 : num[p] / std::max(den[p], 1e-9);
        aCombler = aCombler || (loin[p] && utilise[p]);
    }
    if (aCombler) {
        std::vector<int32_t> ir, ic;
        plusProcheDehors(loin, 1.0, 1.0, ir, ic);
        for (long p = 0; p < W * H; p++) den[p] = num[(long)ir[p] * W + ic[p]];
        num.swap(den);
    }
    for (double& z : num) z *= F;
    return num;
}

Champ retraitsContour(const Champ& ztop, const Masque& nb, double z0, const Reglages& r) {
    long rmax = (long)(r.emax / PY) + 1, cmax = (long)(r.emax / PX) + 1;
    long lw = 2 * cmax + 1;
    std::vector<double> dd((2 * rmax + 1) * lw);
    for (long di = -rmax; di <= rmax; di++) {
        for (long dj = -cmax; dj <= cmax; dj++) {
            double d = std::hypot(di * PY, dj * PX);
            dd[(di + rmax) * lw + dj + cmax] = d > r.emax ? INF : d;
        }
    }
    Champ marge(W * H), retrait(W * H, 0.0);
    for (long p = 0; p < W * H; p++) marge[p] = nb[p] ? ztop[p] - z0 - r.bord : INF;
    for (long i = 0; i < H; i++) {
        for (long j = 0; j < W; j++) {
            if (nb[i * W + j] == 0 || nb[i * W + j] == 4) continue;
            double e = r.emax;
            long j0 = std::max(j - cmax, 0L), n = std::min(j + cmax, W - 1) - j0 + 1;
            for (long a = std::max(i - rmax, 0L); a <= std::min(i + rmax, H - 1); a++) {
                const double* ma = &marge[a * W + j0];
                const double* da = &dd[(a - i + rmax) * lw + cmax + j0 - j];
                for (long k = 0; k < n; k++) e = std::min(e, ma[k] + da[k]);
            }
            retrait[i * W + j] = std::max(e, 0.0);
        }
    }
    return retrait;
}

struct Profils {
    Champ ztop, zbot;
    long serre = 0;
    std::vector<double> retraitsBord;
};

Profils profils(Champ& zt, const Masque& nb, double z0, const Reglages& r) {
    Masque interieur(W * H);
    for (long p = 0; p < W * H; p++) interieur[p] = nb[p] == 4;
    std::vector<int32_t> ir, ic;
    plusProcheDehors(interieur, PY, PX, ir, ic);
    interieur.clear();
    interieur.shrink_to_fit();
    Profils pr;
    pr.ztop.swap(zt);
    Champ dist(W * H);
    for (long p = 0; p < W * H; p++) {
        double ty = (double)(ir[p] - p / W) * PY, tx = (double)(ic[p] - p % W) * PX;
        dist[p] = std::sqrt(ty * ty + tx * tx);
        double dr = std::min(dist[p], RT);
        pr.ztop[p] = pr.ztop[p] - RT + std::sqrt(std::max(0.0, RT * RT - (RT - dr) * (RT - dr)));
    }
    Champ retraitBord;
    if (r.adaptatif) retraitBord = retraitsContour(pr.ztop, nb, z0, r);
    pr.zbot.resize(W * H);
    for (long p = 0; p < W * H; p++) {
        double retrait = r.retrait;
        if (r.adaptatif) {
            retrait = retraitBord[(long)ir[p] * W + ic[p]];
            if (nb[p] > 0 && nb[p] < 4) pr.retraitsBord.push_back(retrait);
        }
        double zb = z0 + std::max(0.0, retrait - dist[p]) * r.pente;
        double haut = pr.ztop[p] - r.bord;
        if (nb[p] > 0 && zb > haut + 1e-6) pr.serre++;
        pr.zbot[p] = std::min(zb, haut);
    }
    return pr;
}

std::vector<double> quantiles(std::vector<double> v, const std::vector<double>& qs) {
    std::sort(v.begin(), v.end());
    std::vector<double> res;
    for (double q : qs) {
        double x = q * (double)(v.size() - 1);
        size_t k = (size_t)x;
        double t = x - (double)k;
        res.push_back(k + 1 < v.size() ? v[k] + (v[k + 1] - v[k]) * t : v[k]);
    }
    return res;
}

std::vector<Triangle> triangles(const Masque& m, const std::vector<int32_t>& numero, int32_t nv, long& nmurs) {
    std::vector<long> cases;
    for (long p = 0; p < HC * WC; p++) {
        if (m[p]) cases.push_back(p);
    }
    auto coins = [&](long c, int32_t k[4]) {
        long i = c / WC, j = c % WC;
        k[0] = numero[i * W + j];
        k[1] = numero[i * W + j + 1];
        k[2] = numero[(i + 1) * W + j];
        k[3] = numero[(i + 1) * W + j + 1];
    };
    std::vector<Triangle> T;
    T.reserve(4 * cases.size());
    const int faces[2][2][3] = {{{0, 3, 1}, {0, 2, 3}}, {{0, 1, 3}, {0, 3, 2}}};
    for (int cote = 0; cote < 2; cote++) {
        int32_t decalage = cote * nv;
        for (const auto& tri : faces[cote]) {
            for (long c : cases) {
                int32_t k[4];
                coins(c, k);
                T.push_back({k[tri[0]] + decalage, k[tri[1]] + decalage, k[tri[2]] + decalage});
            }
        }
    }
    const int murs[4][4] = {{0, 2, 0, -1}, {2, 3, 1, 0}, {3, 1, 0, 1}, {1, 0, -1, 0}};
    size_t avant = T.size();
    for (const auto& mur : murs) {
        for (int moitie = 0; moitie < 2; moitie++) {
            for (long c : cases) {
                long a = c / WC + mur[2], b = c % WC + mur[3];
                if (a >= 0 && a < HC && b >= 0 && b < WC && m[a * WC + b]) continue;
                int32_t k[4];
                coins(c, k);
                int32_t u = k[mur[0]], v = k[mur[1]];
                T.push_back(moitie == 0 ? Triangle{v, u, u + nv} : Triangle{v, u + nv, v + nv});
            }
        }
    }
    nmurs = (long)(T.size() - avant);
    return T;
}

void controlerFermeture(const std::vector<Triangle>& T, int32_t nv) {
    const int64_t n2 = 2 * (int64_t)nv;
    std::vector<int64_t> cle;
    cle.reserve(3 * T.size());
    for (const Triangle& t : T) {
        for (int k = 0; k < 3; k++) {
            int64_t a = t[k], b = t[(k + 1) % 3];
            cle.push_back((std::min(a, b) * n2 + std::max(a, b)) * 2 + (a < b));
        }
    }
    std::sort(cle.begin(), cle.end());
    long doubles = 0, orphelines = 0;
    for (size_t k = 0; k < cle.size();) {
        long sens[2] = {0, 0};
        size_t fin = k;
        for (; fin < cle.size() && cle[fin] / 2 == cle[k] / 2; fin++) sens[cle[fin] % 2]++;
        doubles += std::max(sens[0] - 1, 0L) + std::max(sens[1] - 1, 0L);
        orphelines += sens[0] == 0 ? sens[1] : sens[1] == 0 ? sens[0] : 0;
        k = fin;
    }
    printf("aretes orientees en double : %ld, sans inverse : %ld\n", doubles, orphelines);
    if (doubles || orphelines) arreter("Erreur : maillage non ferme");
}

double ecrireStl(const char* chemin, const std::vector<Triangle>& T, const std::vector<std::array<float, 3>>& P) {
    FILE* f = fopen(chemin, "wb");
    if (!f) arreter("Erreur : impossible d'ecrire la sortie");
    char entete[80];
    memset(entete, ' ', sizeof entete);
    memcpy(entete, "soleil flottant 10x20 cm", 24);
    uint32_t n = (uint32_t)T.size();
    bool ok = fwrite(entete, 1, 80, f) == 80 && fwrite(&n, 4, 1, f) == 1;
    double vol = 0;
    std::vector<char> bloc;
    for (size_t d = 0; d < T.size() && ok; d += 1 << 21) {
        size_t fin = std::min(T.size(), d + (1 << 21));
        bloc.assign((fin - d) * 50, 0);
        for (size_t k = d; k < fin; k++) {
            const auto &a = P[T[k][0]], &b = P[T[k][1]], &c = P[T[k][2]];
            float u[3] = {b[0] - a[0], b[1] - a[1], b[2] - a[2]};
            float v[3] = {c[0] - a[0], c[1] - a[1], c[2] - a[2]};
            float nrm[3] = {u[1] * v[2] - u[2] * v[1], u[2] * v[0] - u[0] * v[2], u[0] * v[1] - u[1] * v[0]};
            float l = std::max(std::sqrt(nrm[0] * nrm[0] + nrm[1] * nrm[1] + nrm[2] * nrm[2]), 1e-12f);
            for (float& x : nrm) x /= l;
            float rec[12] = {nrm[0], nrm[1], nrm[2], a[0], a[1], a[2], b[0], b[1], b[2], c[0], c[1], c[2]};
            memcpy(&bloc[(k - d) * 50], rec, 48);
            double bc[3] = {(double)b[1] * c[2] - (double)b[2] * c[1], (double)b[2] * c[0] - (double)b[0] * c[2],
                            (double)b[0] * c[1] - (double)b[1] * c[0]};
            vol += (a[0] * bc[0] + a[1] * bc[1] + a[2] * bc[2]) / 6;
        }
        ok = fwrite(bloc.data(), 1, bloc.size(), f) == bloc.size();
    }
    if (fclose(f) != 0 || !ok) arreter("Erreur : ecriture de la sortie");
    return vol;
}

template <class T>
std::vector<T> lire(const char* chemin, size_t n) {
    std::vector<T> v(n);
    FILE* f = fopen(chemin, "rb");
    if (!f || fread(v.data(), sizeof(T), n, f) != n || fgetc(f) != EOF) {
        fprintf(stderr, "Erreur : '%s' absent ou pas de la taille de la grille\n", chemin);
        exit(1);
    }
    fclose(f);
    return v;
}

double nombre(const char* texte) {
    char* fin;
    double v = strtod(texte, &fin);
    if (fin == texte || *fin != '\0' || !std::isfinite(v)) {
        fprintf(stderr, "Erreur : '%s' n'est pas un nombre\n", texte);
        exit(1);
    }
    return v;
}

Reglages lireReglages(int argc, char** argv) {
    const char* usage = "Usage : flottant <z.f32> <tri.u8> <sortie.stl> [socle bord emax|fixe [retrait angle]]";
    if ((argc != 4 && argc != 7 && argc != 9) || (argc == 9 && strcmp(argv[6], "fixe") != 0)) arreter(usage);
    Reglages r;
    if (argc >= 7) {
        r.socle = nombre(argv[4]);
        r.bord = nombre(argv[5]);
        r.adaptatif = strcmp(argv[6], "fixe") != 0;
        if (r.adaptatif) r.emax = nombre(argv[6]);
        if (r.adaptatif && !(r.emax > 0)) arreter("Erreur : retrait maximal emax > 0");
    }
    if (argc == 9) {
        r.retrait = nombre(argv[7]);
        r.angle = nombre(argv[8]);
        if (!(r.retrait > 0) || !(r.angle >= 45 && r.angle < 90))
            arreter("Erreur : retrait > 0 et angle entre 45 (autoportant) et 90 exclu");
    }
    r.pente = r.adaptatif ? 1.0 : std::tan(r.angle * (M_PI / 180.0));
    if (r.adaptatif) {
        printf("socle %g, bord %g, biseau adaptatif max %.1f a 45 deg\n", r.socle, r.bord, r.emax);
    } else {
        printf("socle %g, bord %g, biseau fixe : retrait %.2f a %g deg, hauteur %.2f\n", r.socle, r.bord, r.retrait,
               r.angle, r.retrait * r.pente);
    }
    return r;
}

Masque masqueDesCases(const Masque& t) {
    Masque m = carre3(carre3(t, HC, WC, true), HC, WC, false);
    m = carre3(carre3(m, HC, WC, false), HC, WC, true);
    nettoyer(m);
    int tours = leverPincements(m);
    long n = 0;
    for (uint8_t c : m) n += c;
    printf("cases %ld, pincements leves en %d tour(s)\n", n, tours);
    if (n == 0) arreter("Erreur : aucune case pleine");
    return m;
}

void imprimerProfils(const Profils& pr, const Masque& nb, double z0, const Reglages& r) {
    if (r.adaptatif) {
        std::vector<double> q = quantiles(pr.retraitsBord, {.5, .1, .9, 1});
        printf("retrait au bord (mm) : mediane %.2f, 10 %% %.2f, 90 %% %.2f, max %.2f\n", q[0], q[1], q[2], q[3]);
    }
    printf("points ou le dessous a du suivre le dessus : %ld\n", pr.serre);
    std::vector<double> hb;
    double ecartMin = INF, zmin = INF, zmax = -INF;
    for (long p = 0; p < W * H; p++) {
        if (nb[p] == 0) continue;
        double e = pr.ztop[p] - pr.zbot[p];
        ecartMin = std::min(ecartMin, e);
        zmin = std::min(zmin, pr.ztop[p]);
        zmax = std::max(zmax, pr.ztop[p]);
        if (nb[p] < 4) hb.push_back(e);
    }
    std::vector<double> q = quantiles(hb, {.5, .9, .99, 1});
    printf("bord vertical (mm) : mediane %.2f, 90 %% %.2f, 99 %% %.2f, max %.2f\n", q[0], q[1], q[2], q[3]);
    printf("z0 %.3f, dessus %.3f..%.3f, epaisseur mini %.3f mm\n", z0, zmin, zmax, ecartMin);
}

}  // namespace

int main(int argc, char** argv) {
    Reglages r = lireReglages(argc, argv);
    std::vector<float> za = lire<float>(argv[1], W * H);
    Masque t = lire<uint8_t>(argv[2], WC * HC);
    for (uint8_t& c : t) c = c > 0;

    Masque m = masqueDesCases(t);
    Masque nb = voisinage(m);
    Champ zt = reliefFloute(za, t, nb);
    za = std::vector<float>();
    t = Masque();
    double z0 = INF;
    for (long p = 0; p < W * H; p++) {
        if (nb[p]) z0 = std::min(z0, zt[p]);
    }
    z0 -= r.socle;
    Profils pr = profils(zt, nb, z0, r);
    imprimerProfils(pr, nb, z0, r);

    std::vector<int32_t> numero(W * H, -1);
    int32_t nv = 0;
    for (long p = 0; p < W * H; p++) {
        if (nb[p]) numero[p] = nv++;
    }
    std::vector<std::array<float, 3>> P(2 * (size_t)nv);
    double boite[3][2] = {{INF, -INF}, {INF, -INF}, {INF, -INF}};
    for (long p = 0; p < W * H; p++) {
        if (numero[p] < 0) continue;
        double x = (double)(p % W) * PX, y = 280.0 / 3 * F - (double)(p / W) * PY;
        double z[2] = {pr.ztop[p], pr.zbot[p]};
        for (int k = 0; k < 2; k++) {
            P[numero[p] + k * nv] = {(float)x, (float)y, (float)z[k]};
            const double c[3] = {x, y, z[k]};
            for (int a = 0; a < 3; a++) {
                boite[a][0] = std::min(boite[a][0], c[a]);
                boite[a][1] = std::max(boite[a][1], c[a]);
            }
        }
    }
    pr = Profils();
    nb = Masque();

    long nmurs = 0;
    std::vector<Triangle> T = triangles(m, numero, nv, nmurs);
    m = Masque();
    numero = std::vector<int32_t>();
    printf("%d sommets par face, %zu triangles dont %ld de murs\n", nv, T.size(), nmurs);
    controlerFermeture(T, nv);

    double vol = ecrireStl(argv[3], T, P);
    printf("volume %.3f cm3, boite %.2f x %.2f x %.2f mm\n", vol / 1000, boite[0][1] - boite[0][0],
           boite[1][1] - boite[1][0], boite[2][1] - boite[2][0]);
    printf("ecrit %s\n", argv[3]);
    return 0;
}
