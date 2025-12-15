#include <global.h>
#include <strategie.h>
#include <instruction.h>
#include <herkulex.h>
#include <debug.h>
#include <evitement.h>
#include <ihm.h>
#include "mbed.h"

extern Herkulex herkulex;
extern int recalageErreur;  // 0 si recalage réussi, valeur négative sinon

struct S_Dodge_queue dodgeq;

unsigned char InversStrat = 1;                              // Si à 1, indique que l'on part de l'autre cote de la table(inversion des Y)
unsigned short waitingAckID = 0;                            // L'id du ack attendu
unsigned short waitingAckFrom = 0;                          // La provenance du ack attendu
int waitingAckID_FIN;
int waitingAckFrom_FIN;
unsigned short waitingId = 0;
volatile E_stratGameEtat gameEtat = ETAT_GAME_INIT;
EventFlags flag;
couleurDepart color ; 
int blackdoves = 0 ; 


// // E_stratGameEtat gameEtat = ETAT_CHECK_CARTES;
// T_etat strat_etat_s = INIT;


    
// int waitingAckID_FIN;
// int waitingAckFrom_FIN;

// Ticker ticker;


// Ticker chrono;
// Timeout AffTime;
// Ticker timer_strat;
// Timer cartesCheker;//Le timer pour le timeout de la vérification des cartes
// Timer gameTimer;
// Timer debugetatTimer;
// Timer timeoutWarning;
// Timer timeoutWarningWaitEnd;
// Timeout chronoEnd;//permet d'envoyer la trame CAN pour la fin

// unsigned char screenChecktry = 0;
// unsigned char test[32] = {32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32};

signed char asser_stop_direction=0;
// char counter = 0;
// char check;
// char Jack = 1;
short SCORE_GLOBAL=18;
// short SCORE_GR=0;
// short SCORE_PR=0;
// unsigned short distance_recalage;
// unsigned short distance_revenir;

// unsigned short x;
// unsigned short y;
// unsigned char isStopEnable = 1;//Permet de savoir si il faut autoriser le stop via les balises
unsigned short flag_check_carte = 0; //, flag_strat = 0, flag_timer;
// int flagReceptionTelemetres = 0, flagNonRepriseErrorMot = 0;

// int Flag_Bras_Re = 0, Flag_Manche_Bas = 0, Flag_Manche_Moy = 0, Flag_pavillon = 0, Flag_bon_port = 0; //Flag utilisé pour compter le score des bras / manches / pavillon
// unsigned short Flag_num_bras;


signed short x_robot,y_robot,theta_robot;//La position du robot, theta en dizieme de degree
signed short target_x_robot, target_y_robot, target_theta_robot, target_sens;
signed short depart_x, depart_y, depart_theta_robot;
// signed short avant_gauche, avant_droit;
EnumInstructionType actionPrecedente;
// //unsigned char FIFO_ecriture=0; //Position du fifo pour la reception CAN
// int flagSendCan=1;
unsigned char Cote = 0; //0 -> JAUNE | 1 -> VIOLET
unsigned char Hauteur = 0; //Robot du haut -> 1, du bas -> 0
// unsigned short angleRecalage = 0;
// unsigned char checkCurrent = 0;
// unsigned char countAliveCard = 0;
// unsigned char ligne=0;
// int Fevitement=0;
// int EvitEtat= 0;
// int stop_evitement=0;
// signed char nbStrat = 0; //N° de la strategie (1-10)
// unsigned char ModeDemo = 0; // Si à 1, indique que l'on est dans le mode demo
// unsigned char countRobotNear = 0;//Le nombre de robot à proximité
// unsigned char ingnorBaliseOnce = 0;//une fois détecté réinitialise
// unsigned char ingnorBalise = 0;//0:balise ignore 1:on ecoute la balise
short direction;
// char val_girou ;

// unsigned char debug_bon_port = 0;

// unsigned char ingnorInversionOnce = 0;//Pour ignorer l'inversion des instruction une fois

// // Instruction instruction;

// char couleur1, couleur2, couleur3;
// float cptf;
// int cpt,cpt1;

// E_stratGameEtat     memGameEtat= gameEtat;
// E_stratGameEtat     lastEtat  = ETAT_CHECK_CARTES;
// E_Stratposdebut etat_pos=RECALAGE_1;


//void //SendRawId (unsigned short id);
//void can2Rx_ISR(void);

// signed char blocage_balise;
// void print_segment(int nombre, int decalage);
// void affichage_compteur (int nombre);
// void effacer_segment(long couleur);
// unsigned char doAction(unsigned char id, unsigned short arg1, short arg2);
// unsigned short telemetreDistance=0;
// unsigned short telemetreDistance_avant_gauche=0;
// unsigned short telemetreDistance_avant_droite=0;
// unsigned short telemetreDistance_arriere_gauche=0;
// unsigned short telemetreDistance_arriere_droite=0;

// unsigned char DT_AVD_interrupt=0;
// unsigned char DT_AVG_interrupt=0;
// unsigned char DT_ARD_interrupt=0;
// unsigned char DT_ARG_interrupt=0;

// unsigned short id_check[NOMBRE_CARTES]= {CHECK_MOTEUR,CHECK_BALISE};
// unsigned short id_alive[NOMBRE_CARTES]= {ALIVE_MOTEUR,ALIVE_BALISE};
// InterruptIn jackB1(PG_11); //  entrée numerique en interruption pour le jack (JackB1 sur la carte esclave)
// InterruptIn jackA1(PA_6);

Instruction instruction;
Evitement evitement;
int etat_evitement = 0; bool EVITEMENT = false;
Timer timer_evitement;

