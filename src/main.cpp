#include <cstdlib>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "allegement.hpp"
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
    std::string autreSortie;
    float hauteur = 0;
    bool toutesLesFaces = false;
    int face = -1;
    bool cotes = false;
    bool plein = false;
    float socle = SANS_SOCLE;
    float marge = 2;
    int seuilPoussieres = 0;
    int seuilTrous = 0;
    float largeurBille = 0;
};

void afficherUsage(const std::string& programme) {
    std::cerr << "Usage : " << programme << " <fichier.obj> [commande]" << std::endl
              << std::endl
              << "Sans commande : analyse le maillage (points, faces, aretes, bords)" << std::endl
              << std::endl
              << "extruder <sortie.obj|sortie.stl> <hauteur> [face|all [cotes|plein [socle]]]" << std::endl
              << "  hauteur seule : liste des faces du dessus, puis choix au clavier" << std::endl
              << "  face          : cette face monte, reliee a ses voisines" << std::endl
              << "  all           : surface ouverte -> coquille fermee de <hauteur> d'epaisseur vers le bas" << std::endl
              << "                  (copie verticale de la nappe) ;" << std::endl
              << "                  solide ferme -> tout le dessus monte" << std::endl
              << "  all cotes     : comme all, plus une patte de <hauteur> d'epaisseur sous chaque bord, cote matiere," << std::endl
              << "                  qui descend jusqu'a un plan situe marge sous le point le plus bas de la coque" << std::endl
              << "  all cotes <z> : memes pattes, mais jusqu'au plan z = <z> donne" << std::endl
              << "  all plein     : solide plein a fond plat, <hauteur> sous le point le plus bas du dessus ;" << std::endl
              << "                  les rainures restent ouvertes de haut en bas" << std::endl
              << "  all plein <z> : meme solide, fond au plan z = <z> donne" << std::endl
              << "  option marge=<mm> (n'importe ou) : descente des pattes sous la coque, 2 mm par defaut" << std::endl
              << "  option poussieres=<N> (n'importe ou) : avant d'epaissir, supprime les morceaux de moins de N faces" << std::endl
              << "  option trous=<N> (n'importe ou) : avant d'epaissir, bouche les trous d'au plus N aretes de tour" << std::endl
              << "  option bille=<largeur> (n'importe ou, avec all seul) : le dessous suit une bille de <hauteur>" << std::endl
              << "                  de haut et de <largeur> de rayon a plat ; mur d'au moins min(hauteur, largeur)" << std::endl
              << "                  partout, bille ronde si largeur = hauteur" << std::endl
              << std::endl
              << "alleger <sortie.obj|sortie.stl> [autre_sortie.obj|autre_sortie.stl]" << std::endl
              << "  fusionne les triangles sans changer la surface (paliers plats, fond, murs droits)" << std::endl
              << "  deux sorties : le meme resultat ecrit dans les deux fichiers (ex. OBJ et STL)" << std::endl
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
        } else if (mot.rfind("trous=", 0) == 0) {
            options.seuilTrous = std::stoi(mot.substr(6));
        } else if (mot.rfind("marge=", 0) == 0) {
            options.marge = std::stof(mot.substr(6));
        } else if (mot.rfind("bille=", 0) == 0) {
            options.largeurBille = std::stof(mot.substr(6));
            if (!(options.largeurBille > 0)) {
                throw std::invalid_argument("bille : la largeur doit etre positive");
            }
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
        if (argc < 5 || argc > 8) {
            throw std::invalid_argument("extruder : arguments incorrects");
        }
        options.sortie = argv[3];
        options.hauteur = std::stof(argv[4]);
        if (argc >= 6) {
            std::string choix = argv[5];
            if (choix == "all") {
                options.toutesLesFaces = true;
                std::string suite = argc >= 7 ? argv[6] : "";
                if (argc >= 7 && suite != "cotes" && suite != "plein") {
                    throw std::invalid_argument("extruder : apres all, seuls 'cotes' et 'plein' sont acceptes");
                }
                options.cotes = suite == "cotes";
                options.plein = suite == "plein";
                if (argc == 8) {
                    if (!options.cotes && !options.plein) {
                        throw std::invalid_argument("extruder : le socle n'a de sens qu'avec cotes ou plein");
                    }
                    options.socle = std::stof(argv[7]);
                }
            } else {
                if (argc >= 7) {
                    throw std::invalid_argument("extruder : cotes et plein n'ont de sens qu'avec all");
                }
                options.face = std::stoi(choix);
            }
        }
        if (options.largeurBille > 0 && (!options.toutesLesFaces || options.cotes || options.plein)) {
            throw std::invalid_argument("extruder : bille n'a de sens qu'avec all seul");
        }
    } else if (options.commande == "alleger") {
        if (argc != 4 && argc != 5) {
            throw std::invalid_argument("alleger : arguments incorrects");
        }
        options.sortie = argv[3];
        if (argc == 5) {
            options.autreSortie = argv[4];
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
    if (options.seuilTrous > 0) {
        int nbBouches = 0;
        courant = boucherPetitsTrous(courant, tableCourante, options.seuilTrous, nbBouches);
        tableCourante = construireAretes(courant);
        std::cout << nbBouches << " petit(s) trou(s) bouche(s) (au plus " << options.seuilTrous << " aretes de tour)"
                  << std::endl;
    }
    if (options.plein) {
        float socle = options.socle;
        if (std::isnan(socle)) {
            socle = zMinimum(courant, tableCourante) - options.hauteur;
        }
        std::cout << "Plein jusqu'a z = " << socle << " (epaisseur minimale "
                  << zMinimum(courant, tableCourante) - socle << ")" << std::endl;
        return remplirJusquAuSol(courant, tableCourante, socle);
    }
    if (!options.cotes) {
        return epaissir(courant, tableCourante, options.hauteur, options.largeurBille, -HAUT);
    }
    float socle = options.socle;
    if (std::isnan(socle)) {
        socle = zMinimum(courant, tableCourante) - options.hauteur - options.marge;
    }
    std::cout << "Cotes jusqu'a z = " << socle << " (dessous de la coque a z = "
              << zMinimum(courant, tableCourante) - options.hauteur << " au plus bas)" << std::endl;
    return epaissirAvecPattes(courant, tableCourante, options.hauteur, socle);
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
        titre = options.plein ? "Solide plein" : options.cotes ? "Solide avec cotes" : "Solide";
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

int commandeAlleger(const Maillage& maillage, const Options& options) {
    int nbFusions = 0;
    Maillage leger = alleger(maillage, nbFusions);
    std::cout << nbFusions << " fusion(s) d'aretes sans perte" << std::endl;
    afficherSolide(leger, "Allege");
    if (!ecrireFichier(leger, options.sortie)) {
        return 3;
    }
    if (!options.autreSortie.empty() && !ecrireFichier(leger, options.autreSortie)) {
        return 3;
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
        table = TableAretes();
        if (options.commande == "alleger") {
            return commandeAlleger(maillage, options);
        }
        return commandeDecouper(maillage, options);
    } catch (const std::exception& e) {
        std::cerr << "Erreur : " << e.what() << std::endl;
        return 4;
    }
}
