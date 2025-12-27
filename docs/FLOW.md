# 🔄 FLUX D'EXÉCUTION - Écran Stratégie V4

> Diagrammes de flux détaillés du système

---

## Table des matières

1. [Séquence de démarrage](#séquence-de-démarrage)
2. [Machine d'états match](#machine-détats-match)
3. [Flux interface utilisateur](#flux-interface-utilisateur)
4. [Communication CAN](#communication-can)
5. [Test ventouses](#test-ventouses)

---

## Séquence de démarrage

### Initialisation système (main.cpp)

```
DÉBUT
  │
  ├─► Initialisation périphériques
  │    ├─► Serial USB (921600 baud)
  │    ├─► LEDs (PG_6, PD_4, PD_5)
  │    └─► Jack pin (D7)
  │
  ├─► Création instances threading
  │    ├─► ThreadCAN threadCAN;
  │    ├─► ThreadSD threadSD;
  │    ├─► ThreadLvgl threadLvgl;
  │    └─► ThreadSound (singleton)
  │
  ├─► Création instances contrôle
  │    ├─► Ihm ihm(&threadLvgl);
  │    ├─► Deplacement deplacement(threadCAN);
  │    └─► Herkulex herkulex(threadCAN);
  │
  ├─► Enregistrements CAN
  │    ├─► threadSD.registerCANControl(threadCAN)
  │    └─► threadCAN.registerIds(0x01, 0x7FF, canProcessRx)
  │
  ├─► Attente carte SD (timeout 60s)
  │    ├─► if (FLAG_NO_CARD) → Attendre
  │    ├─► if (timeout) → NVIC_SystemReset()
  │    └─► if (FLAG_READY) → Continuer
  │
  ├─► Chargement configuration
  │    ├─► readConfig() → /sd/config.ini
  │    └─► Populate global config map
  │
  ├─► Listing fichiers stratégie
  │    ├─► listeFichiers() → /sd/strategie/*.txt
  │    └─► Remplir vector<string> fichiers
  │
  ├─► Initialisation interface
  │    ├─► ihm.show(fichiers)        # Onglet Match
  │    ├─► ihm.ActionneurInit()      # Onglet Actionneur
  │    └─► ihm.testTabInit()         # Onglet Test
  │
  ├─► Mise à jour statut SD
  │    └─► ihm.updateCarteSd2Status(true, nb_fichiers)
  │
  ├─► Démarrage Ticker SD (2s)
  │    └─► Mise à jour périodique statut carte
  │
  └─► BOUCLE PRINCIPALE (état = multi_init)
       └─► while (1) { ... }
```

---

## Machine d'états match

### États principaux

```
┌─────────────────────────────────────────────────────────┐
│ MACHINE D'ÉTATS STRATÉGIE (machineStrategie())          │
└─────────────────────────────────────────────────────────┘

[ETAT_GAME_INIT]
  │ Initialisation variables
  │ Chargement première instruction
  │
  ↓
[ETAT_GAME_RECALAGE] ← optionnel
  │ Recalage bordures X/Y/Theta
  │ Précision positionnement robot
  │
  ↓
[ETAT_GAME_WAIT_FOR_JACK]
  │ Affichage "Tirer le jack pour démarrer"
  │ Attendre jack() == false
  │
  ↓
[ETAT_GAME_START]
  │ Démarrage timer match 100s
  │ timerMatch.attach(forceFinMatch, 100s)
  │
  ↓
[ETAT_GAME_LOAD_NEXT_INSTRUCTION] ◄──────────┐
  │ listeInstructions.suivante()              │
  │ Si plus d'instructions → END_LOOP         │
  │                                           │
  ↓                                           │
[ETAT_GAME_PROCESS_INSTRUCTION]               │
  │ switch (instruction.type) {               │
  │   case MV_XYT:                            │
  │     deplacement.positionXYTheta(...)      │
  │   case MV_LINE:                           │
  │     deplacement.toutDroit(...)            │
  │   case MV_TURN:                           │
  │     deplacement.rotation(...)             │
  │   case ACTION:                            │
  │     herkulex.controleActionneur(...)      │
  │   ...                                     │
  │ }                                         │
  │                                           │
  ↓                                           │
[ETAT_GAME_WAIT_ACK]                          │
  │ Attendre ACK CAN de la carte concernée    │
  │ flag.wait_any(ACKFrom_FLAG)               │
  │ if (obstacle détecté) → OBSTACLE          │
  │                                           │
  ↓                                           │
[ETAT_GAME_CHECK_NEXT]                        │
  │ Vérifier next action de l'instruction     │
  │ if (next == CONTINUE) ───────────────────┘
  │ if (next == WAIT) → attendre délai
  │ if (next == END) → END_LOOP
  │
  ↓
[ETAT_GAME_OBSTACLE] ← si lidar détecte
  │ evitement.traiterObstacle()
  │ Esquive ou attente disparition
  │ Retour à PROCESS ou LOAD_NEXT
  │
  ↓
[ETAT_END_LOOP]
  │ Arrêt moteurs
  │ Affichage score final
  │ NVIC_SystemReset() après délai
  │
  └─► FIN
```

### Timeline match

```
t=0s    : Retrait jack
        ├─► Démarrage timer 100s
        └─► Première instruction

t=0-100s: Exécution séquentielle instructions
        ├─► MV_* : Mouvements robot
        ├─► ACTION : Actionneurs
        └─► Attente ACK entre chaque

t=100s  : forceFinMatch() appelé
        ├─► flagForceFinMatch = true
        ├─► État → END_LOOP
        └─► Arrêt robot

t=103s  : Reset système
        └─► NVIC_SystemReset()
```

---

## Flux interface utilisateur

### Boucle principale (main.cpp)

```
État: multi_init
  │
  ├─► if (ihm.departClicked())
  │    ├─► choix = ihm.choixStrategie()
  │    ├─► lectureFichier(choix)
  │    ├─► ihm.msgBoxmatchshow(fichiers[choix])
  │    ├─► Créer thread: show = new Thread(run_show)
  │    └─► État → show_run_page
  │
  ├─► if (ihm.Test_ventouse())
  │    ├─► ihm.showButtonascenceurBox()
  │    ├─► Attendre sélection 5s
  │    ├─► selasc = selected_ascenceur
  │    ├─► ihm.showButtonSelectionBox()
  │    ├─► Attendre sélection 5s
  │    ├─► choix_niveau = selected_level
  │    └─► État → test
  │
  ├─► if (ihm.construction_niveaux_2())
  │    └─► [Même flux que Test_ventouse]
  │
  ├─► if (ihm.Niveaux_base())
  │    └─► [Même flux que Test_ventouse]
  │
  ├─► if (ihm.Position_init())
  │    └─► [Même flux que Test_ventouse]
  │
  ├─► if (ihm.lacherflag())
  │    └─► [Même flux que Test_ventouse]
  │
  └─► if (ihm.autretest())
       ├─► if (flag_test_ventouses_clicked)
       │    └─► État → test_ventouse_position
       └─► else
            └─► threadCAN.sendAck(TEST_BRAS_1, 8)

État: show_run_page
  │
  ├─► run_show() lancé en thread
  │    ├─► color = hyou (de ihm.choixCouleur())
  │    ├─► gameEtat = ETAT_GAME_LOAD_NEXT_INSTRUCTION
  │    └─► while (machineStrategie()) { ... }
  │
  └─► État → multi_init

État: test
  │
  ├─► ihm.msgBoxInit("Test en cours", ...)
  ├─► threadCAN.sendAck(idprisencompte, choix_niveau)
  ├─► Attendre 3s
  ├─► ihm.msgBoxClose()
  └─► État → multi_init
```

---

## Communication CAN

### Flux envoi message

```
Émetteur (deplacement, herkulex, ...)
  │
  ├─► threadCAN.send(CANMessage msg)
  │    │
  │    ├─► Mail write_queue
  │    │    └─► Ajouter message au buffer
  │    │
  │    └─► Thread CAN write (priorité AboveNormal)
  │         ├─► msg = write_queue.get()
  │         ├─► can.write(msg)
  │         └─► Boucle
  │
  └─► waitingAckID = msg.id
       └─► flags.wait_any(ACKFrom_FLAG)
```

### Flux réception message

```
Bus CAN
  │
  ├─► Interruption CAN_RX
  │    │
  │    └─► Thread CAN read (priorité High)
  │         ├─► can.read(msg)
  │         ├─► read_queue.put(msg)
  │         └─► Boucle
  │
  └─► Thread CAN dispatch (priorité High)
       ├─► msg = read_queue.get()
       │
       ├─► Recherche callback pour msg.id
       │    └─► for (auto& reg : m_ids)
       │         if (msg.id >= reg.idMin && msg.id <= reg.idMax)
       │              reg.callback(&msg)
       │
       └─► canProcessRx(&msg) appelé
            ├─► strategie.canProcessRx()
            │    ├─► Traiter ACK
            │    ├─► Traiter odométrie
            │    └─► flags.set(ACKFrom_FLAG)
            │
            ├─► evitement.trameCan()
            │    └─► Traiter données lidar
            │
            └─► Mettre à jour global vars
                 ├─► x_robot, y_robot, theta_robot
                 └─► SCORE_*
```

### Callbacks enregistrés

```
ID Range          | Callback                | Traitement
──────────────────┼─────────────────────────┼───────────────────
0x001-0x7FF       | canProcessRx (global)   | Tous messages
0x3F0-0x3F3       | threadSD callback       | Commandes SD CAN
```

---

## Test ventouses

### Flux cascade (nouveau v0.1)

```
Onglet Test
  │
  ├─► Clic "Test Ventouses"
  │    ├─► flag_test_ventouses_clicked = true
  │    ├─► ihm.flags.set(IHM_FLAG__autre)
  │    └─► État → test_ventouse_position
  │
  ↓
test_ventouse_position
  │
  ├─► ihm.showVentousePositionBox()
  │    ├─► Message box avec boutons:
  │    │    [ Gauche ] [ Droite ]
  │    │    [ Les deux ] [ Annuler ]
  │    └─► Titre: "Position Ventouses" (bleu, 40pt)
  │
  ├─► Attendre 5s
  │    └─► Utilisateur clique ou timeout
  │
  ├─► ihm.showVentousePositionBoxClose()
  │
  ├─► if (selected_ventouse_position == 4 || == 0)
  │    └─► État → multi_init  # Annuler/Timeout
  │
  └─► else if (selected_ventouse_position >= 1 && <= 3)
       └─► État → test_ventouse_numero
            │
            ↓
test_ventouse_numero
  │
  ├─► ihm.showVentouseNumeroBox()
  │    ├─► Message box avec boutons:
  │    │    [ V1 ] [ V2 ] [ V3 ]
  │    │    [ V4 ] [ Les 4 ] [ Annuler ]
  │    └─► Titre: "Numero Ventouse" (vert, 40pt)
  │
  ├─► Attendre 5s
  │
  ├─► ihm.showVentouseNumeroBoxClose()
  │
  ├─► if (selected_ventouse_numero == 6 || == 0)
  │    └─► État → multi_init  # Annuler/Timeout
  │
  └─► else if (selected_ventouse_numero >= 1 && <= 5)
       └─► État → test_ventouse_action
            │
            ↓
test_ventouse_action
  │
  ├─► ihm.showVentouseActionBox()
  │    ├─► Message box avec boutons:
  │    │    [ Attraper ] [ Lâcher ]
  │    │    [ Annuler ]
  │    └─► Titre: "Action Ventouse" (orange, 40pt)
  │
  ├─► Attendre 5s
  │
  ├─► ihm.showVentouseActionBoxClose()
  │
  ├─► if (selected_ventouse_action == 3 || == 0)
  │    └─► État → multi_init  # Annuler/Timeout
  │
  └─► else if (selected_ventouse_action >= 1 && <= 2)
       │
       ├─► printf() résultat sélections
       │    ├─► Position: 1/2/3
       │    ├─► Numéro: 1/2/3/4/5
       │    └─► Action: 1/2
       │
       ├─► TODO: threadCAN.sendAck(ID_VENTOUSE, valeur)
       │
       └─► État → multi_init
```

### Variables globales test ventouses

```cpp
// Déclarées dans ihm.cpp, exportées dans ihm.h
int selected_ventouse_position = 0;  // 1=Gauche, 2=Droite, 3=Les deux, 4=Annuler
int selected_ventouse_numero = 0;    // 1-4=V1-V4, 5=Les 4, 6=Annuler
int selected_ventouse_action = 0;    // 1=Attraper, 2=Lâcher, 3=Annuler
bool flag_test_ventouses_clicked = false;
```

### Gestionnaires événements

```
ventouse_position_event_handler()
  ├─► Récupère texte bouton cliqué
  ├─► if ("Gauche") → selected_ventouse_position = 1
  ├─► if ("Droite") → selected_ventouse_position = 2
  ├─► if ("Les deux") → selected_ventouse_position = 3
  └─► if ("Annuler") → selected_ventouse_position = 4

ventouse_numero_event_handler()
  ├─► Récupère texte bouton cliqué
  ├─► if ("V1") → selected_ventouse_numero = 1
  ├─► if ("V2") → selected_ventouse_numero = 2
  ├─► if ("V3") → selected_ventouse_numero = 3
  ├─► if ("V4") → selected_ventouse_numero = 4
  ├─► if ("Les 4") → selected_ventouse_numero = 5
  └─► if ("Annuler") → selected_ventouse_numero = 6

ventouse_action_event_handler()
  ├─► Récupère texte bouton cliqué
  ├─► if ("Attraper") → selected_ventouse_action = 1
  ├─► if ("Lâcher") → selected_ventouse_action = 2
  └─► if ("Annuler") → selected_ventouse_action = 3
```

---

## Séquences critiques

### Séquence démarrage match

```
1. Utilisateur sélectionne stratégie (Onglet Match)
   └─► ihm.roller (fichier), ihm.couleur (jaune/bleu)

2. Utilisateur clique "GO"
   └─► ihm.flags.set(IHM_FLAG_DEPART)

3. main.cpp détecte departClicked()
   ├─► Lecture fichier stratégie
   ├─► Parsing instructions → listeInstructions
   └─► Lancement thread run_show()

4. run_show() démarre
   ├─► gameEtat = ETAT_GAME_LOAD_NEXT_INSTRUCTION
   └─► while (machineStrategie()) { ... }

5. Machine stratégie : LOAD → PROCESS → WAIT_ACK → LOAD ...
   └─► Exécution séquentielle jusqu'à END_LOOP

6. Fin match (100s ou fin instructions)
   ├─► Arrêt moteurs
   ├─► Affichage score
   └─► Reset après 3s
```

### Séquence envoi commande CAN

```
1. Code appelle deplacement.positionXYTheta(1000, 500, 1.57)

2. deplacement.cpp
   ├─► Prépare CANMessage
   │    └─► msg.id = ASSERVISSEMENT_XYT (0x020)
   ├─► threadCAN.send(msg)
   └─► Stocke waitingAckID = 0x020

3. threadCAN.send()
   └─► Mail write_queue.put(msg)

4. Thread CAN write
   ├─► msg = write_queue.get()
   └─► can.write(msg) → Bus CAN

5. Carte moteur reçoit, exécute, envoie ACK
   └─► ACK (id=0x101, data[0]=0x020)

6. Thread CAN read
   ├─► can.read(ack_msg)
   └─► Mail read_queue.put(ack_msg)

7. Thread CAN dispatch
   ├─► ack_msg = read_queue.get()
   └─► canProcessRx(&ack_msg)

8. canProcessRx()
   ├─► Vérifie if (ack_msg.id == ACK_MOTEUR)
   ├─► Vérifie if (ack_msg.data[0] == waitingAckID)
   └─> flags.set(ACKFrom_FLAG)

9. deplacement.cpp débloqué
   ├─► flags.wait_any(ACKFrom_FLAG) retourne
   └─► Fonction retourne au appelant
```

---

**Document créé le** : 27 décembre 2025
**Dernière mise à jour** : 27 décembre 2025
**Version projet** : v0.1