/****************************************************************************************/
/* FUNCTION NAME: canProcessRx                                                          */
/* DESCRIPTION  : Fait évoluer l'automate de l'IHM en fonction des receptions sur le CAN*/
/****************************************************************************************/
void canProcessRx(CANMessage *rxMsg)
{
        int identifiant=rxMsg->id;
      
        // printCANMsg(*rxMsg);
        if (waitingId == identifiant) {waitingId = 0;}
        switch(identifiant) {
            case ALIVE_MOTEUR:{
                if(gameEtat != ETAT_GAME_INIT){
                    deplacement.setOdoPetit(x_robot, y_robot, theta_robot); //Pose probléme au debut lors du recalage du debut
                }
                printf("Base roulante a reset !! \n");

            }
                break;

            case ALIVE_BALISE:
                {
                    
                }
                break;

            case RESET_IHM:
                // {strat_etat_s = CHOIX;}
                break; 

            case DEBUG_FAKE_JAKE://Permet de lancer le match à distance
            case GLOBAL_JACK:{
                // if(gameEtat == ETAT_GAME_WAIT_FOR_JACK) {
                //     gameEtat = ETAT_GAME_START;
                //     //SendRawId(ACKNOWLEDGE_JACK);
                // }
                flag.set(JACK);
            }
                break;

            case ALIVE_ACTIONNEURS_AVANT:    //pas de break donc passe directement dans ECRAN_ALL_CHECK mais conserve l'ident initial
            //case ALIVE_ACTIONNEURS_ARRIERE:
            case ALIVE_HERKULEX:
            case ECRAN_ALL_CHECK:{
                if(waitingAckFrom == rxMsg->id) {
                    waitingAckFrom = 0;//C'est la bonne carte qui indique qu'elle est en ligne
                }
                flag_check_carte=1;
            }
                break;
            case ASSERVISSEMENT_ERROR_MOTEUR: { //erreur asservissement

                //unsigned short recieveAckID= (unsigned short)rxMsg->data[0]  | ( ((unsigned short)rxMsg->data[1]) <<8);
                //memcpy(&recieveAckID, rxMsg->data, 2);
                // if(recieveAckID == waitingAckID_FIN && waitingAckFrom_FIN == INSTRUCTION_END_MOTEUR) {
                //     if(flagNonRepriseErrorMot) {
                //         actual_instruction = instruction.nextLineError;
                //         gameEtat = ETAT_GAME_LOAD_NEXT_INSTRUCTION;
                //         flagNonRepriseErrorMot = 0;
                //     } else {
                //         flagNonRepriseErrorMot = 1;
                //         timeoutWarningWaitEnd.reset();
                //         timeoutWarningWaitEnd.start();
                //         //gameEtat = ETAT_WARING_END_BALISE_WAIT;
                //     }
                // }
                /*
                if(flagNonRepriseErrorMot) {
                    actual_instruction = instruction.nextLineError;
                    gameEtat = ETAT_GAME_LOAD_NEXT_INSTRUCTION;
                    flagNonRepriseErrorMot = 0;
                } else {
                    flagNonRepriseErrorMot = 1;
                    gameEtat = ETAT_WARNING_END_LAST_INSTRUCTION;
                }*/
            }
            break;

            /////////////////////////////////////Acknowledges de Reception de la demande d'action////////////////////////////////////////
            case ACKNOWLEDGE_HERKULEX:{

            }break;
            case ACKNOWLEDGE_BALISE:    //pas de break donc passe directement dans ACK_FIN_ACTION mais conserve l'ident initial
            {

            }break;
            case ACKNOWLEDGE_ACTIONNEURS:{
                unsigned short recieveAckID = (unsigned short)rxMsg->data[0]  | ( ((unsigned short)rxMsg->data[1]) <<8);
                if( (waitingAckFrom == identifiant )&& (recieveAckID == waitingAckID) ) 
                {
                    printf(" ack de debut recu \n");
                    waitingAckFrom = 0;
                    waitingAckID = 0;
                    flag.set(AckFrom_FLAG);
                }
                if( (waitingAckFrom_FIN == identifiant ) && (recieveAckID == waitingAckID_FIN) ) {
                    printf(" ack de fin recu \n");
                    waitingAckFrom_FIN = 0;
                    waitingAckID_FIN = 0;
                    flag.set(AckFrom_FIN_FLAG);
                }
            }break;
            /////////////////////////////////////////////Acknowledges de la fin d'action/////////////////////////////////////////////////
            case ACKNOWLEDGE_MOTEUR:{
                ////printf("ACKNOWLEDGE_MOTEUR\n");
                unsigned short recieveAckID = (unsigned short)rxMsg->data[0]  | ( ((unsigned short)rxMsg->data[1]) <<8);
                if( (waitingAckFrom == identifiant )&& (recieveAckID == waitingAckID) ) 
                {
                    ////printf(" ack de debut recu \n");
                    waitingAckFrom = 0;
                    waitingAckID = 0;
                    flag.set(AckFrom_FLAG);
                }
                if( (waitingAckFrom_FIN == identifiant ) && (recieveAckID == waitingAckID_FIN) ) {
                    //printf(" ack de fin recu \n");
                    waitingAckFrom_FIN = 0;  
                    waitingAckID_FIN = 0;
                    flag.set(AckFrom_FIN_FLAG);
                }
            }break;
            case INSTRUCTION_END_Actionneur:{
                unsigned short recieveAckID = (unsigned short)rxMsg->data[0]  | ( ((unsigned short)rxMsg->data[1]) <<8);
                if( (waitingAckFrom == identifiant )&& (recieveAckID == waitingAckID) ) 
                {
                    printf(" ack de debut recu \n");
                    waitingAckFrom = 0;
                    waitingAckID = 0;
                    flag.set(AckFrom_FLAG);
                }
                if( (waitingAckFrom_FIN == identifiant ) && (recieveAckID == waitingAckID_FIN) ) {
                    printf(" ack de fin recu \n");
                    waitingAckFrom_FIN = 0;
                    waitingAckID_FIN = 0;
                    flag.set(AckFrom_FIN_FLAG);
                }
            }break;
            case INSTRUCTION_END_ACTIONNEURS:{
                unsigned short recieveAckID = (unsigned short)rxMsg->data[0]  | ( ((unsigned short)rxMsg->data[1]) <<8);
                if( (waitingAckFrom == identifiant )&& (recieveAckID == waitingAckID) ) 
                {
                    printf(" ack de debut recu \n");
                    waitingAckFrom = 0;
                    waitingAckID = 0;
                    flag.set(AckFrom_FLAG);
                }
                if( (waitingAckFrom_FIN == identifiant ) && (recieveAckID == waitingAckID_FIN) ) {
                    printf(" ack de fin recu \n");
                    waitingAckFrom_FIN = 0;
                    waitingAckID_FIN = 0;
                    flag.set(AckFrom_FIN_FLAG);
                }
            }break;
            case ACK_FIN_ACTION:{

            }break;
            case INSTRUCTION_END_MOTEUR:
            {
                unsigned short recieveAckID = (unsigned short)rxMsg->data[0]  | ( ((unsigned short)rxMsg->data[1]) <<8);
                //printf("INSTRUCTION_END_MOTEUR\n");
                //memcpy(&recieveAckID, rxMsg->data, 2);
                /*
                //on desactive la balise dans les rotations XYT
                if(rxMsg->id==ACKNOWLEDGE_MOTEUR && ASSERVISSEMENT_XYT==recieveAckID)ingnorBalise=1;
                if(rxMsg->id==INSTRUCTION_END_MOTEUR && ASSERVISSEMENT_XYT_ROTATE==recieveAckID)ingnorBalise=0;

                //on desactive la balise dans les rotations
                if(rxMsg->id==ACKNOWLEDGE_MOTEUR && ASSERVISSEMENT_ROTATION==recieveAckID)ingnorBalise=1;
                if(rxMsg->id==INSTRUCTION_END_MOTEUR && ASSERVISSEMENT_ROTATION==recieveAckID)ingnorBalise=0;
                */

                // SendMsgCan(0x666,&ingnorBalise,1);
                if( (waitingAckFrom == identifiant )&& (recieveAckID == waitingAckID) ) 
                {
                    //printf(" ack de debut recu \n");
                    waitingAckFrom = 0;
                    waitingAckID = 0;
                    flag.set(AckFrom_FLAG);
                }
                if( (waitingAckFrom_FIN == identifiant ) && (recieveAckID == waitingAckID_FIN) ) {
                    //printf(" ack de fin recu \n");
                    waitingAckFrom_FIN = 0;
                    waitingAckID_FIN = 0;
                    flag.set(AckFrom_FIN_FLAG);
                }

                /*
                               if((waitingAckFrom == rxMsg->id) &&
                               ((unsigned short)rxMsg->data[0]  |  (((unsigned short)rxMsg->data[1])<<8) == waitingAckID)  )
                               {
                                   waitingAckFrom = 0;
                                   waitingAckID = 0;
                               }
                               if(waitingAckFrom_FIN == rxMsg->id && ((unsigned short)rxMsg->data[0]
                               |(((unsigned short)rxMsg->data[1])<<8) == waitingAckID_FIN))
                               {
                                   waitingAckFrom_FIN = 0;
                                   waitingAckID_FIN = 0;
                               }
                     */
            }
                
                break;
            case ODOMETRIE_BIG_POSITION:{
                x_robot=rxMsg->data[0]|((unsigned short)(rxMsg->data[1])<<8);
                y_robot=rxMsg->data[2]|((unsigned short)(rxMsg->data[3])<<8);
                theta_robot=rxMsg->data[4]|((signed short)(rxMsg->data[5])<<8);
            }
                break;
            case ODOMETRIE_SMALL_POSITION:{
                x_robot=rxMsg->data[0]|((unsigned short)(rxMsg->data[1])<<8);
                y_robot=rxMsg->data[2]|((unsigned short)(rxMsg->data[3])<<8);
                theta_robot=rxMsg->data[4]|((signed short)(rxMsg->data[5])<<8);
            }
                break;

            case ACK_ACTION:{
                if(waitingAckID == rxMsg->id) {
                    waitingAckFrom = 0;
                    waitingAckID = 0;
                }
            }
                break;
            case IDCAN_POS_XY_OBJET:{
                evitement.trameCan(rxMsg);

                // uint8_t id = rxMsg->data[0];
                // short x_obstacle=rxMsg->data[1]|((unsigned short)(rxMsg->data[2])<<8);
                // short y_obstacle=rxMsg->data[3]|((unsigned short)(rxMsg->data[4])<<8);
                // signed short theta_obstacle= rxMsg->data[5]|((signed short)(rxMsg->data[6])<<8);//dizieme de degree
                // // short distance_lidar = (rxMsg->data[6]|((unsigned short)(rxMsg->data[7])<<8));

                // int delta_x = x_robot - x_obstacle;
                // int delta_y = y_robot - y_obstacle;
                // int distance_lidar = sqrt((delta_x * delta_x) + (delta_y * delta_y));
                // //if(distance != distance_lidar){printf("distance != distance_lidar\n");}
                // if(distance_lidar<600 && distance_lidar > 200){
                //     printf("IDCAN_POS_XY_OBJET ; x_obstacle : %d ; y_obstacle : %d ; theta_obstacle : %d, distance_lidar : %d\n", x_obstacle, y_obstacle, theta_obstacle, distance_lidar);
                //     // if(gameEtat == ETAT_GAME_LOAD_NEXT_INSTRUCTION ||){deplacement.stop();}
                // }
                
                // switch (etat_evitement)
                // {
                // case 0:{
                //     // if(!Activation_Lidar){break;}
                //     if(instruction.lineNumber<=1){break;}
                //     if(id == 0xFF){break;}
                //     if(evitement.lidar_danger(x_obstacle, y_obstacle, theta_obstacle, distance_lidar) == DANGER_MV){
                //         printf("IDCAN_POS_XY_OBJET ;DANGER_MV ; x_obstacle : %d ; y_obstacle : %d ; theta_obstacle : %d, distance_lidar : %d\n", x_obstacle, y_obstacle, theta_obstacle, distance_lidar);
                //         EVITEMENT = true;
                //         deplacement.stop();
                //         // wait_us(5 * 1000);
                //         // deplacement.asservOn(true);
                //         // wait_us(20 * 1000);
                //         // deplacement.toutDroit(2);
                //         // flag.set(AckFrom_FLAG);
                //         // flag.set(AckFrom_FIN_FLAG);

                //         gameEtat  = ETAT_DOING_NOTHING;
                //         etat_evitement = 1;
                //         timer_evitement.start();
                //         timer_evitement.reset();

                //         threadCAN.send(BALISE_DANGER);
                //     }else if(evitement.lidar_danger(x_obstacle, y_obstacle, theta_obstacle, distance_lidar) == DANGER_ST){  // A enlever en temps de match mais à laisser pour les tests
                //         // printf("IDCAN_POS_XY_OBJET ;DANGER_ST ; x_obstacle : %d ; y_obstacle : %d ; theta_obstacle : %d, distance_lidar : %d\n", x_obstacle, y_obstacle, theta_obstacle, distance_lidar);
                //         // deplacement.stop();
                //         // wait_us(5 * 1000);
                //         // deplacement.asservOn(true);
                //         // wait_us(20 * 1000);
                //         // deplacement.toutDroit(2);
                //         // deplacement.asservOff();
                //         // wait_us(20 * 1000);
                //         // deplacement.toutDroit(1);
                //         // wait_us(20 * 1000);
                //         // deplacement.asservOn(true);
                        

                    // }
                // }break;

                // case 1:{
                //     // if((evitement.lidar_danger(x_obstacle, y_obstacle, theta_obstacle, distance_lidar) == NO_DANGER && distance_lidar<1250) || (timer_evitement.read_ms() > 3000)){
                //     //     timer_evitement.stop();
                //     //     timer_evitement.reset();

                //     //     deplacement.asservOn(true);
                //     //     gameEtat  = ETAT_GAME_PROCESInstruction;
                //     //     etat_evitement = 0; EVITEMENT = false;
                //     //     flag.clear(AckFrom_FLAG);
                //     //     flag.clear(AckFrom_FIN_FLAG);
                //     //     evitement.lidar_end_danger(&instruction, &dodgeq, target_x_robot, target_y_robot, target_theta_robot);

                //     //     threadCAN.send(BALISE_END_DANGER);

                //     // }
                //     if(timer_evitement.read_ms() > 3000){
                //         gameEtat  = ETAT_GAME_LOAD_NEXT_INSTRUCTION;
                //         deplacement.asservOn(true);
                //         etat_evitement = 0; EVITEMENT = false;
                //         flag.clear(AckFrom_FLAG);
                //         flag.clear(AckFrom_FIN_FLAG);
                //     }
                    
                // }break;
                // case 2:{
                //     if((evitement.lidar_danger(x_obstacle, y_obstacle, theta_obstacle, distance_lidar) == NO_DANGER && distance_lidar<1250) || (timer_evitement.read_ms() > 3000)){
                //         timer_evitement.stop();
                //         timer_evitement.reset();
                //         deplacement.asservOn(true);
                //         etat_evitement = 0;
                //     }
                // }break;                

                // default:
                //     break;
                // }
                

            }
            break;

            case IDCAN_SET_SCORE:{
                unsigned short score = rxMsg->data[0]|((unsigned short)(rxMsg->data[1])<<8);
                SCORE_GLOBAL = score;
            }break;

            default:
                break;
        }
}

