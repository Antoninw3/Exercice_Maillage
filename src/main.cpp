#include <iostream>
#include <string>
#include <vector>

#include "bords.hpp"
#include "decoupeur.hpp"
#include "epaississement.hpp"
#include "maillage.hpp"
#include "reparateur.hpp"

const Point HAUT(0, 0, 1);

void afficherUsage(const std::string& programme) {
    std::cerr << "Usage : " << programme << " <fichier.obj> [commande]" << std::endl
              << std::endl
              << "Sans commande : analyse le maillage (points, faces, aretes, bords)" << std::endl
              << std::endl
              << "extruder <sortie.obj|sortie.stl> <hauteur> [face|all]" << std::endl
              << "  hauteur seule : liste des faces du dessus, puis choix au clavier" << std::endl
              << "  face          : cette face monte, reliee a ses voisines" << std::endl
              << "  all           : surface ouverte -> dalle fermee ; solide ferme -> tout le dessus monte" << std::endl
              << std::endl
              << "decouper <prefixe>" << std::endl
              << "  coupe le maillage en 4 quarts puis les rebouche, et ecrit :" << std::endl
              << "  <prefixe>_quart1_decoupe.obj  (apres decoupe)" << std::endl
              << "  <prefixe>_quart1_referme.obj  (apres rebouchage)" << std::endl
              << "  <prefixe>_quart1_referme.stl  (apres rebouchage, en STL)" << std::endl;
}

void afficherSolide(const Maillage& solide, const std::string& titre) {
    TableAretes aretes = construireAretes(solide);
    std::string etat = "NON ferme";
    if (analyserBords(aretes).estFerme()) {
        etat = "ferme";
    }
    std::cout << titre << " : " << solide.n_vertices() << " points, " << solide.n_faces() << " faces, "
              << aretes.size() << " aretes, " << etat << std::endl;
}

bool finitPar(const std::string& texte, const std::string& fin) {
    if (texte.size() < fin.size()) {
        return false;
    }
    return texte.compare(texte.size() - fin.size(), fin.size(), fin) == 0;
}

bool ecrireFichier(Maillage maillage, const std::string& chemin) {
    bool ok = false;
    if (finitPar(chemin, ".stl") || finitPar(chemin, ".STL")) {
        maillage.triangulate();
        ok = OpenMesh::IO::write_mesh(maillage, chemin, OpenMesh::IO::Options::Binary);
    } else if (finitPar(chemin, ".obj") || finitPar(chemin, ".OBJ")) {
        ok = OpenMesh::IO::write_mesh(maillage, chemin);
    } else {
        std::cerr << "Erreur : extension de sortie inconnue, utiliser .stl ou .obj" << std::endl;
        return false;
    }

    if (!ok) {
        std::cerr << "Erreur : impossible d'ecrire '" << chemin << "'" << std::endl;
        return false;
    }
    std::cout << "Fichier ecrit : " << chemin << std::endl;
    return true;
}

bool estDansLaListe(const std::vector<int>& liste, int valeur) {
    for (int i = 0; i < (int)liste.size(); i++) {
        if (liste[i] == valeur) {
            return true;
        }
    }
    return false;
}

int choisirFaceDuDessus(const std::vector<int>& candidates, int demandee) {
    if (candidates.empty()) {
        throw std::runtime_error("aucune face du dessus dans ce maillage (essayer un autre axe du haut)");
    }
    std::cout << candidates.size() << " face(s) du dessus possibles : " << formaterPlages(candidates) << std::endl;

    int face = demandee;
    while (!estDansLaListe(candidates, face)) {
        if (face >= 0) {
            std::cout << "La face " << face << " n'est pas sur le dessus." << std::endl;
        }
        std::cout << "Numero de la face ou ajouter de la matiere : " << std::flush;
        if (!(std::cin >> face)) {
            throw std::runtime_error("aucune face choisie");
        }
    }
    return face;
}

