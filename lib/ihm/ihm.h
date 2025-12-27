#ifndef __IHM_H
#define __IHM_H

#include "mbed.h"
#include "lvgl.h"
#include <threadLvgl.h>
#include <vector>
#include <threadCAN.h>


#define FONT_NORMAL &liberation_24
#define FONT_LARGE &liberation_40

// Variables globales pour test ventouses
extern int selected_ventouse_position;
extern int selected_ventouse_numero;
extern int selected_ventouse_action;
extern bool flag_test_ventouses_clicked;

class Ihm
{
protected:
    typedef enum
    {
        IHM_FLAG_DEPART =                   (1UL << 0),
        IHM_FLAG_REFRESH_SD =               (1UL << 1),
        IHM_FLAG_RECALAGE =                 (1UL << 2),
        IHM_FLAG_START =                    (1UL << 3),
        IHM_FLAG_START_CANCEL =             (1UL << 4),
        IHM_FLAG_MSGBOX_CANCEL =            (1UL << 5),
        IHM_FLAG_RECALAGE_HAUTGAUCHE =      (1UL << 6),
        IHM_FLAG_RECALAGE_HAUTDROIT =       (1UL << 7),
        IHM_FLAG_RECALAGE_BASGAUCHE =       (1UL << 8),
        IHM_FLAG_RECALAGE_BASDROIT =        (1UL << 9),
        IHM_FLAG_RECALAGE_ETAT =            (1UL << 10),
        IHM_FLAG_PLAY =                     (1UL << 11),
        IHM_FLAG_STOP =                     (1UL << 12),
        IHM_FLAG_SAVE_CONFIG =              (1UL << 13),
        IHM_FLAG_RESET =                    (1UL << 14),
        IHM_FLAG_VOLUME =                   (1UL << 15),


        IHM_FLAG_Lacher =   (1UL << 16),

        IHM_FLAG_Testventouse= (1UL << 17),
            IHM_FLAG_Testventouse_avant = (1UL << 28),
            IHM_FLAG_Testventouse_arriere= (1UL <<29),

        IHM_FLAG_Gradin_niveaux_2 =   (1UL << 18),
            IHM_FLAG_Gradin_niveaux_2_avant =   (1UL << 24),
            IHM_FLAG_Gradin_niveaux_2_arriere =   (1UL << 25),

        IHM_FLAG_Niveaux_base =    (1UL << 19),
        IHM_FLAG_Niveaux_base_avant  =   (1UL << 26),
        IHM_FLAG_Niveaux_base_arriere =   (1UL << 27),
        IHM_FLAG__choix_niv = (1UL << 31) , 

       IHM_FLAG__Position_init =       (1UL << 20),

        IHM_FLAG_RECALAGE_HAUTMILIEU =      (1UL << 21),
        IHM_FLAG_ACTIONNEUR_ASSERV_ON =     (1UL << 22),
        IHM_FLAG_ACTIONNEUR_ASSERV_OFF =    (1UL << 23),
        IHM_FLAG__autre= (1UL << 30 )  ,

    } IhmFlag;
    EventFlags flags;
    ThreadLvgl *m_threadLvgl;
    lv_style_t styleTitre;
    lv_obj_t *tabView;
    // Onglet "match"
    lv_obj_t *tabMatch;
    lv_obj_t *roller;
    lv_obj_t *couleur;
    lv_obj_t *depart;
    int departCouleur;
    int departStrategie;
    // Onglet "config"
    lv_obj_t *tabConfig;
    lv_obj_t *configRoller;
    lv_obj_t *configPlay;
    lv_obj_t *labelPlay;
    lv_obj_t *configVolume;
    lv_obj_t *labelVolume;
    lv_obj_t *configSave;
    lv_obj_t *configReset;
    //Onglet "test actionneur"
    lv_obj_t *tabActionneur;
    lv_obj_t *Pos_init;

    //Onglet "Test"
    lv_obj_t *tabTest;
    lv_obj_t *btnTestVentouses;

    lv_obj_t *testventouse;
    lv_obj_t *testventouse_avant ;
    lv_obj_t *testventouse_arriere ;

    lv_obj_t *Gradinniveaux2;
        lv_obj_t * Gradinniveaux2_avant ; 
        lv_obj_t * Gradinniveaux2_arriere ; 