void printCANMsg(CANMessage& msg) {
    printf("  ID      = 0x%.3x\r\n", msg.id);
    printf("  Type    = %d\r\n", msg.type);
    printf("  format  = %d\r\n", msg.format);
    printf("  Length  = %d\r\n", msg.len);
    printf("  Data    =");            
    (unsigned short)msg.data[0]  | ( ((unsigned short)msg.data[1]) <<8);
    printf("\r\n");
 }

void remplirStruct(CANMessage &theDATA, int idf, char lenf, char dt0f, char dt1f, char dt2f, char dt3f, char dt4f, char dt5f, char dt6f, char dt7f){
  theDATA.type = CANData;
  if(idf>0x7FF){theDATA.format = CANExtended;}
  else{theDATA.format = CANStandard;}
  theDATA.id = idf;
  theDATA.len = lenf;
  theDATA.data[0] = dt0f;
  theDATA.data[1] = dt1f;
  theDATA.data[2] = dt2f;
  theDATA.data[3] = dt3f;
  theDATA.data[4] = dt4f;
  theDATA.data[5] = dt5f;
  theDATA.data[6] = dt6f;
  theDATA.data[7] = dt7f;
}


bool machineStrategie() {
    //static Instruction instruction;
 
    switch (gameEtat) {
        case ETAT_GAME_LOAD_NEXT_INSTRUCTION:
            // printf("load next instruction\n");
            // if (actual_instruction >= nb_instructions || actual_instruction == 255) {
          
              
            if (listeInstructions.fin()) {
               printf("\n Fin des instruction \n") ; 
                gameEtat = ETAT_END;
                
                // Il n'y a plus d'instruction, fin du jeu
            } else {
                // instruction = strat_instructions[actual_instruction];
                
                instruction = listeInstructions.enCours();
                   
                // On effectue le traitement de l'instruction
                gameEtat = ETAT_GAME_PROCESInstruction;
            }
            break;

        case ETAT_GAME_PROCESInstruction: {
            //      Traitement de l'instruction, envoie de la trame CAN
             printf("\n Liste des instruction en cours \n\n") ; 
            debugInstruction(instruction);
            procesInstructions(instruction);
        } break;

        case ETAT_GAME_MVT_DANGER: {
           
            if (flag.wait_all(AckFrom_FIN_FLAG, 50) != osFlagsErrorTimeout) {//Si c'est égale à osFlagsErrorTimeout, alors il a timeout, sinon on a bien reçu le ack et on peut passer à la suite
                if (gameEtat != ETAT_GAME_OBSTACLE) {gameEtat = ETAT_GAME_INSTRUCTION_FINIE;}
           }
        } break;

        case ETAT_GAME_INSTRUCTION_FINIE: {
            listeInstructions.suivante();
             
            gameEtat = ETAT_GAME_LOAD_NEXT_INSTRUCTION;
        } break;

        case ETAT_GAME_OBSTACLE: {
            ThisThread::sleep_for(1s);
            deplacement.positionXYTheta(target_x_robot, target_y_robot, target_theta_robot, target_sens);
            waitingAckID = ASSERVISSEMENT_XYT;
            waitingAckFrom = ACKNOWLEDGE_MOTEUR;
            flag.wait_all(AckFrom_FLAG, 20000);
            waitingAckID_FIN = ASSERVISSEMENT_XYT;
            waitingAckFrom_FIN = INSTRUCTION_END_MOTEUR;
            gameEtat = ETAT_GAME_MVT_DANGER;
        } break;

        case ETAT_END: {
            printf("GAME ENDED\n");
            return false;
        } break;

        default:
            break;
    }
    return true;
}