int commandeExtruder(Maillage maillage, int argc, char** argv) {
    if (argc < 5) {
        afficherUsage(argv[0]);
        return 1;
    }
    std::string sortie = argv[3];
    float hauteur = std::stof(argv[4]);

    if (argc == 6 && std::string(argv[5]) == "all") {
        if (analyserBords(construireAretes(maillage)).estFerme()) {
            std::vector<int> dessus = facesDuDessus(maillage, HAUT);
            maillage = extruderFaces(maillage, dessus, hauteur, HAUT);
            afficherSolide(maillage, "Dessus extrude (" + std::to_string(dessus.size()) + " faces)");
        } else {
            maillage = epaissir(maillage, hauteur, HAUT);
            afficherSolide(maillage, "Solide");
        }
    } else {
        int demandee = -1;
        if (argc == 6) {
            demandee = std::stoi(argv[5]);
        }
        int face = choisirFaceDuDessus(facesDuDessus(maillage, HAUT), demandee);
        std::vector<int> selection;
        selection.push_back(face);
        maillage = extruderFaces(maillage, selection, hauteur, HAUT);
        afficherSolide(maillage, "Face " + std::to_string(face) + " extrudee");
    }

    if (!ecrireFichier(maillage, sortie)) {
        return 3;
    }
    return 0;
}

std::string sansExtension(const std::string& chemin) {
    size_t point = chemin.rfind('.');
    size_t barre = chemin.rfind('/');
    if (point == std::string::npos || (barre != std::string::npos && point < barre)) {
        return chemin;
    }
    return chemin.substr(0, point);
}

int commandeDecouper(const Maillage& maillage, int argc, char** argv) {
    if (argc != 4) {
        afficherUsage(argv[0]);
        return 1;
    }
    std::string prefixe = sansExtension(argv[3]);

    Decoupeur decoupeur(maillage);
    Reparateur reparateur(decoupeur.plans());

    std::cout << std::endl << "--- Decoupe en 4 quarts ---" << std::endl;
    std::vector<Maillage> quarts = decoupeur.quatreQuarts();
    for (int i = 0; i < (int)quarts.size(); i++) {
        std::string nom = prefixe + "_quart" + std::to_string(i + 1);
        if (quarts[i].n_faces() == 0) {
            std::cout << "Quart " << i + 1 << " : vide, ignore" << std::endl;
            continue;
        }
        afficherSolide(quarts[i], "Quart " + std::to_string(i + 1));
        if (!ecrireFichier(quarts[i], nom + "_decoupe.obj")) {
            return 3;
        }
    }

    std::cout << std::endl << "--- Rebouchage des 4 quarts ---" << std::endl;
    for (int i = 0; i < (int)quarts.size(); i++) {
        std::string nom = prefixe + "_quart" + std::to_string(i + 1);
        if (quarts[i].n_faces() == 0) {
            continue;
        }
        Maillage ferme = reparateur.reboucher(quarts[i]);
        afficherSolide(ferme, "Quart " + std::to_string(i + 1) + " (" + std::to_string(reparateur.nombreBouchons()) + " bouchon(s))");
        if (!ecrireFichier(ferme, nom + "_referme.obj")) {
            return 3;
        }
        if (!ecrireFichier(ferme, nom + "_referme.stl")) {
            return 3;
        }
    }
    return 0;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        afficherUsage(argv[0]);
        return 1;
    }

    Maillage maillage;
    if (!OpenMesh::IO::read_mesh(maillage, argv[1])) {
        std::cerr << "Erreur : impossible de lire '" << argv[1] << "'" << std::endl;
        return 2;
    }

    std::cout << maillage.n_vertices() << " points, " << maillage.n_faces() << " faces" << std::endl;
    TableAretes aretes = construireAretes(maillage);
    std::cout << aretes.size() << " aretes (OpenMesh en compte " << maillage.n_edges() << ")" << std::endl;
    std::cout << decrireBords(analyserBords(aretes)) << std::endl;

    if (argc == 2) {
        return 0;
    }

    std::string commande = argv[2];
    try {
        if (commande == "extruder") {
            return commandeExtruder(maillage, argc, argv);
        }
        if (commande == "decouper") {
            return commandeDecouper(maillage, argc, argv);
        }
        std::cerr << "Erreur : commande inconnue '" << commande << "'" << std::endl;
        afficherUsage(argv[0]);
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Erreur : " << e.what() << std::endl;
        return 4;
    }
}
