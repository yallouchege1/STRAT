#include "fichiers.h"
#include <algorithm>
#include <cstring>

static bool compInf(Fichier *f1, Fichier *f2)
{
    return f1->getName() < f2->getName();
}

bool Fichier::isValidChar(char c)
{
    if ((unsigned char)(c)<32) return false;
    if (c=='/') return false;
    if (c=='\\') return false;
    if (c==':') return false;
    if (c=='*') return false;
    if (c=='?') return false;
    if (c=='"') return false;
    if (c=='<') return false;
    if (c=='>') return false;
    if (c=='{') return false;
    if (c=='}') return false;
    if (c=='!') return false;
    return true;
}

bool Fichier::isValidName(const string &str, bool isDir)
{
    const char *n = str.c_str();
    if (n == nullptr) return false;
    if (str.size() == 0) return false;
    if (n[0] == ' ') return false;
    char c;
    for (unsigned int i=0; i<str.size(); i++) {
        c = n[i];
        if (isValidChar(c)) continue;
        return false;
    }
    return true;
}

void Fichier::actualiseFullName()
{
    if (m_parent) m_fullname = string(m_parent->getFullName()) + "/" + m_nom;
    else m_fullname = "";
}

bool Fichier::setName(const string &n)
{
    if (m_parent) {
        if (m_parent->dirExist(n)) return false;
        if (m_parent->fileExist(n)) return false;
    }
    bool ok = isValidName(n);
    if (ok) {
        m_nom = n;
        actualiseFullName();
    }
    return ok;
}

Dossier *Fichier::toDossier()
{
    if (isDir()) return static_cast<Dossier *>(this);
    return nullptr;
}

Dossier::~Dossier()
{
    clear();
#ifdef MBED
    if (m_serialize) delete[] m_serialize;
    m_serialize = nullptr;
#endif
}

void Dossier::clear()
{
    for (unsigned int i=0; i<m_fichiers.size(); i++) delete m_fichiers[i];
    m_fichiers.clear();
    for (unsigned int i=0; i<m_dossiers.size(); i++) delete m_dossiers[i];
    m_dossiers.clear();
    m_flagFilled = false;
}

bool Dossier::fileExist(const string &n)
{
    bool ok = (getFile(n) != nullptr);
    return ok;
}

bool Dossier::dirExist(const string &n)
{
    bool ok = (getDir(n) != nullptr);
    return ok;
}

Fichier *Dossier::getFile(const string &n)
{
    Fichier *f = nullptr;
    for (unsigned int i=0; i<m_fichiers.size(); i++) {
        if (n == m_fichiers[i]->getName()) {
            f = m_fichiers[i];
            break;
        }
    }
    return f;
}

Dossier *Dossier::getDir(const string &n)
{
    Dossier *d = nullptr;
    for (unsigned int i=0; i<m_dossiers.size(); i++) {
        if (n == m_dossiers[i]->getName()) {
            d = m_dossiers[i];
            break;
        }
    }
    return d;
}

Fichier *Dossier::getFile(unsigned int num)
{
    if (num < m_fichiers.size()) {
        return m_fichiers[num];
    }
    return nullptr;
}

Dossier *Dossier::getDir(unsigned int num)
{
    if (num < m_dossiers.size()) {
        return m_dossiers[num];
    }
    return nullptr;
}

Fichier *Dossier::addFile(const string &n)
{
    Fichier *nouv = nullptr;
    if (Fichier::isValidName(n)) {
        if (!fileExist(n)) {
            if (!dirExist(n)) {
                nouv = new Fichier(this, n);
                m_fichiers.push_back(nouv);
            }
        }
    }
    return nouv;
}

Dossier *Dossier::addDir(const string &n)
{
    Dossier *nouv = nullptr;
    if (Dossier::isValidName(n)) {
        if (!fileExist(n)) {
            if (!dirExist(n)) {
                nouv = new Dossier(this, n);
                m_dossiers.push_back(nouv);
            }
        }
    }
    return nouv;
}

