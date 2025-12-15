#include "identCrac.h"
#include "deplacement.h"

Deplacement::Deplacement(ThreadCAN &threadCAN)
{
    m_can = &threadCAN;
}

void Deplacement::positionXYTheta(uint16_t x, uint16_t y, int16_t theta, uint8_t sens)
{
    m_can->send(ASSERVISSEMENT_XYT, x, y, (uint16_t)theta, sens);
}

void Deplacement::rotation(int16_t angle)
{
    m_can->send(ASSERVISSEMENT_ROTATION, (uint16_t)angle);
}

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
void Deplacement::toutDroit(int16_t distance)
{
    m_can->send(ASSERVISSEMENT_RECALAGE, (uint16_t)distance, (uint8_t)0, (uint16_t)0, (uint8_t)0);
}

void Deplacement::recalage(int16_t distance, uint8_t recalage, uint16_t newValue)
{
    m_can->send(ASSERVISSEMENT_RECALAGE, (uint16_t)distance, recalage, newValue, (uint8_t)0);
}

void Deplacement::courbure(int16_t rayon, int16_t angle, int8_t sens)
{
    m_can->send(ASSERVISSEMENT_COURBURE, (uint16_t)rayon, (uint16_t)angle, (uint8_t)sens, (uint8_t)0);
}

void Deplacement::asservOn(bool enable)
{
    uint8_t status = (enable) ? 1 : 0;
    m_can->send(ASSERVISSEMENT_ENABLE, status);
}

void Deplacement::setOdoGrand(uint16_t x, uint16_t y, int16_t theta)
{
     m_can->send(ODOMETRIE_BIG_POSITION/*ASSERVISSEMENT_ODOMETRIE*/, x, y, (uint16_t)theta);
}

void Deplacement::setOdoPetit(uint16_t x, uint16_t y, int16_t theta)
{
     m_can->send(ODOMETRIE_SMALL_POSITION/*ASSERVISSEMENT_ODOMETRIE*/, x, y, (uint16_t)theta);
}

void Deplacement::vitesse(uint16_t valeur)
{
    m_can->send(ASSERVISSEMENT_CONFIG_VIT, valeur);//N'est pas configurée
}

void Deplacement::vitesseAccelDecel(uint16_t vitesse, uint16_t acceleration)//, uint16_t deceleration)
{
    m_can->send(ASSERVISSEMENT_CONFIG, vitesse, acceleration);//, deceleration);//deceleration calculé automatiquement a partir de l'acceleration, DMAX = ((double)amax)*0.75*k*k;

    // msgTx.data[6]=(uint8_t)(acceleration&0x00FF);//cloto
    // msgTx.data[7]=(uint8_t)((acceleration&0xFF00)>>8);//cloto

}

void Deplacement::accelDecel(uint16_t acceleration, uint16_t deceleration)
{
    m_can->send(ASSERVISSEMENT_CONFIG_ACCEL, acceleration, deceleration);
}

void Deplacement::courbeBezier(uint8_t nbCourbes, int16_t P1[][2], int16_t C1[][2], int16_t C2[][2], uint8_t sens)
{
    m_can->send(ASSERVISSEMENT_BEZIER, nbCourbes, sens);
    
    ThisThread::sleep_for(150ms);
    
    for (uint8_t i = 0; i < nbCourbes; i++)
    {
        m_can->send(ASSERVISSEMENT_BEZIER, (uint16_t)P1[i][0], (uint16_t)P1[i][1], (uint16_t)C1[i][0], i);
        
        ThisThread::sleep_for(150ms);
        
        m_can->send(ASSERVISSEMENT_BEZIER, (uint16_t)C1[i][1], (uint16_t)C2[i][0], (uint16_t)C2[i][1], i+100);
        // i + 100  =  Bidouille pour envoyer les points en deux trames
        
        ThisThread::sleep_for(150ms);
    }
}

void Deplacement::sendJack(bool enable){
    if(enable){m_can->send(GLOBAL_JACK);}
}
void Deplacement::stop(bool enable){
    if(enable){m_can->send(ASSERVISSEMENT_STOP);}
}