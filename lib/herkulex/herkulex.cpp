#include "herkulex.h"
#include "identCrac.h"

Herkulex::Herkulex(ThreadCAN &threadCAN) {
    m_can = &threadCAN;
}

void Herkulex::changerIdHerkulexPince(uint8_t id) {
    m_can->send(IDCAN_PINCE_CHANGE_ID_HERKULEX, id);
}

void Herkulex::controleHerkulexPosition(uint8_t IDHerkulex, short position, CouleurHerkulex setLed) {
    uint8_t commande = 0, playtime = 0x3C, couleur = 0;
    if (setLed == VERT) {
        couleur = 0x04;
    } else if (setLed == ROUGE) {
        couleur = 0x10;
    } else if (setLed == BLEU) {
        couleur = 0x08;
    }
    m_can->send(IDCAN_HERKULEX, IDHerkulex, commande, (uint16_t)position, playtime, couleur);
}

void Herkulex::controleHerkulexPositionMulEnsemble(uint8_t IDHerkulex, short position1, uint8_t ID2Herkulex, short position2) {
    char commande = 1, playtime = 0x3C;
    m_can->send(IDCAN_HERKULEX, IDHerkulex, commande, (uint16_t)position1, ID2Herkulex, (uint16_t)position2, playtime);
}

void Herkulex::controleHerkulexCouple(uint8_t IDHerkulex, bool couple) {
    uint8_t c = (couple) ? 0x60 : 0x00;
    m_can->send(IDCAN_HERKULEX_Torque, IDHerkulex, c);
}

void Herkulex::clearHerkulex(uint8_t IDHerkulex) {
    m_can->send(IDCAN_HERKULEX_Clear, IDHerkulex);
}

void Herkulex::controleHerkulexTurnMode(uint8_t IDHerkulex, uint16_t vitesse) {
    uint8_t setLed = 0x04;
    m_can->send(ID_HERKULEX_VITESSE, IDHerkulex, vitesse, setLed);
}

void Herkulex::controlePince(uint8_t Etage, uint8_t etatHerkulex, uint8_t sens) {
    m_can->send(IDCAN_PINCE, Etage, etatHerkulex, sens);
}

void Herkulex::controlePinceArriere(uint8_t etatPince, bool poseCerise){//// 0 -> fermé, 1 -> position gateau, 2 -> ouvert
    if(etatPince>2 || etatPince<0){etatPince = 2;}
    bool presenceGatoInAccount = true;
    m_can->send(IDCAN_PINCE_ARRIERE, etatPince, (uint8_t)poseCerise, (uint8_t)presenceGatoInAccount);
}

void Herkulex::stepMotorHauteur(int mm) {
    uint32_t position = mm * (3600.0 / 80.0);
    m_can->send(IDCAN_STEP_MOT_POS, position);
}

void Herkulex::stepMotorMode(uint8_t mode) {
    uint8_t m0 = mode & 0x01;
    uint8_t m1 = (mode >> 1) & 0x01;
    uint8_t m2 = (mode >> 2) & 0x01;
    m_can->send(IDCAN_STEP_MOT_MODE, m0, m1, m2);
}

/// voir si ne pas garder epreuve avant --------------

void Herkulex::controleAspirateur(bool activation){
   // m_can->send(IDCAN_ASPIRATEUR_DROIT, (uint8_t)activation);
}

void Herkulex::controleAspirateurGauche(bool activation){
   // m_can->send(IDCAN_ASPIRATEUR_GAUCHE, (uint8_t)activation);
}

void Herkulex::controleLanceur(bool activation){

   // m_can->send(IDCAN_LANCEUR, (uint8_t)activation);
}

void Herkulex::poseCerise(bool presenceGatoInAccount){
    //m_can->send(IDCAN_POSE_CERISE, (uint8_t)presenceGatoInAccount);
}
