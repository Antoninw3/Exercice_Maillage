#include <cstdlib>
#include <cmath>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "aretes.hpp"
#include "bords.hpp"
#include "decoupeur.hpp"
#include "epaississement.hpp"
#include "maillage.hpp"
#include "reparateur.hpp"

struct Options {
    std::string programme;
    std::string fichier;
    std::string commande;
    std::string sortie;
    float hauteur = 0;
    bool toutesLesFaces = false;
    int face = -1;
    float plancher = SANS_PLANCHER;
    int seuilPoussieres = 0;
};

void afficherUsage(const std::string& programme) {
    std::cerr << "Usage : " << programme << " <fichier.obj> [commande]" << std::endl
              << std::endl
              << "Sans commande : analyse le maillage (points, faces, aretes, bords)" << std::endl
              << std::endl
              << "extruder <sortie.obj|sortie.stl> <hauteur> [face|all [plancher]]" << std::endl
              << "  hauteur seule : liste des faces du dessus, puis choix au clavier" << std::endl
              << "  face          : cette face monte, reliee a ses voisines" << std::endl
              << "  all           : surface ouverte -> coquille fermee de <hauteur> d'epaisseur vers le bas ;" << std::endl
              << "                  solide ferme -> tout le dessus monte" << std::endl
              << "  all <plancher>: comme all, mais le dessous ne descend pas sous z = plancher (plat a ces endroits)" << std::endl
              << "  option poussieres=<N> (n'importe ou) : avant d'epaissir, supprime les morceaux de moins de N faces" << std::endl
              << std::endl
              << "decouper <prefixe>" << std::endl
              << "  coupe le maillage en 4 quarts puis les rebouche, et ecrit :" << std::endl
              << "  <prefixe>_quart1_decoupe.obj  (apres decoupe)" << std::endl
              << "  <prefixe>_quart1_referme.obj  (apres rebouchage)" << std::endl
              << "  <prefixe>_quart1_referme.stl  (apres rebouchage, en STL)" << std::endl;
}

Options lireOptions(int argcBrut, char** argvBrut) {
    Options options;
    options.programme = argvBrut[0];

    std::vector<char*> argv;
    for (int i = 0; i < argcBrut; i++) {
        std::string mot = argvBrut[i];
        if (mot.rfind("poussieres=", 0) == 0) {
            options.seuilPoussieres = std::stoi(mot.substr(11));
        } else {
            argv.push_back(argvBrut[i]);
        }
    }
    int argc = argv.size();

    if (argc < 2) {
        throw std::invalid_argument("fichier manquant");
    }
    options.fichier = argv[1];
    if (argc == 2) {
        return options;
    }
    options.commande = argv[2];

    if (options.commande == "extruder") {
        if (argc < 5 || argc > 7) {
            throw std::invalid_argument("extruder : arguments incorrects");
        }
        options.sortie = argv[3];
        options.hauteur = std::stof(argv[4]);
        if (argc >= 6) {
            std::string choix = argv[5];
            if (choix == "all") {
                options.toutesLesFaces = true;
                if (argc == 7) {
                    options.plancher = std::stof(argv[6]);
                }
            } else {
                if (argc == 7) {
                    throw std::invalid_argument("extruder : le plancher n'a de sens qu'avec all");
                }
                options.face = std::stoi(choix);
            }
        }
    } else if (options.commande == "decouper") {
        if (argc != 4) {
            throw std::invalid_argument("decouper : arguments incorrects");
        }
        options.sortie = argv[3];
    } else {
        throw std::invalid_argument("commande inconnue '" + options.commande + "'");
    }
    return options;
}

void afficherSolide(const Maillage& solide, const std::string& titre) {
    TableAretes table = construireAretes(solide);
    std::string etat = analyserBords(table).estFerme() ? "ferme" : "NON ferme";
    std::cout << titre << " : " << solide.n_vertices() << " points, " << solide.n_faces() << " faces, "
              << table.size() << " aretes, " << etat << std::endl;
}

bool finitPar(const std::string& texte, const std::string& fin) {
    return texte.size() >= fin.size() && texte.compare(texte.size() - fin.size(), fin.size(), fin) == 0;
}

