#ifndef GLOBAL_H
#define GLOBAL_H

#include "mbed.h"

// #include <debug.h>

#include <CAN.h>
#include <threadCAN.h>
#include <threadSD.h>
#include <threadLvgl.h>
#include <deplacement.h>
#include <identCrac.h>
#include <instruction.h>
//#include "Strategie.h"


// ****************************************************************************************
// * CONSTANTES SYMBOLIQUES                                                               *
// ****************************************************************************************



#define SIZE_FIFO               50 //Taille du buffer pour le bus CAN

#define SIZE                    750 //Taille d'une ligne du fichier


/****
** Variable à modifier en fonction du robot
***/

//-------------------------------------------------------------------------
#define NOMBRE_CARTES           2 //Le nombre de carte présente sur le petit robot
#define POSITION_DEBUT_X 300
#define POSITION_DEBUT_Y 300
#define POSITION_DEBUT_T 0
#define MOITIEE_ROBOT 118

#define M_PI 3.14159265358979323846
// extern E_stratGameEtat gameEtat = ETAT_GAME_INIT;
//extern T_etat strat_etat_s = INIT;

// extern CANMessage msgRxBuffer[SIZE_FIFO];
// extern unsigned char FIFO_ecriture;
extern ThreadCAN threadCAN;
extern Deplacement deplacement;

extern signed short x_robot;
extern signed short y_robot;
extern signed short theta_robot;
extern signed short target_x_robot, target_y_robot, target_theta_robot, target_sens;
extern signed char asser_stop_direction;
extern short direction;
extern signed char nbStrat;
extern unsigned short flag_check_carte;
extern unsigned char Cote;
extern unsigned char Hauteur;
// extern DigitalOut led1,led2,led3,led4;

extern short SCORE_GLOBAL;
extern short SCORE_PR, SCORE_GR;

extern EventFlags flag;

extern Assiette assiette_choisie;
extern couleurDepart color;
extern EnumInstructionType actionPrecedente;

extern unsigned char InversStrat;//Si à 1, indique que l'on part de l'autre cote de la table(inversion des Y)

extern unsigned short waitingAckID;//L'id du ack attendu
extern unsigned short waitingAckFrom;//La provenance du ack attendu
extern int waitingAckID_FIN;
extern int waitingAckFrom_FIN;
extern unsigned short waitingId;

extern unsigned char isStopEnable;


#endif