    lv_obj_t *niveaux_base ;
        lv_obj_t *niveauxbase_avant ;
        lv_obj_t *niveauxbase_arriere ; 

    lv_obj_t * lacher ;
    lv_obj_t * autre ;

    // Test Ventouses - Navigation 3 niveaux
    lv_obj_t *btnTestVentouses;          // Bouton principal dans ActionneurInit
    lv_obj_t *ventousesContainer;        // Container pour les menus de navigation

    // Onglet "CarteSD"
    lv_obj_t *tabCarteSD;
    lv_obj_t *spinnerCarteSD;
    lv_obj_t *labelCarteSDStatus;
    lv_obj_t *labelCarteSDFileCount;

    int volume;
    int mp3;

    // Message Box recalage
    lv_obj_t *msgBoxRecalage;
    lv_obj_t * msgBoxmatchmessage ; 
    // Message Box jack
    lv_obj_t *msgBoxJack;
    // Message Box générique
    lv_obj_t *msgBox;
    lv_obj_t *msgBoxChoixNIV ;

    void carteSDInit(lv_obj_t *parent);
    static void eventHandler(lv_event_t *e);
    bool getFlag(IhmFlag f, bool clearIfSet = true);
    

public:
    // Variables publiques pour le système de test ventouses (accessibles par callbacks)
    int ventousesCote;                   // 0=gauche, 1=droite, 2=les_deux
    int ventousesNumero;                 // 1/2/3/4
    int ventousesAction;                 // 0=attraper, 1=lacher, 2=les_deux

    Ihm(ThreadLvgl *t);
    void show(const vector<string> fichiers);
    void matchRollerSetOptions(const vector<string> fichiers, bool lock = true);
    void recalagePositionInit();

    bool Test_ventouse(bool clearIfSet = true) { return getFlag(  IHM_FLAG_Testventouse, clearIfSet); }
         bool Test_ventouse_avant(bool clearIfSet = true) { return getFlag(  IHM_FLAG_Testventouse_avant, clearIfSet); }
          bool Test_ventouse_arriere(bool clearIfSet = true) { return getFlag(  IHM_FLAG_Testventouse_arriere, clearIfSet); }

    bool construction_niveaux_2(bool clearIfSet = true) { return getFlag(IHM_FLAG_Gradin_niveaux_2, clearIfSet); }
        bool construction_niveaux_2_avant(bool clearIfSet = true) { return getFlag(IHM_FLAG_Gradin_niveaux_2_avant, clearIfSet); }
        bool construction_niveaux_2_arriere(bool clearIfSet = true) { return getFlag(IHM_FLAG_Gradin_niveaux_2_arriere, clearIfSet); }
    
    bool Niveaux_base(bool clearIfSet = true) { return getFlag(   IHM_FLAG_Niveaux_base, clearIfSet); }
        bool Niveaux_base_avant(bool clearIfSet = true) { return getFlag(   IHM_FLAG_Niveaux_base_avant, clearIfSet); }
        bool Niveaux_base_arriere(bool clearIfSet = true) { return getFlag(   IHM_FLAG_Niveaux_base_arriere, clearIfSet); }
     bool choix (bool clearIfSet = true) { return getFlag(IHM_FLAG__choix_niv , clearIfSet); }
    

     bool lacherflag(bool clearIfSet = true) { return getFlag(IHM_FLAG_Lacher, clearIfSet); }



   bool Position_init(bool clearIfSet = true) { return getFlag(IHM_FLAG__Position_init , clearIfSet);}
 bool departClicked(bool clearIfSet = true) { return getFlag(IHM_FLAG_DEPART, clearIfSet); }
bool autretest(bool clearIfSet = true) { return getFlag(IHM_FLAG__autre, clearIfSet); }
    bool testVentousesClicked(bool clearIfSet = true) { return getFlag(IHM_FLAG__autre, clearIfSet); }