bool ecrireFichier(const Maillage& maillage, const std::string& chemin) {
    bool ok = false;
    if (finitPar(chemin, ".stl") || finitPar(chemin, ".STL")) {
        Maillage triangule = maillage;
        triangule.triangulate();
        ok = OpenMesh::IO::write_mesh(triangule, chemin, OpenMesh::IO::Options::Binary);
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
    for (int v : liste) {
        if (v == valeur) {
            return true;
        }
    }
    return false;
}

int choisirFaceDuDessus(const std::vector<int>& candidates, int demandee) {
    if (candidates.empty()) {
        throw std::runtime_error("aucune face du dessus dans ce maillage");
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

int compterMorceaux(const TableAretes& table) {
    int nbMorceaux = 0;
    morceauDeChaqueFace(table, nbMorceaux);
    return nbMorceaux;
}

Maillage epaissirNappe(const Maillage& nappe, const TableAretes& table, const Options& options) {
    Maillage courant = nappe;
    TableAretes tableCourante = table;

    int pincements = compterPincements(table);
    if (pincements > 0) {
        courant = separerPincements(nappe, table);
        tableCourante = construireAretes(courant);
        std::cout << pincements << " sommet(s) pince(s) separe(s)" << std::endl;
    }

    if (options.seuilPoussieres > 0) {
        int avant = compterMorceaux(tableCourante);
        courant = supprimerPoussieres(courant, tableCourante, options.seuilPoussieres);
        tableCourante = construireAretes(courant);
        int apres = compterMorceaux(tableCourante);
        std::cout << avant - apres << " poussiere(s) supprimee(s), " << apres << " morceau(x) conserve(s)" << std::endl;
    }
    return epaissir(courant, tableCourante, options.hauteur, -HAUT, options.plancher);
}

int commandeExtruder(const Maillage& maillage, const TableAretes& table, const Options& options) {
    Maillage resultat;
    std::string titre;

    if (options.toutesLesFaces && analyserBords(table).estFerme()) {
        std::vector<int> dessus = facesDuDessus(maillage, HAUT);
        resultat = extruderFaces(maillage, table, dessus, options.hauteur, HAUT);
        titre = "Dessus extrude (" + std::to_string(dessus.size()) + " faces)";
    } else if (options.toutesLesFaces) {
        resultat = epaissirNappe(maillage, table, options);
        titre = "Solide";
        if (!std::isnan(options.plancher)) {
            std::ostringstream flux;
            flux << "Solide (dessous plat sous z = " << options.plancher << ")";
            titre = flux.str();
        }
    } else {
        int face = choisirFaceDuDessus(facesDuDessus(maillage, HAUT), options.face);
        resultat = extruderFaces(maillage, table, {face}, options.hauteur, HAUT);
        titre = "Face " + std::to_string(face) + " extrudee";
    }

    afficherSolide(resultat, titre);
    return ecrireFichier(resultat, options.sortie) ? 0 : 3;
}

std::string sansExtension(const std::string& chemin) {
    const char* extensions[] = {".obj", ".OBJ", ".stl", ".STL"};
    for (const char* extension : extensions) {
        if (finitPar(chemin, extension)) {
            return chemin.substr(0, chemin.size() - std::string(extension).size());
        }
    }
    return chemin;
}

int commandeDecouper(const Maillage& maillage, const Options& options) {
    std::string prefixe = sansExtension(options.sortie);
    std::vector<Plan> plans = plansMedians(maillage);

    std::cout << std::endl << "--- Decoupe en 4 quarts ---" << std::endl;
    std::vector<Maillage> quarts = quatreQuarts(maillage, plans);
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
        if (quarts[i].n_faces() == 0) {
            continue;
        }
        std::string nom = prefixe + "_quart" + std::to_string(i + 1);
        Maillage ferme = reboucher(quarts[i], plans);
        int bouchons = ferme.n_faces() - quarts[i].n_faces();
        afficherSolide(ferme, "Quart " + std::to_string(i + 1) + " (" + std::to_string(bouchons) + " bouchon(s))");
        if (!ecrireFichier(ferme, nom + "_referme.obj") || !ecrireFichier(ferme, nom + "_referme.stl")) {
            return 3;
        }
    }
    return 0;
}

int main(int argc, char** argv) {
    Options options;
    try {
        options = lireOptions(argc, argv);
    } catch (const std::exception& e) {
        std::cerr << "Erreur : " << e.what() << std::endl << std::endl;
        afficherUsage(argv[0]);
        return 1;
    }

    Maillage maillage;
    if (!OpenMesh::IO::read_mesh(maillage, options.fichier)) {
        std::cerr << "Erreur : impossible de lire '" << options.fichier << "'" << std::endl;
        return 2;
    }

    std::cout << maillage.n_vertices() << " points, " << maillage.n_faces() << " faces" << std::endl;
    TableAretes table = construireAretes(maillage);
    std::cout << table.size() << " aretes (OpenMesh en compte " << maillage.n_edges() << ")" << std::endl;
    std::cout << decrireBords(analyserBords(table)) << std::endl;
    std::cout << compterMorceaux(table) << " morceau(x)" << std::endl;

    if (options.commande.empty()) {
        return 0;
    }
    try {
        if (options.commande == "extruder") {
            return commandeExtruder(maillage, table, options);
        }
        return commandeDecouper(maillage, options);
    } catch (const std::exception& e) {
        std::cerr << "Erreur : " << e.what() << std::endl;
        return 4;
    }
}