bool Dossier::rmDir(Dossier *dir)
{
    for (unsigned int i=0; i<m_dossiers.size(); i++) {
        if (m_dossiers[i] == dir) {
            delete dir;
            m_dossiers.erase(m_dossiers.begin()+i);
            return true;
        }
    }
    return false;
}

bool Dossier::rmFile(Fichier *file)
{
    for (unsigned int i=0; i<m_fichiers.size(); i++) {
        if (m_fichiers[i] == file) {
            delete file;
            m_fichiers.erase(m_fichiers.begin()+i);
            return true;
        }
    }
    return false;
}

string Dossier::serialize()
{
    string s = getFullName();
    for (unsigned int i=0; i<m_dossiers.size(); i++) {
        s += "*" + m_dossiers[i]->getName();
    }
    for (unsigned int i=0; i<m_fichiers.size(); i++) {
        s += ":" + m_fichiers[i]->getName();
    }
    s += "?";
    return s;
}

bool Dossier::isEmpty()
{
    return (m_fichiers.size()+m_dossiers.size()) == 0;
}

void Dossier::sort()
{
    std::sort(m_dossiers.begin(), m_dossiers.end(), compInf);
    std::sort(m_fichiers.begin(), m_fichiers.end(), compInf);
    for (unsigned int i=0; i<m_dossiers.size(); i++) m_dossiers[i]->sort();
}

static vector<string> split(const string &source, char sep = '/')
{
    vector<string> liste;
    string tmp = source;
    unsigned int pos;
    while ((pos = tmp.find_first_of(sep)) < tmp.size()) {
        liste.push_back(tmp.substr(0, pos));
        tmp.erase(0, pos+1);
    }
    liste.push_back(tmp);
    return liste;
}

template <typename T> unsigned int indexOf(const vector<T> &tab, const T &elt)
{
    unsigned int i;
    for (i=0; i<tab.size(); i++) {
        if (tab[i] == elt) break;
    }
    return i;
}

void Fichier::validate(string &str)
{
    string rep;
    const char *n = str.c_str();
    if (n == nullptr) {
        str = "________.___";
        return;
    }
    if (!isValidChar(n[0])) rep = "_"; else rep = n[0];
    int i, point;
    char c;
    for (i=1; i<8; i++) {
        c = n[i];
        if (isValidChar(c)) {
            rep += c;
        } else if (c=='\0') {
            str = rep;
            return;
        } else if (c=='.') {
            rep += '.';
            break;
        } else {
            rep += '_';
        }
    }
    if (c != '.') {
        point = -1;
        while (n[i]) {
            if (n[i] == '.') point = i;
            i++;
        }
        if (point == -1) {
            str = rep;
            return;
        }
        rep += '.';
        i = point;
    }
    int j = i+1;
    for (i=j; i<j+3; i++) {
        c = n[i];
        if (c=='\0') {
            if (rep.back() == '.') rep.pop_back();
            str = rep;
            return;
        }
        if (isValidChar(c)) rep += c; else rep += '_';
    }
    str = rep;
}

int Fichier::row()
{
    if (m_parent) return m_parent->indexOf(this);
    return 0;
}

Fichier *Fichier::findFile(const string &path)
{
    vector<string> liste = split(path);
    // liste vide : pas normal
    if (liste.size()==0) return nullptr;
    // premier élément pas vide : pas chemin absolu
    if (liste[0].size()!=0) return nullptr;
    // trouve la racine
    Dossier *d;
    d = (m_parent) ? m_parent : static_cast<Dossier *>(this);
    while (d->getParent()) d = d->getParent();
    // descend dans l'arbre
    for (unsigned int i=1; i<liste.size()-1; i++) {
        if (!d->dirExist(liste[i])) { // le chemin n'existe pas
            return nullptr;
        }
        d = d->getDir(liste[i]);
    }
    return d->getFile(liste.back());
}

Dossier *Fichier::findDir(const string &path)
{
    vector<string> liste = split(path);
    // liste vide : pas normal
    if (liste.size()==0) return nullptr;
    // premier élément pas vide : pas chemin absolu
    if (liste[0].size()!=0) return nullptr;
    // trouve la racine
    Dossier *d;
    d = (m_parent) ? m_parent : static_cast<Dossier *>(this);
    while (d->getParent()) d = d->getParent();
    // si chemin racine
    if (path == "/") return d;
    // descend dans l'arbre
    for (unsigned int i=1; i<liste.size(); i++) {
        if (!d->dirExist(liste[i])) { // le chemin n'existe pas
            return d;
        }
        d = d->getDir(liste[i]);
    }
    return d;
}

