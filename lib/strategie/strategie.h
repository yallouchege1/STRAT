#ifndef CRAC_STRATEGIE
#define CRAC_STRATEGIE

#include "mbed.h"
#include "threadSD.h"
#include <instruction.h>

typedef enum
{
    AckFrom_FLAG     =   (1UL << 0),
    AckFrom_FIN_FLAG =   (1UL << 1),
    JACK             =   (1UL << 2),
}ACKFlags;

typedef enum
{
    RECALAGE_1,
    RECULER_1,
    TOURNER,
    RECALAGE_2,
    RECULER_2,
    GOTOPOS,
    FIN_POS,
}E_Stratposdebut;
    
typedef enum {
    INIT,
    ATT,
    CHOIX,
    DEMO,
    DEMO2,
    TEST_MOTEUR, 
    TEST_COULEUR, 
    TEST_SERVO_BRAS, 
    TEST_VENTOUSE, 
    TEST_ELECTROV,
    TEST_AUD, 
    TEST_DIVE,
    TEST_ASSERVE,
    TEST_MANCHES,
    TEST_GIROUS,
    SELECT_SIDE, 
    SELECT_ROB,
    TACTIQUE, 
    DETAILS,
    LECTURE, 
    LAUNCH, 
    AFF_WAIT_JACK, 
    WAIT_JACK,
    FIN} T_etat;
    
typedef enum
{
    ETAT_DOING_NOTHING,
    ETAT_CHECK_CARTE_SCREEN, //Envoie check carte screen 
    ETAT_CHECK_CARTE_SCREEN_WAIT_ACK, //Time out de 1s si erreur clignotement des led et fin prog 
    ETAT_CHECK_CARTES, //Envoie check toutes les carte 
    ETAT_CHECK_CARTES_WAIT_ACK, //Time out de 1s 
    ETAT_WAIT_FORCE,//Attente du forçage du lancement 
    ETAT_CONFIG, //attente reception du choix du mode( debug ou game) 
    ETAT_GAME_INIT,//Mise en mémoire du fichier de stratégie 
    ETAT_GAME_RECALAGE,
    ETAT_GAME_WAIT_FOR_JACK,//Attente du retrait du jack
    ETAT_GAME_START,//Lancement du timer 100s
    ETAT_GAME_LOAD_NEXT_INSTRUCTION, // 
    ETAT_GAME_PROCESInstruction, // 
    ETAT_GAME_WAIT_ACK, // 
    ETAT_GAME_JUMP_TIME,
    ETAT_GAME_JUMP_CONFIG,
    ETAT_GAME_JUMP_POSITION,
    ETAT_GAME_WAIT_END_INSTRUCTION,
    ETAT_GAME_MVT_DANGER,
    ETAT_GAME_INSTRUCTION_FINIE,
    ETAT_GAME_OBSTACLE,
    ETAT_END,
    ETAT_END_LOOP,
} E_stratGameEtat;
extern volatile E_stratGameEtat gameEtat;
extern T_etat strat_etat_s;
// extern Timer gameTimer;

#define ROBOT_A_GAUCHE 0
#define ROBOT_A_DROITE 1
#define ROBOT_EN_HAUT 0
#define ROBOT_EN_BAS 1



void canProcessRx(CANMessage *rxMsg);
void Strategie(void);

// short recalageAngulaireCapteur(void);
// short recalageDistanceX(void);
// short recalageDistanceY(void);
// void isr_end_danger(void);

void printCANMsg(CANMessage& msg);
void remplirStruct(CANMessage &theDATA, int idf, char lenf, char dt0f, char dt1f, char dt2f, char dt3f, char dt4f, char dt5f, char dt6f, char dt7f);

bool machineStrategie();
bool machineRecalageInit();
bool machineRecalage();
void procesInstructions(Instruction instruction);

string AckToString(int id);
bool getFlag(ACKFlags f, bool clearIfSet);
#endif
