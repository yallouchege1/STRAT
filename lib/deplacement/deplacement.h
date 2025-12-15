#ifndef __DEPLACEMENT_H
#define __DEPLACEMENT_H

#include "threadCAN.h"



class Deplacement {
protected:
    ThreadCAN *m_can;

public:
    Deplacement(ThreadCAN &threadCAN);
    void positionXYTheta(uint16_t x, uint16_t y, int16_t theta, uint8_t sens);
    void rotation(int16_t angle);
/*********************************************************************************************/
/* FUNCTION NAME: GoStraight                                                                 */
/* DESCRIPTION  : Transmission CAN correspondant à une ligne droite, avec ou sans recalage   */
/*  recalage : 0 => pas de recalage                                                          */
/*             1 => recalage en X                                                            */
/*             2 => Recalage en Y                                                            */
/*  newValue : Uniquement en cas de recalage, indique la nouvelle valeur de l'odo            */
/*  isEnchainement : Indique si il faut executer l'instruction en enchainement               */
/*                   0 => non                                                                */
/*                   1 => oui                                                                */
/*                   2 => dernière instruction de l'enchainement                             */
/*********************************************************************************************/
    void toutDroit(int16_t distance);
    void recalage(int16_t distance, uint8_t recalage, uint16_t newValue);
    void courbure(int16_t rayon, int16_t angle, int8_t sens);

    void asservOn(bool enable = true);
    void asservOff(bool enable = true) { asservOn(!enable); }
    void setOdoPetit(uint16_t x, uint16_t y, int16_t theta);
    void setOdoGrand(uint16_t x, uint16_t y, int16_t theta);

    void vitesse(uint16_t valeur);
    void vitesseAccelDecel(uint16_t vitesse, uint16_t acceleration);//, uint16_t deceleration);
    void accelDecel(uint16_t acceleration, uint16_t deceleration);
    void courbeBezier(uint8_t nbCourbes, int16_t P1[][2], int16_t C1[][2], int16_t C2[][2], uint8_t sens);
    void sendJack(bool enable = true);
    void stop(bool enable = true);

};

#endif