    bool refreshSDClicked(bool clearIfSet = true) { return getFlag(IHM_FLAG_REFRESH_SD, clearIfSet); }
    bool recalageClicked(bool clearIfSet = true) { return getFlag(IHM_FLAG_RECALAGE, clearIfSet); }
    bool jackSimuleClicked(bool clearIfSet = true) { return getFlag(IHM_FLAG_START, clearIfSet); }
    bool jackAnnuleClicked(bool clearIfSet = true) { return getFlag(IHM_FLAG_START_CANCEL, clearIfSet); }
    bool msgBoxCancelClicked(bool clearIfSet = true) { return getFlag(IHM_FLAG_MSGBOX_CANCEL, clearIfSet); }
    bool recalageHautGaucheClicked(bool clearIfSet = true) { return getFlag(IHM_FLAG_RECALAGE_HAUTGAUCHE, clearIfSet); }
    bool recalageBasGaucheClicked(bool clearIfSet = true) { return getFlag(IHM_FLAG_RECALAGE_BASGAUCHE, clearIfSet); }
    bool recalageHautDroitClicked(bool clearIfSet = true) { return getFlag(IHM_FLAG_RECALAGE_HAUTDROIT, clearIfSet); }
    bool recalageBasDroitClicked(bool clearIfSet = true) { return getFlag(IHM_FLAG_RECALAGE_BASDROIT, clearIfSet); }
    bool recalageMilieuHautClicked(bool clearIfSet = true) { return getFlag(IHM_FLAG_RECALAGE_HAUTMILIEU, clearIfSet);}
    
    bool activationRecalageClicked(bool clearIfSet = true) { return getFlag(IHM_FLAG_RECALAGE_ETAT, clearIfSet); }
    bool playClicked(bool clearIfSet = true) { return getFlag(IHM_FLAG_PLAY, clearIfSet); }
    bool stopClicked(bool clearIfSet = true) { return getFlag(IHM_FLAG_STOP, clearIfSet); }
    bool saveConfigClicked(bool clearIfSet = true) { return getFlag(IHM_FLAG_SAVE_CONFIG, clearIfSet); }
    bool resetClicked(bool clearIfSet = true) { return getFlag(IHM_FLAG_RESET, clearIfSet); }
    bool volumeChanged(bool clearIfSet = true) { return getFlag(IHM_FLAG_VOLUME, clearIfSet); }
   // bool actionneurPoseCeriseClicked(bool clearIfSet = true) { return getFlag(IHM_FLAG_ACTIONNEUR_POSE_CERISE, clearIfSet); }
    
    
    bool asservOnClicked(bool clearIfSet = true) { return getFlag(IHM_FLAG_ACTIONNEUR_ASSERV_ON, clearIfSet); }
    bool asservOffClicked(bool clearIfSet = true) { return getFlag(IHM_FLAG_ACTIONNEUR_ASSERV_OFF, clearIfSet); }
    int choixStrategie() { return departStrategie; }
    int choixCouleur() { return departCouleur; }
    int choixVolume() { return volume; }
    int choixMp3() { return mp3; }
    void msgBoxRecalageInit(const string &strategie);
    void msgBoxRecalageClose();
    void msgBoxJackInit();
    void msgBoxJackClose();
    void msgBoxInit(const char *titre, const char *msg, bool boutonAnnuler);
    void msgBoxMessage(const char *msg);
    void msgBoxClose();
    void configInit(const vector<string> fichiers, int v);
    void ActionneurInit();
    void configRollerSetOptions(const vector<string> fichiers, bool lock = true);
    void configStopPlaying();
    void showButtonSelectionBox() ; 
    void showButtonSelectionBoxClose() ; 
    void showButtonascenceurBox() ;
    void showButtonascenceurBoxClose() ;
    void msgBoxmatchshow (const string &strategie) ;
    void msgBoxmatchshowclose ()  ;
    void testTabInit();
    void showVentousePositionBox();
    void showVentousePositionBoxClose();
    void showVentouseNumeroBox();
    void showVentouseNumeroBoxClose();
    void showVentouseActionBox();
    void showVentouseActionBoxClose();

    // Test Ventouses - Méthodes de navigation
    void showVentousesNiveau1();         // Affiche: Gauche/Droite/Les deux
    void showVentousesNiveau2(int cote); // Affiche: Ventouse 1/2/3/4/Annuler
    void showVentousesNiveau3(int cote, int numero); // Affiche: Attraper/Lâcher/Les deux/Annuler
    void executeVentouse(int cote, int numero, int action); // Fonction finale d'exécution
    void closeVentousesMenu();           // Ferme le menu et retourne à ActionneurInit

    // CarteSD methods
    void updateCarteSDStatus(bool detected, int fileCount);


};


#endif
