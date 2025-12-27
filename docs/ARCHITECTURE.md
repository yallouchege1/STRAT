# 🏗️ ARCHITECTURE - Écran Stratégie V4

> Documentation technique détaillée de l'architecture logicielle et matérielle

---

## Table des matières

1. [Vue d'ensemble](#vue-densemble)
2. [Architecture logicielle](#architecture-logicielle)
3. [Modules détaillés](#modules-détaillés)
4. [Graphe de dépendances](#graphe-de-dépendances)
5. [Communication CAN](#communication-can)
6. [Threading et concurrence](#threading-et-concurrence)
7. [Système de fichiers](#système-de-fichiers)

---

## Vue d'ensemble

### Principes architecturaux

1. **Modularité par responsabilité** : Chaque module a un rôle clairement défini
2. **RTOS-first design** : Utilisation intensive de mbed OS pour la concurrence
3. **Pattern callbacks** : Communication CAN via callbacks enregistrées
4. **Thread-safety** : Protection par Mutex pour ressources partagées
5. **Singleton** : Pattern utilisé pour ThreadSound (instance unique)
6. **State Machine** : Machine d'états pour orchestration du match

### Technologies clés

- **Langage** : C++11
- **RTOS** : mbed OS 6.x
- **GUI** : LVGL 8.3.4
- **Système de fichiers** : FAT sur SD card
- **Audio** : Helix MP3 decoder
- **Communication** : CAN 2.0B à 1 Mbps

---

## Architecture logicielle

### Diagramme en couches

```
┌─────────────────────────────────────────────────────────┐
│ NIVEAU 6: APPLICATION                                   │
│ main.cpp - Orchestration système                       │
└─────────────────────────────────────────────────────────┘
                          ↑
┌─────────────────────────────────────────────────────────┐
│ NIVEAU 5: ORCHESTRATION & UI                           │
│ strategie, ihm, test                                    │
└─────────────────────────────────────────────────────────┘
                          ↑
┌─────────────────────────────────────────────────────────┐
│ NIVEAU 4: CONTRÔLE ROBOT                                │
│ deplacement, herkulex, evitement, instruction           │
└─────────────────────────────────────────────────────────┘
                          ↑
┌─────────────────────────────────────────────────────────┐
│ NIVEAU 3: DONNÉES & CONFIGURATION                      │
│ global, config, identCrac                               │
└─────────────────────────────────────────────────────────┘
                          ↑
┌─────────────────────────────────────────────────────────┐
│ NIVEAU 2: THREADING & SYSTÈME                          │
│ threadCAN, threadLvgl, threadSD, threadSound, fichiers │
└─────────────────────────────────────────────────────────┘
                          ↑
┌─────────────────────────────────────────────────────────┐
│ NIVEAU 1: DRIVERS & ABSTRACTIONS                       │
│ sdio, lvgl_fs_driver, mesFonts, identCrac.h            │
└─────────────────────────────────────────────────────────┘
                          ↑
┌─────────────────────────────────────────────────────────┐
│ NIVEAU 0: HARDWARE & BIBLIOTHÈQUES EXTERNES            │
│ mbed, BSP_DISCO_F469NI, lvgl-8.3.4, mp3decoder         │
└─────────────────────────────────────────────────────────┘
```

---

## Modules détaillés

### 1. THREADING - Communication asynchrone

#### **threadCAN** - Bus CAN RTOS

**Fichiers** : `threadCAN.h`, `threadCAN.cpp`, `privateCAN.h`

**Responsabilités** :
- Gestion bus CAN2 (1 Mbps) sur PB_5 (RX) et PB_13 (TX)
- 3 threads internes : lecture, dispatch, écriture
- Buffer mail : 100 messages (read + write queues)
- Système callbacks par plages d'IDs

**API principale** :
```cpp
class ThreadCAN {
public:
    // Enregistrer callback pour plage d'IDs
    void registerIds(int idMin, int idMax, void (*callback)(CANMessage*));

    // Envoyer message CAN
    void send(CANMessage& msg);
    void send(int id, char* data, char len);
    void sendAck(uint32_t id, char data);

    // Contrôle threads
    void start();
    void stop();
};
```

**Priorités threads** :
- CAN read : `osPriorityHigh`
- CAN dispatch : `osPriorityHigh`
- CAN write : `osPriorityAboveNormal`

**Thread-safety** : Mutex interne pour accès buffers

---

#### **threadLvgl** - Rendu GUI

**Fichiers** : `threadLvgl.h`, `threadLvgl.cpp`

**Responsabilités** :
- Thread dédié rendu LVGL
- Ticker refresh 5ms (200 FPS max)
- Protection mutex pour opérations GUI
- Initialisation LCD OTM8009A 800x480
- Initialisation tactile FT6x06

**API principale** :
```cpp
class ThreadLvgl {
public:
    // Protection mutex obligatoire
    void lock();
    void unlock();

    // Démarrage thread rendu
    void start();
};
```

**Utilisation** :
```cpp
threadLvgl.lock();
// Opérations LVGL (création widgets, etc.)
lv_label_set_text(label, "Test");
threadLvgl.unlock();
```

---

#### **threadSD** - Système de fichiers

**Fichiers** : `threadSD.h`, `threadSD.cpp`

**Responsabilités** :
- Montage FAT filesystem à `/sd/`
- Opérations asynchrones (LS, CD, MKDIR, DEL, RENAME, UPLOAD, DOWNLOAD, COPY)
- Contrôle CAN distant (4 IDs : 0x3F0-0x3F3)
- Drapeaux d'état : INIT, NO_CARD, READY, BUSY, CAN_REQUEST, CAN_CONTROL

**API principale** :
```cpp
class ThreadSD {
public:
    enum Flags {
        FLAG_INIT = (1 << 0),
        FLAG_NO_CARD = (1 << 1),
        FLAG_READY = (1 << 2),
        FLAG_BUSY = (1 << 3)
    };

    void registerCANControl(ThreadCAN& can);
    void waitReady();
    int status();
    string cdName(const char* path);
    string ls();
};
```

**Timeout** : 60s si carte SD absente → reset système

---

#### **threadSound** - Décodage MP3

**Fichiers** : `threadSound.h`, `threadSound.cpp`

**Responsabilités** :
- Singleton (instance unique)
- Décodage MP3 via Helix codec
- Playback sur CS43L22 codec audio
- Callbacks DMA (half + complete)

**API principale** :
```cpp
class ThreadSound {
public:
    // Singleton
    static ThreadSound& getInstance();

    // Contrôle playback
    void play(const char* filename);
    void stop();
    void pause();
    void resume();
    void setVolume(uint8_t volume);  // 0-100
    void mute(bool enable);

    // État
    enum Error { CODEC_ERROR, DMA_ERROR, FILE_ERROR, ... };
};
```

---

### 2. CONTRÔLE ROBOT

#### **deplacement** - Asservissement moteurs

**Fichiers** : `deplacement.h`, `deplacement.cpp`

**Responsabilités** :
- Interface CAN vers carte asservissement moteur
- Commandes déplacement : XYT, rotation, ligne, courbure
- Courbes Bézier multi-segments
- Recalage bordures (X/Y/T)
- Lecture odométrie

**API principale** :
```cpp
class Deplacement {
public:
    Deplacement(ThreadCAN& can);

    // Mouvements
    void positionXYTheta(int x, int y, int theta);
    void rotation(int angle, bool relatif);
    void toutDroit(int distance);
    void courbure(int rayon, int angle);
    void bezier(int nb_segments, ...);

    // Recalage
    void recalageX();
    void recalageY();
    void recalageTheta();

    // Configuration
    void vitesseMax(int vitesse);
    void acceleration(int accel);

    // Jack sécurité
    bool jack();
};
```

**IDs CAN utilisés** : 0x020-0x038 (asservissement), 0x026-0x028 (odométrie)

---

#### **herkulex** - Actionneurs

**Fichiers** : `herkulex.h`, `herkulex.cpp`

**Responsabilités** :
- Contrôle servos Herkulex (LED RGB feedback)
- Grippers/pinces avant/arrière
- Aspirateurs/ventouses
- Lanceur
- Moteur pas-à-pas hauteur

**API principale** :
```cpp
class Herkulex {
public:
    Herkulex(ThreadCAN& can);

    // Servos
    void controlePince(int id, int position);
    void controleHerkulexPosition(int servo, int angle);
    void controleHerkulexVitesse(int servo, int vitesse);

    // Actionneurs
    void controleAspirateur(int id, bool actif);
    void controleLanceur(bool actif);
    void controleHauteur(int etage);

    // Feedback LED
    enum LED { VERT, ROUGE, BLEU };
    void setLED(int servo, LED couleur);
};
```

**IDs CAN utilisés** : 0x255-0x277

---

### 3. LOGIQUE MATCH

#### **strategie** - Machine d'états

**Fichiers** : `strategie.h`, `strategie.cpp`

**États** (15 au total) :
```cpp
enum E_stratGameEtat {
    ETAT_GAME_INIT,                     // Initialisation
    ETAT_GAME_RECALAGE,                 // Calibration bordures
    ETAT_GAME_WAIT_FOR_JACK,            // Attente retrait jack
    ETAT_GAME_START,                    // Début match (timer 100s)
    ETAT_GAME_LOAD_NEXT_INSTRUCTION,    // Charger instruction suivante
    ETAT_GAME_PROCESS_INSTRUCTION,      // Exécuter instruction
    ETAT_GAME_WAIT_ACK,                 // Attendre accusé réception
    ETAT_GAME_OBSTACLE,                 // Gestion obstacle lidar
    ETAT_END_LOOP                       // Fin match
    // ...
};
```

**Boucle principale** :
```cpp
// Dans main.cpp
while (machineStrategie()) {
    // État machine mis à jour automatiquement
    // Retourne false quand match terminé
}
```

**Handler CAN** :
```cpp
void canProcessRx(CANMessage* msg) {
    // Traite ACKs, odométrie, lidar
    // Met à jour flags pour débloquer état WAIT_ACK
}
```

---

#### **instruction** - Parser stratégie

**Fichiers** : `instruction.h`, `instruction.cpp`

**Types d'instructions** :
```cpp
enum EnumInstructionType {
    MV_BEZIER,      // Courbe Bézier
    MV_COURBURE,    // Arc cercle
    MV_LINE,        // Ligne droite
    MV_TURN,        // Rotation
    MV_XYT,         // Position absolue
    MV_RECALAGE,    // Recalage bordure
    ACTION,         // Commande actionneur
    PINCE,          // Commande pince
    UNKNOWN
};
```

**Structure instruction** :
```cpp
struct Instruction {
    int ordre;              // Numéro séquentiel
    EnumInstructionType type;
    int parametres[12];     // Paramètres variables
    EnumInstructionPrecision precision;
    EnumInstructionNextAction next;
};
```

**Format fichier** :
```
B 2 100 50 0 3.14 1 2 0 50 100 0 0    # Bézier 2 segments
L 1000 N 1 0 0                        # Ligne 1000mm
T 90 R 1 0 0                          # Tourner 90° droite
A PINCE 1 2                           # Action pince
```

---

### 4. INTERFACE UTILISATEUR

#### **ihm** - GUI tactile

**Fichiers** : `ihm.h`, `ihm.cpp`

**Onglets** (5 au total) :
1. **CarteSd2** : Détection carte SD + compteur fichiers
2. **Match** ("Show") : Sélection stratégie + couleur + GO
3. **Test Actionneur** : Tests manuels actionneurs
4. **Test** : Tests ventouses (nouveau v0.1)
5. **Config** : (optionnel, à implémenter)

**Flags événements** (31 au total) :
```cpp
enum IhmFlag {
    IHM_FLAG_DEPART,
    IHM_FLAG_REFRESH_SD,
    IHM_FLAG_RECALAGE,
    IHM_FLAG_START,
    IHM_FLAG_Testventouse,
    IHM_FLAG_Gradin_niveaux_2,
    IHM_FLAG_Niveaux_base,
    IHM_FLAG__autre,
    // ... 31 flags au total
};
```

**API principale** :
```cpp
class Ihm {
public:
    Ihm(ThreadLvgl* t);

    // Initialisation onglets
    void show(const vector<string>& fichiers);
    void ActionneurInit();
    void testTabInit();

    // Vérification événements (flags)
    bool departClicked();
    bool Test_ventouse();
    bool construction_niveaux_2();
    bool Niveaux_base();
    bool Position_init();
    bool lacherflag();
    bool autretest();

    // Message boxes de sélection
    void showVentousePositionBox();
    void showVentouseNumeroBox();
    void showVentouseActionBox();

    // Statut carte SD
    void updateCarteSd2Status(bool detected, int fileCount);

    // Récupération choix utilisateur
    int choixStrategie();
    int choixCouleur();
};
```

**Polices** :
- `liberation_24` : Police normale (24pt)
- `liberation_40` : Police large (40pt)

---

### 5. DONNÉES ET CONFIGURATION

#### **global** - Variables partagées

**Fichier** : `global.h` (header-only)

**Variables clés** :
```cpp
// Position robot (mm, radians)
extern float x_robot, y_robot, theta_robot;
extern float target_x_robot, target_y_robot, target_theta_robot;

// État robot
extern int Cote;  // 0=JAUNE, 1=VIOLET
extern couleurDepart color;  // Jaune ou BLEU

// État match
extern E_stratGameEtat gameEtat;
extern ListeInstruction listeInstructions;
extern int SCORE_GLOBAL, SCORE_PR, SCORE_GR;

// Communication
extern EventFlags flag;  // Flags inter-thread
extern uint32_t waitingAckID, waitingAckFrom, waitingId;
```

**Constantes** :
```cpp
#define SIZE_FIFO 50
#define SIZE 750
#define MOITIE_ROBOT 118  // mm
```

---

#### **identCrac** - Définitions CAN

**Fichier** : `identCrac.h` (header-only, 254 #defines)

**Plages d'IDs** :

| Plage | Fonction | Exemples |
|-------|----------|----------|
| 0x001-0x010 | Contrôle global | GLOBAL_STOP (0x001), GLOBAL_JACK (0x008) |
| 0x020-0x038 | Asservissement | ASSERVISSEMENT_XYT (0x020), ROTATION (0x023) |
| 0x100-0x11F | ACKs | ACK_MOTEUR (0x101), ACK_ACTIONNEURS (0x103) |
| 0x111-0x11F | FIN instruction | INSTRUCTION_END_MOTEUR (0x111) |
| 0x255-0x277 | Actionneurs | VENT_AV (0x266), VENT_AR (0x267), PINCE (0x255) |
| 0x3B0-0x3C6 | Télémétrie | ODOMETRIE_X_POSITION (0x026) |

---

#### **config** - Fichier INI

**Fichiers** : `config.h`, `config.cpp`, `mIni.h`

**Structure** :
```cpp
extern mINI::INIStructure config;

void readConfig();   // Charge depuis /sd/config.ini
void writeConfig();  // Sauvegarde modifications
```

**Exemple config.ini** :
```ini
[Dossiers]
strategie=/strategie
mp3=/mp3

[Robot]
vitesse_max=100
acceleration=50
couleur=jaune

[Audio]
volume=75
```

**Utilisation** :
```cpp
config["Dossiers"]["strategie"];  // "/strategie"
config["Robot"]["vitesse_max"];    // "100"
```

---

## Graphe de dépendances

### Dépendances inter-modules

```
main.cpp
├─► threadCAN
├─► threadLvgl
├─► threadSD
│    └─► threadCAN (contrôle CAN)
│    └─► fichiers.h (listing)
├─► threadSound
├─► ihm
│    └─► threadLvgl (rendu)
│    └─► threadCAN (flags)
├─► deplacement
│    └─► threadCAN (commandes)
├─► herkulex
│    └─► threadCAN (commandes)
├─► strategie
│    └─► threadSD (charger instructions)
│    └─► instruction.h (parser)
│    └─► deplacement (exécuter mouvements)
│    └─► herkulex (exécuter actions)
├─► config
│    └─► mIni.h (parser INI)
├─► global.h (variables)
└─► identCrac.h (IDs CAN)
```

### Flux de communication CAN

```
Initialisation (main.cpp):
  threadCAN.registerIds(0x01, 0x7FF, canProcessRx)
    │
    └─► Callback global canProcessRx()
          ├─► strategie.canProcessRx() [ACK, positions]
          ├─► evitement.trameCan() [lidar]
          ├─► Global vars update [x_robot, y_robot, theta_robot]
          └─► flags.set() [débloquer état machine]

Exécution match (strategie.machineStrategie()):
  ├─► deplacement.* () → envoi CAN → attente ACK
  ├─► herkulex.* () → envoi CAN → attente ACK
  └─► instruction parsing & execution
```

---

## Threading et concurrence

### Threads actifs

| Thread | Priorité | Période/Trigger | Stack Size | Fonction |
|--------|----------|------------------|------------|----------|
| CAN read | High | Event (CAN RX) | 2048 | Lecture messages CAN |
| CAN dispatch | High | Event (Mail) | 2048 | Dispatch callbacks |
| CAN write | AboveNormal | Event (Mail) | 2048 | Écriture messages CAN |
| LVGL render | Normal | 5 ms (Ticker) | 4096 | Rendu GUI |
| SD daemon | Normal | Event (Mail) | 4096 | Opérations fichiers |
| Sound decode | Normal | Event (DMA) | 4096 | Décodage MP3 |
| Main | Normal | 10 ms (loop) | Default | Machine états match |

### Synchronisation

**EventFlags** : Signalisation inter-thread
```cpp
EventFlags flag;
flag.set(ACKFrom_FLAG);        // Set flag
flag.wait_any(ACKFrom_FLAG);   // Attendre flag (bloquant)
flag.clear(ACKFrom_FLAG);      // Effacer flag
```

**Mutex** : Protection ressources partagées
```cpp
Mutex mutex;
mutex.lock();
// Section critique
mutex.unlock();
```

**Mail** : Communication thread-safe
```cpp
Mail<CANMessage, 100> mailbox;
mailbox.put(msg);              // Envoi non-bloquant
msg = mailbox.get();           // Réception bloquante
```

---

## Système de fichiers

### Montage SD

```cpp
// Dans threadSD
FATFileSystem fs("sd");
SDIOBlockDevice sd(/* SDIO pins */);

int err = fs.mount(&sd);
if (err) {
    // Carte SD absente ou erreur montage
}
```

### Arborescence typique

```
/sd/
├── config.ini              # Configuration système
├── strategie/
│   ├── strat_jaune.txt     # Stratégie côté jaune
│   ├── strat_bleu.txt      # Stratégie côté bleu
│   └── test.txt            # Stratégie test
└── mp3/
    ├── depart.mp3          # Son démarrage
    ├── fin.mp3             # Son fin match
    └── erreur.mp3          # Son erreur
```

### Opérations fichiers

```cpp
// Lister fichiers
string reply = threadSD.cdName("/strategie");
string liste = threadSD.ls();

// Lire fichier
FILE* f = fopen("/sd/strategie/strat_jaune.txt", "r");
while (fread(&data, 1, 1, f)) {
    // Traiter données
}
fclose(f);

// Écrire fichier
FILE* f = fopen("/sd/config.ini", "w");
fprintf(f, "[Section]\nkey=value\n");
fclose(f);
```

---

## Points d'attention

### Thread-safety

⚠️ **Critique** : LVGL n'est PAS thread-safe
```cpp
// TOUJOURS protéger avec mutex
threadLvgl.lock();
lv_label_set_text(label, "Test");  // OK
threadLvgl.unlock();

// JAMAIS sans protection
lv_label_set_text(label, "Test");  // ❌ CRASH possible !
```

### CAN callbacks

⚠️ **Ne pas bloquer** dans les callbacks CAN
```cpp
void canCallback(CANMessage* msg) {
    flags.set(MY_FLAG);             // ✅ OK

    ThisThread::sleep_for(100ms);   // ❌ INTERDIT !
    printf("Long message...");      // ❌ Éviter (lent)
}
```

### Carte SD

⚠️ **Toujours vérifier** FLAG_READY avant opérations
```cpp
threadSD.waitReady();  // Bloque jusqu'à prêt
int status = threadSD.status();
if (status & ThreadSD::FLAG_READY) {
    // Opérations fichiers sûres
}
```

---

**Document créé le** : 27 décembre 2025
**Dernière mise à jour** : 27 décembre 2025
**Version projet** : v0.1