void procesInstructions(Instruction instruction) {
    gameEtat = ETAT_GAME_INSTRUCTION_FINIE;//Pour passer à la suivante pour toutes les instructions sans MV ou autre
      int timeopr = 1000; 
    switch (instruction.order) {

        case MV_RECALAGE: {//code inversion sur X Fait
           //if (instruction.nextActionType == MECANIQUE) {
                instruction.nextActionType = WAIT;
                int16_t distance = (((instruction.direction == FORWARD) ? 1 : -1) * 1000);  // On indique une distance de 3000 pour etre sur que le robot va ce recaler
                uint8_t coordonnee = 0;
                uint16_t val_recalage;
                actionPrecedente = MV_RECALAGE;
                if (instruction.precision == RECALAGE_Y) {
                    coordonnee = 2;
                    // if (InversStrat == 1 && ingnorInversionOnce == 0)
                    // {
                    //   val_recalage = 3000 - instruction.arg1; // Inversion du Y
                    // }
                    // else
                    // {
                    val_recalage = instruction.arg1;
                    // }
                } else {
                    coordonnee = 1;
                    if(color == Jaune){
                        val_recalage = 2000 - instruction.arg1;// Inversion du X
                    }else{
                        val_recalage = instruction.arg1;
                    }
                    
                }
                deplacement.recalage(distance, coordonnee, val_recalage);
                waitingAckID = ASSERVISSEMENT_RECALAGE;
                waitingAckFrom = ACKNOWLEDGE_MOTEUR;
                flag.wait_all(AckFrom_FLAG, timeopr);  // si jamais il y a un timeout, qu'est ce qu'il faut faire? retenter? arreter le match?
               printf("\n\n Recalage du robot \n") ; 
                waitingAckID_FIN = ASSERVISSEMENT_RECALAGE;
                waitingAckFrom_FIN = INSTRUCTION_END_MOTEUR;
                flag.wait_all(AckFrom_FIN_FLAG, 20000);

            // } else {
            // }
        } break;
        case MV_TURN: {//code inversion sur X Fait
            int16_t angle = instruction.arg3;
            target_x_robot = x_robot;
            target_y_robot = y_robot;
            target_theta_robot = theta_robot + angle;
            actionPrecedente = MV_TURN;
            // if (InversStrat == 1 && ingnorInversionOnce == 0)
            // {
            //   angle = -angle;
            // }
            if(color == Jaune){
                angle = -angle;
            }

            if (instruction.direction == ABSOLUTE && (int16_t)theta_robot) {
                // C'est une rotation absolu, il faut la convertir en relative

                angle = (int16_t)(angle - (int16_t)theta_robot);// % 3600;
                if (angle > 1800){angle -= 3600;}
                else if (angle <= -1800){angle += 3600;}//Calcule le chemin le plus court
            }
            
            if(angle){
                waitingAckID = ASSERVISSEMENT_ROTATION;
                waitingAckFrom = ACKNOWLEDGE_MOTEUR;
                angle = angle / 10 ; 
                deplacement.rotation(angle);
            // printf("deplacement.rotation(angle : %d);\n", angle);
               flag.wait_all(AckFrom_FLAG, timeopr);

                waitingAckID_FIN = ASSERVISSEMENT_ROTATION;
                waitingAckFrom_FIN = INSTRUCTION_END_MOTEUR;
                printf("\n\nEn attente de fin de mouvement \n") ; 
                flag.wait_all(AckFrom_FIN_FLAG);
            }
            printf("Mouvement de rotation de %d°" , angle) ; 
            gameEtat = ETAT_GAME_INSTRUCTION_FINIE;//ETAT_GAME_MVT_DANGER
        } break;
        case MV_LINE: {
            waitingAckID = ASSERVISSEMENT_RECALAGE;
            waitingAckFrom = ACKNOWLEDGE_MOTEUR;
            
            
           
            actionPrecedente = MV_LINE;
            int16_t distance = (((instruction.direction == FORWARD) ? 1 : -1) * instruction.arg1);
            
            target_x_robot = x_robot + distance * cos((double)theta_robot * M_PI / 1800.0);
            target_y_robot = y_robot + distance * sin((double)theta_robot * M_PI / 1800.0);
            target_theta_robot = theta_robot;
            target_sens = (instruction.direction == FORWARD) ? 1 : -1;

            deplacement.toutDroit(distance);
           
            flag.wait_all(AckFrom_FLAG , timeopr);
           
            
            waitingAckID_FIN = ASSERVISSEMENT_RECALAGE;
            waitingAckFrom_FIN = INSTRUCTION_END_MOTEUR;
            printf("\n\nEn attente de fin de mouvement \n") ; 

            flag.wait_all(AckFrom_FIN_FLAG  );
            printf ( "\n\n Mouvement ligne droite de %dcm \n\n" , distance) ; 
            
            gameEtat = ETAT_GAME_INSTRUCTION_FINIE;//ETAT_GAME_MVT_DANGER
        } break;
        case MV_XYT: {//code inversion sur X Fait
            // on effectue XYT normalement selon les instructions
            uint16_t x;
            uint16_t y = instruction.arg2;
            int16_t theta;
            uint8_t sens;
            actionPrecedente = MV_XYT;

            // en avant ou en arriére 
            if (instruction.direction == BACKWARD) {
                sens = -1;
            } else {
                sens = 1;
            }
            // --------------------------------

            if(color == Jaune){//code inversion sur X Fait
                x = 2000 - instruction.arg1;// Inversion du X
                theta = 1800 + instruction.arg3;
                    if(theta > 1800){theta -= 3600;} 
                    else if(theta < -1800){theta += 3600;}
            }else{
                x = instruction.arg1;
                theta = instruction.arg3;
            }

            // --------------------------- 

            waitingAckID = ASSERVISSEMENT_XYT;
            waitingAckFrom = ACKNOWLEDGE_MOTEUR;

          

            // --- ------ 
            if ((x <= 0) || (y <= 0)) {
                //deplacement.positionXYTheta(target_x_robot, target_y_robot, target_theta_robot, sens);
               
            } else {
                deplacement.positionXYTheta(x, y, theta, sens);
                flag.wait_all(AckFrom_FLAG, timeopr);
                target_x_robot = x;
                target_y_robot = y;
                target_theta_robot = theta;
                target_sens = sens;

                

                waitingAckID_FIN = ASSERVISSEMENT_XYT;
                waitingAckFrom_FIN = INSTRUCTION_END_MOTEUR;
                printf("\n\nEn attente de fin de mouvement \n") ; 
                flag.wait_all(AckFrom_FIN_FLAG);
                gameEtat = ETAT_GAME_INSTRUCTION_FINIE;//ETAT_GAME_MVT_DANGER
//              flag.wait_all(AckFrom_FIN_FLAG, 20000);//A ne pas mettre, geré dans ETAT_GAME_MVT_DANGER avec le le lidar
            }
          
            printf("\n\n Mouvement vers une valeur x : %u , y : %u , theta : %d et sens : %d\n\n" , x , y , theta , sens ) ; 
            
        } break;
        case MV_COURBURE: {//code inversion Fait mais a tester
            //    int16_t rayon;
            int16_t angle;
            int8_t sens;

            float alpha = 0, theta = 0;
            short alph = 0;
            actionPrecedente = MV_COURBURE;

            sens = ((instruction.direction == LEFT) ? 1 : -1);
            angle = instruction.arg3;
            /*if(InversStrat == 1 && ingnorInversionOnce == 0) {
                localData1 = -localData1;//Inversion de la direction
            }*/
            if(color == Jaune){//A tester, je ne suis pas sûre
                sens = -sens;
                angle = -angle;//Inversion de la direction
            }
            enum EnumInstructionDirection directionxyt;
            waitingAckID = ASSERVISSEMENT_COURBURE;
            waitingAckFrom = ACKNOWLEDGE_MOTEUR;
          
            
            deplacement.courbure(instruction.arg1, angle, sens);
            flag.wait_all(AckFrom_FLAG, timeopr);

            alph = angle;
            if (angle > 0) {
                direction = 1;
            } else {
                direction = -1;
            }
            if (angle > 0) {
                directionxyt = FORWARD;
                asser_stop_direction = 1;
            } else {
                directionxyt = BACKWARD;
                asser_stop_direction = -1;
            }
            alpha = angle * M_PI / 1800.0f;
            theta = theta_robot * M_PI / 1800.0f;
            int nbre;
            nbre = abs(alph) / 450;
            for (int c = 0; c < nbre + 1; c++) {
                dodgeq.inst[c].order = MV_XYT;
                dodgeq.inst[c].direction = directionxyt;
                if (instruction.direction == LEFT) {                                                  //-------------LEFT
                    target_x_robot = x_robot + instruction.arg1 * (sin(theta + alpha) - sin(theta));  // X
                    target_y_robot = y_robot + instruction.arg1 * (cos(theta) - cos(theta + alpha));  // Y
                    target_theta_robot = theta_robot + alph;                                          // T
                    dodgeq.inst[c].arg1 = target_x_robot;                                             // X
                    dodgeq.inst[c].arg2 = target_y_robot;                                             // Y
                    dodgeq.inst[c].arg3 = target_theta_robot;                                         // T
                } else {
                    target_x_robot = x_robot + instruction.arg1 * (sin(theta) - sin(theta - alpha));  // X
                    target_y_robot = y_robot + instruction.arg1 * (cos(theta - alpha) - cos(theta));  // Y
                    target_theta_robot = theta_robot - alph;                                          // T
                    dodgeq.inst[c].arg1 = target_x_robot;                                             // X
                    dodgeq.inst[c].arg2 = target_y_robot;                                             // Y
                    dodgeq.inst[c].arg3 = target_theta_robot;                                         // T
                }
                alpha -= alpha / ((float)nbre);
                alph -= alph / (nbre + 1);
            }

          
            waitingAckID_FIN = ASSERVISSEMENT_COURBURE;
            waitingAckFrom_FIN = INSTRUCTION_END_MOTEUR;
            flag.wait_all(AckFrom_FIN_FLAG);
            printf("\n\n deplacement courbure de : %d \n" , angle  ) ; 
            gameEtat = ETAT_GAME_INSTRUCTION_FINIE; //ETAT_GAME_MVT_DANGER

//            flag.wait_all(AckFrom_FIN_FLAG, 20000);
        } break;
        case PINCE: {
            uint8_t Etage = (instruction.arg1 & 0xFF);
            uint8_t etatHerkulex = ((instruction.arg2 == 1) ? 1 : 0);
            uint8_t sens = (instruction.arg3 & 0xFF);
            actionPrecedente = PINCE;
            // if (instruction.nextActionType != ENCHAINEMENT){
                // waitingAckID = IDCAN_PINCE;
                // waitingAckFrom = ACKNOWLEDGE_ACTIONNEURS;
            // }
            
            // printf("Herkulex.controlePince(Etage : %d, etatHerkulex : %d, sens : %d);\n",Etage,etatHerkulex, sens);
            
            herkulex.controlePince(Etage, etatHerkulex, sens);
            printf("Herkulex.controlePince(Etage : %d, etatHerkulex : %d, sens : %d);\n", Etage, etatHerkulex, sens);

            // if (instruction.nextActionType != ENCHAINEMENT){
                // flag.wait_all(AckFrom_FLAG, 20000);

                waitingAckID_FIN = IDCAN_PINCE;
                // waitingAckFrom_FIN = INSTRUCTION_END_PINCE;
                flag.wait_all(AckFrom_FIN_FLAG, 10000);
            // }

            
        } break;
            case ACTION:
            {
                actionPrecedente = ACTION;
                int niveaux = 8 ; 
                printf("\n suivant : %d \n" , instruction.arg1) ; 
                waitingAckFrom = ACKNOWLEDGE_ACTIONNEURS;
            
                if ( instruction.arg1 == 70) // action position_base ou niveaux_base
                {
                    if ( instruction.arg2 == 1) // avant 
                    {
                             niveaux = instruction.arg3 ; 
                             threadCAN.sendAck(IDCAN_NIV_BASE_AV , niveaux) ; 
                             waitingAckID =IDCAN_NIV_BASE_AV ;
                             waitingAckID_FIN =IDCAN_NIV_BASE_AV ; 
                             printf ("Action Niveaux base avant de niveaux %d \n" , niveaux) ; 
                    }
                    else if ( instruction.arg2 == 2 ) //  arriere 
                    {
                        
                        niveaux = instruction.arg3 ; 
                        threadCAN.sendAck(IDCAN_NIV_BASE_ARR , niveaux) ; 
                        waitingAckID =IDCAN_NIV_BASE_ARR ;
                        waitingAckID_FIN =IDCAN_NIV_BASE_ARR ; 
                        printf ("Action Niveaux base arriére de niveaux %d \n" , niveaux) ; 

                    }
                    flag.wait_all(AckFrom_FLAG, timeopr);

                }

                else if ( instruction.arg1 == 80) // action constuire 
                {
                    if ( instruction.arg2 == 1) // avant 
                    {
                         
                             threadCAN.send(IDCAN_Construire_avant) ; 
                             waitingAckID =IDCAN_Construire_avant ;
                             waitingAckID_FIN =IDCAN_Construire_avant ; 
                             printf ("Action constuire un gradin avant \n") ; 
                    }
                    else if ( instruction.arg2 == 2 ) //  arriere 
                    {
                        
                        threadCAN.send(IDCAN_Construire_arriere) ; 
                        waitingAckID =IDCAN_Construire_arriere ;
                       waitingAckID_FIN =IDCAN_Construire_arriere ; 
                        printf ("Action constuire un gradin arriére \n") ; 

                    }
                    flag.wait_all(AckFrom_FLAG, timeopr);
                    printf("\n suivant \n") ; 
                }
             
                else if ( instruction.arg1 == 90 ) // action poser ou lacher 
                {
                    
                    if ( instruction.arg2 == 1) // avant 
                    {
                            niveaux = instruction.arg3 ; 
                             threadCAN.sendAck(IDCAN_LACHE_AV , niveaux) ; 
                             waitingAckID =IDCAN_LACHE_AV ;
                             waitingAckID_FIN =IDCAN_LACHE_AV ; 
                             
                             printf ("Action poser  avant \n") ; 
                    }
                    else if ( instruction.arg2 == 2 ) //  arriere 
                    {
                        
                        niveaux = instruction.arg3 ; 
                        threadCAN.sendAck(IDCAN_LACHE_AR , niveaux) ; 
                        waitingAckID =IDCAN_LACHE_AR;
                        waitingAckID_FIN =IDCAN_LACHE_AR; 
                        printf ("Action poser  arriére \n") ; 

                    }
                   flag.wait_all(AckFrom_FLAG, timeopr);

                }

                else if ( instruction.arg1 == 100) // action Position initiale 
                {
                    if ( instruction.arg2 == 1) // avant 
                    {
                         
                             threadCAN.send(POS_avant) ; 
                             waitingAckID =POS_avant ;
                             waitingAckID_FIN =POS_avant ; 
                             printf ("Action position initiale  avant \n") ; 
                    }
                    else if ( instruction.arg2 == 2 ) //  arriere 
                    {
                        
                        threadCAN.send(POS_arriere) ; 
                        waitingAckID =POS_arriere ;
                        waitingAckID_FIN =POS_arriere ; 
                        printf ("Action position initiale arriére \n") ; 

                    }

                   flag.wait_all(AckFrom_FLAG, timeopr);
                }
                

                else if  ( instruction.arg1 == 110 ) // action deploiyement banderole 
                { 
                             threadCAN.send(DEP_banderole) ; 
                             waitingAckID =DEP_banderole ;
                             waitingAckID_FIN =DEP_banderole ; 
                             printf ("Action delploiyement de la banderole\n ") ; 
                   
                             flag.wait_all(AckFrom_FLAG, timeopr);
                }
                
                else if ( instruction.arg1 == 120 ) // action ventouse 
                {
                    if ( instruction.arg2 == 1) // avant 
                    {
                         
                             threadCAN.send(VENT_AV) ; 
                             waitingAckID =VENT_AV ;
                             waitingAckID_FIN =VENT_AV ; 
                             printf ("Action ventouse avant \n") ; 
                    }
                    else if ( instruction.arg2 == 2 ) //  arriere 
                    {
                        
                        threadCAN.send(VENT_AR) ; 
                        waitingAckID =VENT_AR ;
                        waitingAckID_FIN =VENT_AR ; 
                        printf ("Action ventouse  arriére \n") ; 

                    }
                    flag.wait_all(AckFrom_FLAG, timeopr);
                }
                
                else if (instruction.arg1 == 130 ) // action pompe 
                {
                    if ( instruction.arg2 == 1) // avant 
                    {
                         
                             threadCAN.send(Pompe_av) ; 
                             waitingAckID =VENT_AV ;
                            waitingAckID_FIN =VENT_AV ; 
                             printf ("Action pompe avant \n") ; 
                    }
                    else if ( instruction.arg2 == 2 ) //  arriere 
                    {
                        
                        threadCAN.send(Pompe_ar) ; 
                        waitingAckID =Pompe_ar ;
                        waitingAckID_FIN =Pompe_ar ; 
                        printf ("Action pompe  arriére \n") ; 

                    }
                    flag.wait_all(AckFrom_FLAG, timeopr);

                }
   
                else if (instruction.arg1 == 140 )  // action saisir 
                {
                    if ( instruction.arg2 == 1 ) // avant 
                     {
                        threadCAN.send(saisir_avant) ; 
                        waitingAckID =saisir_avant ;
                       waitingAckID_FIN =saisir_avant ; 
                        printf ("Action saisir avant \n") ; 

                     }
                    else if ( instruction.arg2 == 0 ) // arriere 
                    {
                        threadCAN.send(saisir_arriere) ; 
                        waitingAckID =saisir_arriere ;
                       waitingAckID_FIN =saisir_arriere ; 
                        printf ("Action saisir arriére \n") ; 

                    }
                    flag.wait_all(AckFrom_FLAG, timeopr);
                }
                
                waitingAckFrom_FIN = INSTRUCTION_END_Actionneur;
                flag.wait_all(AckFrom_FIN_FLAG);
                gameEtat = ETAT_GAME_INSTRUCTION_FINIE;
            
            }
            break;

            case POSITION: 
            {
            actionPrecedente = POSITION;
            uint16_t x;
            uint16_t y = instruction.arg2;
            int16_t theta;
            
            if(color == Jaune){//code inversion sur X Fait
                x = 2000 - instruction.arg1;// Inversion du X
                theta = 1800 + instruction.arg3;
                    if(theta > 1800){theta -= 3600;} 
                    else if(theta < -1800){theta += 3600;}
            }else{
                x = instruction.arg1;
                theta = instruction.arg3;
            }
            
            threadCAN.send(Pos_Init , x , y ); 
            printf("\n\n Position de départ du robot x = %d et y = %d \n" , x ,y ) ; 
            gameEtat = ETAT_GAME_INSTRUCTION_FINIE;
        }
            break ; 
            
            default:
            break;
    }

}