string Fichier::dirOfFile(const string &fullName)
{
    string dir;
    vector<string> liste = split(fullName);
    if (liste.size()==0) return "/";
    for (unsigned int i=1; i<liste.size()-1; i++) {
        dir += "/" + liste[i];
    }
    if (dir.size()==0) return "/";
    return dir;
}

string Fichier::nameOfFile(const string &fullName)
{
    string name;
    vector<string> liste = split(fullName);
    if (liste.size()<2) return name;
    name = liste.back();
    return name;
}

bool Dossier::rm(Fichier *fileDir)
{
    Dossier *d = fileDir->toDossier();
    return (d) ? rmDir(d) : rmFile(fileDir);
}

Fichier *Dossier::child(int row)
{
    if (row < 0) return nullptr;
    unsigned int r = static_cast<unsigned int>(row);
    if (r < m_dossiers.size()) return m_dossiers[r];
    r -= m_dossiers.size();
    if (r < m_fichiers.size()) return m_fichiers[r];
    return nullptr;
}

int Dossier::indexOf(Fichier *f) const
{
    Dossier *d = f->toDossier();
    if (d) return ::indexOf(m_dossiers, d);
    return m_dossiers.size() + ::indexOf(m_fichiers, f);
}

string Dossier::suggestFileName(const string &src)
{
    string dest = src;
    int i = 0;
    char num[11];
    if (!isValidFileName(src)) validate(dest);
    if (dirExist(dest) || fileExist(dest)) {
        vector<string> liste = split(dest, '.');
        string name = liste[0];
        if (liste.size()==1) {
            do {
                sprintf(num, "%03d", i);
                dest = name + '.' + string(num);
                i++;
                if (i>=1000) {
                    // erreur
                    break;
                }
            } while (dirExist(dest) || fileExist(dest));
        } else {
            do {
                sprintf(num, "%d", i);
                int len = strlen(num);
                if (name.size() + len > 8) name.resize(8 - len);
                dest = name + num + '.' + liste[liste.size()-1];
                i++;
            } while (dirExist(dest) || fileExist(dest));
        }
    }
    return dest;
}

#ifdef MBED

int Dossier::fill(FileSystem *fs)
{
    if (m_flagFilled) return 0;
    Dir dir;
    int error;

    error = dir.open(fs, getFullName().c_str());
    if (!error) {
        struct dirent de;
        while (dir.read(&de) > 0) {
            if (de.d_type == DT_DIR) {
                /*Dossier *nouv = */addDir(de.d_name);
            } else {
                /*Fichier *nouv = */addFile(de.d_name);
            }
        }
        // dir.close(); // appel par le destructeur
    }
    m_flagFilled = true;
    return error;
}

char *Dossier::m_serialize = nullptr;

char *Dossier::serialize(FileSystem *fs)
{
    if (!m_flagFilled) fill(fs);
    string rep = serialize();
    if (m_serialize) delete[] m_serialize;
    m_serialize = new char[rep.size()+1];
    strcpy(m_serialize, rep.c_str());
    return m_serialize;
}

#else

void Dossier::fill(char *message)
{
    clear();
    char *p = message;
    int debut = 0;
    while (*p != '\0') {
        if (*p == '*') {
            *p = '\0';
            if (debut == 1) addDir(message);
            else if (debut == 2) addFile(message);
            message = p+1;
            debut = 1;
        } else if (*p == ':') {
            *p = '\0';
            if (debut == 1) addDir(message);
            else if (debut == 2) addFile(message);
            message = p+1;
            debut = 2;
        } else if (*p == '?') {
            *p = '\0';
            if (debut == 1) addDir(message);
            else if (debut == 2) addFile(message);
            break;
        }
        p++;
    }
    m_flagFilled = true;
}

#endif

