#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

int main(int argc, char** argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage : %s <3d-mesh.obj> <dossier>\n", argv[0]);
        return 1;
    }
    const long W = 6240, H = 4160;
    int fd = open(argv[1], O_RDONLY);
    struct stat st;
    if (fd < 0 || fstat(fd, &st) != 0) {
        fprintf(stderr, "Erreur : impossible de lire '%s'\n", argv[1]);
        return 2;
    }
    char* d = (char*)mmap(nullptr, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (d == MAP_FAILED) {
        fprintf(stderr, "Erreur : mmap de '%s'\n", argv[1]);
        return 2;
    }
    char* e = d + st.st_size;
    std::vector<float> z(W * H, 0);
    std::vector<unsigned char> tri((W - 1) * (H - 1), 0);
    size_t nv = 0, nf = 0, horsGrille = 0;
    for (char* p = d; p < e;) {
        char* fin = (char*)memchr(p, '\n', e - p);
        if (!fin) fin = e;
        if (p[0] == 'v' && p[1] == ' ') {
            if (nv == z.size()) {
                fprintf(stderr, "Erreur : plus de %ld sommets, ce n'est pas la grille du soleil\n", W * H);
                return 3;
            }
            char* q = p + 2;
            strtod(q, &q);
            strtod(q, &q);
            z[nv++] = strtod(q, &q);
        } else if (p[0] == 'f' && p[1] == ' ') {
            char* q = p + 2;
            long ligne = H, colonne = W;
            for (int k = 0; k < 3; k++) {
                long i = strtol(q, &q, 10) - 1;
                while (q < fin && *q != ' ') q++;
                if (i / W < ligne) ligne = i / W;
                if (i % W < colonne) colonne = i % W;
            }
            if (ligne < H - 1 && colonne < W - 1) {
                tri[ligne * (W - 1) + colonne]++;
            } else {
                horsGrille++;
            }
            nf++;
        }
        p = fin + 1;
    }
    if (nv != z.size()) {
        fprintf(stderr, "Erreur : %zu sommets au lieu de %ld x %ld\n", nv, W, H);
        return 3;
    }
    fprintf(stderr, "%zu sommets, %zu faces, %zu hors grille\n", nv, nf, horsGrille);
    std::string dossier = argv[2];
    FILE* o = fopen((dossier + "/z.f32").c_str(), "wb");
    FILE* t = fopen((dossier + "/tri.u8").c_str(), "wb");
    if (!o || !t || fwrite(z.data(), 4, z.size(), o) != z.size() || fwrite(tri.data(), 1, tri.size(), t) != tri.size()) {
        fprintf(stderr, "Erreur : ecriture dans '%s'\n", argv[2]);
        return 4;
    }
    fclose(o);
    fclose(t);
    return 0;
}