E_Stratposdebut etat_pos = RECALAGE_1;

// bool machineRecalageInit()
// {
//     etat_pos = RECALAGE_1;
//     recalageErreur = 0;
//     ThisThread::sleep_for(50ms);
//     listeInstructions.debut();
//     // deplacement.asservOn();
//     return true;
// }

// bool machineRecalage() {
//     const Instruction &instruction = listeInstructions.enCours();
//     switch (etat_pos)
//     {
//     case RECALAGE_1: {
//         threadCAN.sendAck(RECALAGE_START, 0);
            
//             waitingAckID = ACKNOWLEDGE_ACTIONNEURS;
//             waitingAckFrom = IDCAN_PINCE_ARRIERE;

//             deplacement.asservOn(); ThisThread::sleep_for(50ms);
//             printf("deplacement.asservOn();\n");

//             herkulex.changerIdHerkulexPince(8); ThisThread::sleep_for(50ms);
//             printf("herkulex.changerIdHerkulexPince(8);\n");
            
//             herkulex.stepMotorMode(0);  ThisThread::sleep_for(50ms);
//             printf("herkulex.stepMotorMode(0);\n");
            
//             //position fermer pour ne pas gener recalage arriere
//             herkulex.controlePinceArriere(0,0); ThisThread::sleep_for(50ms);
//             printf("herkulex.controlePinceArriere(0,0);\n");
            
