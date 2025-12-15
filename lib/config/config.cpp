#include <config.h>
#include <mIni.h>
#include <string>
#include <threadSound.h>

mINI::INIStructure config;

static int verifieInt(std::string &cle, int min, int max, int parDefaut);

bool readConfig() {
    // Crée un objet INIFile pour lire et écrire dans le fichier de configuration
    mINI::INIFile file("/sd/config.ini");
    file.read(config); // Lit le contenu du fichier INI et le charge dans l'objet 'config'

    // 1. Dossier des stratégies
    std::string &dirStrat = config["Dossiers"]["strategie"]; // Récupère la clé 'strategie' sous la section 'Dossiers'
    if (dirStrat == "") 
        dirStrat = "/strategie"; // Si la valeur est vide, attribue la valeur par défaut "/strategie"

    // 2. Dossier des musiques
    std::string &dirMusique = config["Dossiers"]["musique"]; // Récupère la clé 'musique' sous la section 'Dossiers'
    if (dirMusique == "") 
        dirMusique = "/musique"; // Si la valeur est vide, attribue la valeur par défaut "/musique"

    // 3. Volume initial
    std::string &volume = config["Audio"]["volume"]; // Récupère la clé 'volume' sous la section 'Audio'
    int vInit = verifieInt(volume, 0, 100, 75); // Vérifie si 'volume' est un entier entre 0 et 100, sinon attribue 75
    ThreadSound::setVolume(vInit); // Définit le volume initial via la classe ThreadSound

    // 4. Écrit les mises à jour dans le fichier INI
    file.write(config);

    return true; // Retourne toujours 'true', indiquant un succès
}

bool writeConfig() {
    mINI::INIFile file("/sd/config.ini");
    return file.write(config);
}

int verifieInt(std::string &cle, int min, int max, int parDefaut)
{
    int valeur;
    int nbConv = sscanf(cle.c_str(), "%d", &valeur);
    if (nbConv != 1) {
        valeur = parDefaut;
    }
    if (valeur < min) {
        valeur = min;
    } else if (valeur > max) {
        valeur = max;
    }
    cle = std::to_string(valeur);
    return valeur;
}

