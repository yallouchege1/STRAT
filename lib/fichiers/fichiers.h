#ifndef FICHIERS_H
#define FICHIERS_H

#define MBED

#ifdef MBED
#include "mbed.h"
#endif

#include <string>
#include <vector>

using std::string;
using std::vector;

class Dossier;

class Fichier
{
private:
    string m_nom, m_fullname;
    Dossier *m_parent;
    static bool isValidChar(char c);
    void actualiseFullName();
protected:
    static bool isValidName(const string &str, bool isDir);
public:
    Fichier(Dossier *p, const string &n) : m_nom(n), m_parent(p) { actualiseFullName(); }
    virtual ~Fichier() {}
    string getName() { return m_nom; }
    string getFullName() { return m_fullname; }
    Dossier *getParent() { return m_parent; }
    static bool isValidFileName(const string &n) { return isValidName(n, false); }
    virtual bool isValidName(const string &n) { return isValidFileName(n); }
    bool setName(const string &n);
    virtual bool isFile() { return true; }
    virtual bool isDir() { return false; }
    static void validate(string &str);
    Fichier *findFile(const string &path);
    Dossier *findDir(const string &path);
    static string dirOfFile(const string &fullName);
    static string nameOfFile(const string &fullName);
    Dossier *toDossier();
    virtual int childCount() { return 0; }
    virtual Fichier *child(int ) { return nullptr; }
    int row();
};

class Dossier : public Fichier
{
private:
    vector<Fichier *> m_fichiers;
    vector<Dossier *> m_dossiers;
    bool m_flagFilled;
public :
    Dossier(Dossier *p=nullptr, const string &n=string("")) : Fichier(p, n), m_flagFilled(false) {}
    virtual ~Dossier();
    bool fileExist(const string &n);
    bool dirExist(const string &n);
    Fichier *addFile(const string &n);
    Dossier *addDir(const string &n);
    void clear();
    bool isFilled() { return m_flagFilled; }
    Fichier *getFile(unsigned int num);
    Dossier *getDir(unsigned int num);
    Fichier *getFile(const string &n);
    Dossier *getDir(const string &n);
    static bool isValidDirName(const string &n) { return Fichier::isValidName(n, true); }
    virtual bool isValidName(const string &n) { return isValidDirName(n); }
    bool rmDir(Dossier *dir);
    bool rmFile(Fichier *file);
    string serialize();
    bool isEmpty();
    void sort();
    virtual bool isFile() { return false; }
    virtual bool isDir() { return true; }
    bool rm(Fichier *fileDir);
    virtual int childCount() { return m_fichiers.size()+m_dossiers.size(); }
    int dirCount() { return m_dossiers.size(); }
    virtual Fichier *child(int row);
    int indexOf(Fichier *f) const;
    string suggestFileName(const string &src);
#ifdef MBED
    int fill(FileSystem *fs);
    char *serialize(FileSystem *fs);
private:
    static char *m_serialize;
public:
#else // pas MBED
    void fill(char *msg);
#endif // fin pas MBED
};

#endif // FICHIERS_H