//             herkulex.controlePince(0,0,0);  ThisThread::sleep_for(50ms);
//             printf("herkulex.controlePince(0,0,0);\n");
            

            
            

//             int16_t distance = -1000;
//             uint16_t val_recalage = MOITIEE_ROBOT;

//             // if (Hauteur == ROBOT_EN_BAS) {
//             //     val_recalage = 2000 - (MOITIEE_ROBOT);
//             // } else {
//             //     val_recalage = MOITIEE_ROBOT;
//             // }
            
//             // if(assiette_choisie == HG_ASS_VERTE_CARRE){

//             // }else 
//             if(assiette_choisie == HG_ASS_VERTE_CARRE){
//                 depart_x = 225; depart_y = 450 - MOITIEE_ROBOT;         depart_theta_robot = 0;

//             }else if(assiette_choisie == BG_ASS_BLEU_CARRE){
//                 depart_x = 2000 - 225; depart_y = 450 - MOITIEE_ROBOT;         depart_theta_robot = 1800;
//                 val_recalage = 2000 - (MOITIEE_ROBOT);

//             }else if(assiette_choisie == HC_ASS_BLEU){//Base
//                 depart_x = 225; depart_y = (900 + 450) - MOITIEE_ROBOT;   depart_theta_robot = 0;

//             }else if(assiette_choisie == HC_ASS_VERT){//Base
//                 depart_x = 225; depart_y = (3000 - 900 - 450) + MOITIEE_ROBOT;         depart_theta_robot = 0;

//             }else if(assiette_choisie == BC_ASS_BLEU){//Base
//                 depart_x = 2000-225; depart_y = (3000 - 900 - 450) + MOITIEE_ROBOT;         depart_theta_robot = 1800;
//                 val_recalage = 2000 - (MOITIEE_ROBOT);

//             }else if(assiette_choisie == BC_ASS_VERT){//Base
//                 depart_x = 2000-225; depart_y = (900 + 450) - MOITIEE_ROBOT;         depart_theta_robot = 1800;
//                 val_recalage = 2000 - (MOITIEE_ROBOT);

//             }else if(assiette_choisie == HD_ASS_BLEU){
//                 depart_x = 225; depart_y = 450 - MOITIEE_ROBOT;         depart_theta_robot = 0;

//             }else if(assiette_choisie == HD_ASS_VERT){//Recalage sur y
//                 depart_x = 225; depart_y = 450 - MOITIEE_ROBOT;         depart_theta_robot = -900;

//             }else if(assiette_choisie == BD_ASS_BLEU){//Recalage sur y
//                 depart_x = 225; depart_y = 450 - MOITIEE_ROBOT;         depart_theta_robot = -900;
//                 val_recalage = 2000 - (MOITIEE_ROBOT);

//             }else if(assiette_choisie == BD_ASS_VERT){
//                 depart_x = 2000 - 225; depart_y = 450 - MOITIEE_ROBOT;         depart_theta_robot = 0;
//                 val_recalage = 2000 - (MOITIEE_ROBOT);

//             }else{//HG_ASS_VERTE_CARRE
//                 depart_x = 225; depart_y = 450 - MOITIEE_ROBOT;         depart_theta_robot = 0;

//             }

//             deplacement.setOdoPetit(depart_x, depart_y, depart_theta_robot); ThisThread::sleep_for(50ms);
//             printf("deplacement.setOdoPetit(depart_x, depart_y, depart_theta_robot);\n");
            
//             deplacement.setOdoGrand(depart_x, depart_y, depart_theta_robot); ThisThread::sleep_for(50ms);
            
//             waitingAckID = ASSERVISSEMENT_RECALAGE;
//             waitingAckFrom = ACKNOWLEDGE_MOTEUR;
//             if(assiette_choisie == HD_ASS_VERT || assiette_choisie == BD_ASS_BLEU){
//                 deplacement.recalage(distance, 2, val_recalage);
//             }else{
//                 deplacement.recalage(distance, 1, val_recalage);
//             }
            
