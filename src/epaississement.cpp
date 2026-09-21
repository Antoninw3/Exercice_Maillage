#include "epaississement.hpp"

#include <set>
#include <stdexcept>

#include "bords.hpp"

void ajouterFace(Maillage& maillage, const std::vector<Sommet>& sommets, const std::string& quoi) {
    if (!maillage.add_face(sommets).is_valid()) {
        throw std::runtime_error("impossible d'ajouter " + quoi);
    }
}

Maillage epaissir(const Maillage& source, float hauteur, const Point& direction) {
    Maillage solide;
    int nb = source.n_vertices();
    std::vector<Sommet> dessus(nb);
    std::vector<Sommet> dessous(nb);

    for (Sommet s : source.vertices()) {
        Point pointHaut = source.point(s) + direction * hauteur;
        dessus[s.idx()] = solide.add_vertex(pointHaut);
    }
    for (Sommet s : source.vertices()) {
        dessous[s.idx()] = solide.add_vertex(source.point(s));
    }

    for (Face face : source.faces()) {
        std::vector<int> sommets = sommetsDeLaFace(source, face.idx());

        std::vector<Sommet> faceHaut;
        std::vector<Sommet> faceBas;
        for (int i = 0; i < (int)sommets.size(); i++) {
            faceHaut.push_back(dessus[sommets[i]]);
        }
        for (int i = sommets.size() - 1; i >= 0; i--) {
            faceBas.push_back(dessous[sommets[i]]);
        }

        ajouterFace(solide, faceHaut, "une face du dessus");
        ajouterFace(solide, faceBas, "une face du dessous");
    }

    TableAretes aretes = construireAretes(source);
    for (TableAretes::iterator it = aretes.begin(); it != aretes.end(); ++it) {
        if (it->second.size() != 1) {
            continue;
        }

        int a = it->first.first;
        int b = it->first.second;
        if (!faceParcourt(sommetsDeLaFace(source, it->second[0]), a, b)) {
            std::swap(a, b);
        }

        std::vector<Sommet> cote;
        cote.push_back(dessus[b]);
        cote.push_back(dessus[a]);
        cote.push_back(dessous[a]);
        cote.push_back(dessous[b]);
        ajouterFace(solide, cote, "un quad de bord");
    }

    return solide;
}

Maillage extruderFaces(const Maillage& maillage, const std::vector<int>& numerosFaces, float hauteur, const Point& direction) {
    std::set<int> selection(numerosFaces.begin(), numerosFaces.end());
    int nb = maillage.n_vertices();

    Maillage resultat;
    std::vector<Sommet> originaux(nb);
    std::vector<Sommet> deplaces(nb);

    for (Face face : maillage.faces()) {
        bool choisie = selection.count(face.idx()) > 0;

        std::vector<Sommet> sommets;
        for (Sommet s : maillage.fv_range(face)) {
            int i = s.idx();
            if (choisie) {
                if (!deplaces[i].is_valid()) {
                    deplaces[i] = resultat.add_vertex(maillage.point(s) + direction * hauteur);
                }
                sommets.push_back(deplaces[i]);
            } else {
                if (!originaux[i].is_valid()) {
                    originaux[i] = resultat.add_vertex(maillage.point(s));
                }
                sommets.push_back(originaux[i]);
            }
        }
        ajouterFace(resultat, sommets, "une face");
    }

    TableAretes aretes = construireAretes(maillage);
    for (TableAretes::iterator it = aretes.begin(); it != aretes.end(); ++it) {
        int nbChoisies = 0;
        int faceChoisie = -1;
        for (int k = 0; k < (int)it->second.size(); k++) {
            if (selection.count(it->second[k]) > 0) {
                nbChoisies++;
                faceChoisie = it->second[k];
            }
        }
        if (nbChoisies != 1) {
            continue;
        }

        int a = it->first.first;
        int b = it->first.second;
        if (!faceParcourt(sommetsDeLaFace(maillage, faceChoisie), a, b)) {
            std::swap(a, b);
        }
        if (!originaux[a].is_valid()) {
            originaux[a] = resultat.add_vertex(maillage.point(maillage.vertex_handle(a)));
        }
        if (!originaux[b].is_valid()) {
            originaux[b] = resultat.add_vertex(maillage.point(maillage.vertex_handle(b)));
        }

        std::vector<Sommet> cote;
        cote.push_back(deplaces[b]);
        cote.push_back(deplaces[a]);
        cote.push_back(originaux[a]);
        cote.push_back(originaux[b]);
        ajouterFace(resultat, cote, "un quad de cote");
    }

    return resultat;
}

std::vector<int> facesDuDessus(const Maillage& maillage, const Point& haut) {
    Maillage copie = maillage;
    copie.request_face_normals();
    copie.update_face_normals();

    std::vector<int> resultat;
    for (Face face : copie.faces()) {
        if (OpenMesh::dot(copie.normal(face), haut) > 0) {
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