//             // printf("deplacement.recalage(distance : %d, 1, val_recalage : %d);\n", distance, val_recalage);
//             if (flag.wait_all(AckFrom_FLAG, 20000) == osFlagsErrorTimeout) {
//                 // printf("osErrorTimeout, recalage fail, RECALAGE_1, waitingAckID\n");
//                 gameEtat = ETAT_GAME_INIT;
//                 recalageErreur = -1;
//                 return false;
//             }

//             waitingAckID_FIN = ASSERVISSEMENT_RECALAGE;
//             waitingAckFrom_FIN = INSTRUCTION_END_MOTEUR;
//             if (flag.wait_all(AckFrom_FIN_FLAG, 20000) == osFlagsErrorTimeout) {
//                 // printf("osErrorTimeout, recalage fail, RECALAGE_1, waitingAckID_FIN\n");
//                 gameEtat = ETAT_GAME_INIT;
//                 recalageErreur = -2;
//                 return false;
//             }

//             etat_pos = RECULER_1;
//     }   
//     break;
//     case RECULER_1: {
//             waitingAckID = ASSERVISSEMENT_RECALAGE;
//             waitingAckFrom = ACKNOWLEDGE_MOTEUR;
//             int16_t distance = 100;
            
//             deplacement.toutDroit(distance);
//             // printf("deplacement.toutDroit(distance : %d);\n", distance);
//             if (flag.wait_all(AckFrom_FLAG, 20000) == osFlagsErrorTimeout) {
//                 // printf("osErrorTimeout, recalage fail, RECULER_1, waitingAckID\n");
//                 gameEtat = ETAT_GAME_INIT;
//                 recalageErreur = -3;
//                 return false;
//             }

//             waitingAckID_FIN = ASSERVISSEMENT_RECALAGE;
//             waitingAckFrom_FIN = INSTRUCTION_END_MOTEUR;
//             if (flag.wait_all(AckFrom_FIN_FLAG, 20000) == osFlagsErrorTimeout) {
//                 // printf("osErrorTimeout, recalage fail, RECULER_1, waitingAckID_FIN\n");
//                 gameEtat = ETAT_GAME_INIT;
//                 recalageErreur = -4;
//                 return false;
//             }
            
   
//             etat_pos = GOTOPOS;
//         } break;
//         case GOTOPOS: {
//             waitingAckID = ASSERVISSEMENT_XYT;
//             waitingAckFrom = ACKNOWLEDGE_MOTEUR;
//             uint16_t x;
//             uint16_t y = instruction.arg2;
//             int16_t theta = instruction.arg3;;

//             if(color == VERT){//code inversion sur X Fait
//                 x = 2000 - instruction.arg1;// Inversion du X
//                 theta = 1800 + instruction.arg3; if(theta > 1800){theta -= 3600;} else if(theta < -1800){theta += 3600;}
//             }else{
//                 x = instruction.arg1;
//                 theta = instruction.arg3;
//             }

//             deplacement.positionXYTheta(x, y, theta, 0);
//             // printf("deplacement.positionXYTheta(instruction.arg1 : %d, instruction.arg2 : %d, instruction.arg3 : %d, 0);\n", instruction.arg1, instruction.arg2, instruction.arg3);
//             if (flag.wait_all(AckFrom_FLAG, 20000) == osFlagsErrorTimeout) {
//                 // printf("osErrorTimeout, recalage, GOTOPOS, waitingAckID\n");
//                 gameEtat = ETAT_GAME_INIT;
//                 recalageErreur = -5;
//                 return false;
//             }

//             waitingAckID_FIN = ASSERVISSEMENT_XYT;
//             waitingAckFrom_FIN = INSTRUCTION_END_MOTEUR;
//             if (flag.wait_all(AckFrom_FIN_FLAG, 20000) == osFlagsErrorTimeout) {
//                 // printf("osErrorTimeout, recalage, GOTOPOS, waitingAckID_FIN\n");
//                 gameEtat = ETAT_GAME_INIT;
//                 recalageErreur = -6;
//                 return false;
//             }
//             etat_pos = FIN_POS;
//         } break;
//         case FIN_POS: {
//             // actual_instruction = instruction.nextLineOK;
//             int ligne = ((instruction.nextLineOK != instruction.lineNumber) ? instruction.nextLineOK : (instruction.lineNumber+1));
//             listeInstructions.vaLigne(ligne);
//             target_x_robot = x_robot;
//             target_y_robot = y_robot;
//             target_theta_robot = theta_robot;

//             herkulex.controlePince(0,0,0);
//             return false;
//         } break;
    
//     default:
//         break;
//     }
//     return true;
// }
// /*
// switch (etat_pos) {
//         case RECALAGE_1: {
//             threadCAN.sendAck(RECALAGE_START, 0);
            
//             waitingAckID = ACKNOWLEDGE_ACTIONNEURS;
//             waitingAckFrom = IDCAN_PINCE_ARRIERE;
//             herkulex.controlePinceArriere(0,0);//position fermer pour ne pas gener recalage arriere
//             printf("herkulex.controlePinceArriere(0,0);\n");
//             herkulex.changerIdHerkulexPince(8);
//             printf("herkulex.changerIdHerkulexPince(8);\n");

//             herkulex.controlePince(4,0,0);//position à 80 mm de haut pour ne pas gener recalage avant
//             printf("herkulex.controlePince(4,0,0);\n");
//             deplacement.asservOn();
//             printf("deplacement.asservOn();\n");
//             herkulex.stepMotorMode(0);
//             printf("herkulex.stepMotorMode(0);\n");

//             int16_t distance = 1000;
//             uint16_t val_recalage;

//             if (Hauteur == ROBOT_EN_BAS) {
//                 val_recalage = 2000 - (MOITIEE_ROBOT);
//             } else {
//                 val_recalage = MOITIEE_ROBOT;
//                 distance *= -1;
//             }

//             waitingAckID = ASSERVISSEMENT_RECALAGE;
//             waitingAckFrom = ACKNOWLEDGE_MOTEUR;
//             deplacement.recalage(distance, 1, val_recalage);
//             // printf("deplacement.recalage(distance : %d, 1, val_recalage : %d);\n", distance, val_recalage);
//             if (flag.wait_all(AckFrom_FLAG, 20000) == osFlagsErrorTimeout) {
//                 // printf("osErrorTimeout, recalage fail, RECALAGE_1, waitingAckID\n");
//                 gameEtat = ETAT_GAME_INIT;
//                 recalageErreur = -1;
//                 return false;
//             }

//             waitingAckID_FIN = ASSERVISSEMENT_RECALAGE;
//             waitingAckFrom_FIN = INSTRUCTION_END_MOTEUR;
//             if (flag.wait_all(AckFrom_FIN_FLAG, 20000) == osFlagsErrorTimeout) {
//                 // printf("osErrorTimeout, recalage fail, RECALAGE_1, waitingAckID_FIN\n");
//                 gameEtat = ETAT_GAME_INIT;
//                 recalageErreur = -2;
//                 return false;
//             }

//             etat_pos = RECULER_1;
//         } break;
//         case RECULER_1: {
//             waitingAckID = ASSERVISSEMENT_RECALAGE;
//             waitingAckFrom = ACKNOWLEDGE_MOTEUR;
//             int16_t distance = 100;
//             if (Hauteur == ROBOT_EN_BAS) {
//                 distance *= -1;
//             }
//             deplacement.toutDroit(distance);
//             // printf("deplacement.toutDroit(distance : %d);\n", distance);
//             if (flag.wait_all(AckFrom_FLAG, 20000) == osFlagsErrorTimeout) {
//                 // printf("osErrorTimeout, recalage fail, RECULER_1, waitingAckID\n");
//                 gameEtat = ETAT_GAME_INIT;
//                 recalageErreur = -3;
//                 return false;
//             }

//             waitingAckID_FIN = ASSERVISSEMENT_RECALAGE;
//             waitingAckFrom_FIN = INSTRUCTION_END_MOTEUR;
//             if (flag.wait_all(AckFrom_FIN_FLAG, 20000) == osFlagsErrorTimeout) {
//                 // printf("osErrorTimeout, recalage fail, RECULER_1, waitingAckID_FIN\n");
//                 gameEtat = ETAT_GAME_INIT;
//                 recalageErreur = -4;
//                 return false;
//             }
//             etat_pos = TOURNER;
//         } break;
//         case TOURNER: {
//             waitingAckID = ASSERVISSEMENT_ROTATION;
//             waitingAckFrom = ACKNOWLEDGE_MOTEUR;
//             int16_t angle = 900;
//             if (Cote == ROBOT_A_GAUCHE) {
//                 angle = 900;
//             } else if (Cote == ROBOT_A_DROITE) {
//                 angle = -900;
//             }

//             deplacement.rotation(angle);
//             // printf("deplacement.rotation(angle : %d);\n", angle);
//             if (flag.wait_all(AckFrom_FLAG, 20000) == osFlagsErrorTimeout) {
//                 // printf("osErrorTimeout, recalage fail, TOURNER, waitingAckID\n");
//                 gameEtat = ETAT_GAME_INIT;
//                 recalageErreur = -5;
//                 return false;
//             }

//             waitingAckID_FIN = ASSERVISSEMENT_ROTATION;
//             waitingAckFrom_FIN = INSTRUCTION_END_MOTEUR;
//             if (flag.wait_all(AckFrom_FIN_FLAG, 20000) == osFlagsErrorTimeout) {
//                 // printf("osErrorTimeout, recalage fail, TOURNER, waitingAckID_FIN\n");
//                 gameEtat = ETAT_GAME_INIT;
//                 recalageErreur = -6;
//                 return false;
//             }
//             etat_pos = RECALAGE_2;
//         } break;
//         case RECALAGE_2: {
//             int16_t distance = -1000;
//             uint16_t val_recalage;

//             if (Cote == ROBOT_A_DROITE) {
//                 val_recalage = 3000 - (MOITIEE_ROBOT);
//             }  // par devant
//             else {
//                 val_recalage = MOITIEE_ROBOT;
//             }  // par derriere

//             waitingAckID = ASSERVISSEMENT_RECALAGE;
//             waitingAckFrom = ACKNOWLEDGE_MOTEUR;
//             deplacement.recalage(distance, 2, val_recalage);
//             // printf("deplacement.recalage(distance : %d, 2, val_recalage : %d);\n", distance, val_recalage);
//             if (flag.wait_all(AckFrom_FLAG, 20000) == osFlagsErrorTimeout) {
//                 // printf("osErrorTimeout, recalage fail, RECALAGE_2, waitingAckID\n");
//                 gameEtat = ETAT_GAME_INIT;
//                 recalageErreur = -7;
//                 return false;
//             }

//             waitingAckID_FIN = ASSERVISSEMENT_RECALAGE;
//             waitingAckFrom_FIN = INSTRUCTION_END_MOTEUR;
//             if (flag.wait_all(AckFrom_FIN_FLAG, 20000) == osFlagsErrorTimeout) {
//                 // printf("osErrorTimeout, recalage fail, RECALAGE_2, waitingAckID_FIN\n");
//                 gameEtat = ETAT_GAME_INIT;
//                 recalageErreur = -8;
//                 return false;
//             }

//             etat_pos = RECULER_2;
//         } break;
//         case RECULER_2: {
//             waitingAckID = ASSERVISSEMENT_RECALAGE;
//             waitingAckFrom = ACKNOWLEDGE_MOTEUR;
//             int16_t distance = 100;
//             deplacement.toutDroit(distance);
//             // printf("deplacement.toutDroit(distance : %d);\n", distance);
//             if (flag.wait_all(AckFrom_FLAG, 20000) == osFlagsErrorTimeout) {
//                 // printf("osErrorTimeout, recalage fail, RECULER_2, waitingAckID\n");
//                 gameEtat = ETAT_GAME_INIT;
//                 recalageErreur = -9;
//                 return false;
//             }

//             waitingAckID_FIN = ASSERVISSEMENT_RECALAGE;
//             waitingAckFrom_FIN = INSTRUCTION_END_MOTEUR;
//             if (flag.wait_all(AckFrom_FIN_FLAG, 20000) == osFlagsErrorTimeout) {
//                 // printf("osErrorTimeout, recalage fail, RECULER_2, waitingAckID_FIN\n");
//                 gameEtat = ETAT_GAME_INIT;
//                 recalageErreur = -10;
//                 return false;
//             }
//             etat_pos = GOTOPOS;
//         } break;
//         case GOTOPOS: {
//             waitingAckID = ASSERVISSEMENT_XYT;
//             waitingAckFrom = ACKNOWLEDGE_MOTEUR;
//             deplacement.positionXYTheta(instruction.arg1, instruction.arg2, instruction.arg3, 0);
//             // printf("deplacement.positionXYTheta(instruction.arg1 : %d, instruction.arg2 : %d, instruction.arg3 : %d, 0);\n", instruction.arg1, instruction.arg2, instruction.arg3);
//             if (flag.wait_all(AckFrom_FLAG, 20000) == osFlagsErrorTimeout) {
//                 // printf("osErrorTimeout, recalage, GOTOPOS, waitingAckID\n");
//                 gameEtat = ETAT_GAME_INIT;
//                 recalageErreur = -11;
//                 return false;
//             }

//             waitingAckID_FIN = ASSERVISSEMENT_XYT;
//             waitingAckFrom_FIN = INSTRUCTION_END_MOTEUR;
//             if (flag.wait_all(AckFrom_FIN_FLAG, 20000) == osFlagsErrorTimeout) {
//                 // printf("osErrorTimeout, recalage, GOTOPOS, waitingAckID_FIN\n");
//                 gameEtat = ETAT_GAME_INIT;
//                 recalageErreur = -12;
//                 return false;
//             }
//             etat_pos = FIN_POS;
//         } break;
//         case FIN_POS: {
//             // actual_instruction = instruction.nextLineOK;
//             listeInstructions.vaLigne(instruction.nextLineOK);
//             target_x_robot = x_robot;
//             target_y_robot = y_robot;
//             target_theta_robot = theta_robot;

//             herkulex.controlePince(0,0,0);

//             gameEtat = ETAT_GAME_WAIT_FOR_JACK;
//             return false;
//         } break;

//         default:
//             break;
//     }
//     */

//ACKNOWLEDGE_MOTEUR ACKNOWLEDGE_ACTIONNEURS INSTRUCTION_END_PINCE
string AckToString(int id){
    switch (id)
    {
        //Partie waitingAckFrom
    case ACKNOWLEDGE_MOTEUR:
        {
             return "MOTEUR";
        }
        break;
    case ACKNOWLEDGE_ACTIONNEURS:
        {
             return "ACTIONNEURS";
        }
        break;
    case INSTRUCTION_END_Actionneur:
        {
             return "Action ";
        }
        break;
    //Partie waitingAckID
    case ASSERVISSEMENT_RECALAGE:
        {
             return "Recalage ou line";
        }
        break;
    case ASSERVISSEMENT_ROTATION:
        {
             return "ROTATION";
        }
        break;
    case ASSERVISSEMENT_XYT:
        {
             return "XYT";
        }
        break;
    case ASSERVISSEMENT_COURBURE:
        {
             return "COURBURE";
        }
        break;
    case IDCAN_PINCE:
        {
             return "PINCE AVANT";
        }
        break;
    // case IDCAN_POSE_CERISE:
    //     {
    //          return "POSE CERISE";
    //     }
    //     break;
    case IDCAN_PINCE_ARRIERE:
        {
             return "PINCE ARRIERE";
        }
        break;
    // case IDCAN_ASPIRATEUR_DROIT:
    //     {
    //          return "ASPIRATEUR DROIT";
    //     }
    //     break;
    // case IDCAN_ASPIRATEUR_GAUCHE:
    //     {
    //          return "ASPIRATEUR GAUCHE";
    //     }
    //     break;
    // case IDCAN_LANCEUR:
    //     {
    //          return "LANCEUR";
    //     }
    //     break;

    default:
        break;
    }

    return "0";
}

bool getFlag(ACKFlags f, bool clearIfSet) {
    if (flag.get() & f) {
        if (clearIfSet)
            flag.clear(f);
        return true;
    }
    return false;
}