/* mbed Microcontroller Library
 * Copyright (c) 2019 ARM Limited
 * SPDX-License-Identifier: Apache-2.0
 */
#include <instruction.h>
#include <strategie.h>
#include <deplacement.h>
#include <global.h>
#include <herkulex.h>
#include <lvgl.h>
#include <config.h>
#include <threadCAN.h>
#include <threadLvgl.h>
#include <threadSD.h>
#include <threadSound.h>
#include <fstream>
#include <sstream>
#include <ihm.h>
#include "mbed.h"
#include <test.h>

//v0.1 

// ------ init de base pour le tout
BufferedSerial pc(USBTX, USBRX, 921600); // communication avec le pc
ThreadCAN threadCAN;                     // gestion de la communication can
ThreadSD threadSD;                       // gestion de la carde sd
ThreadLvgl threadLvgl;                   // gestion de lvgl  affichage graphique
Ihm ihm(&threadLvgl);                    // l'ecran lcd par lvgl on peut dire comme une liaison // variable ihm
Deplacement deplacement(threadCAN);      // gestion des deplacement par bus can // creation d'un  variable deplacement pour effectuer c'est derniere
Herkulex herkulex(threadCAN);            // herculex = actionneur // creation de la variable actionneur
// --------------------------------------
// gestion fichier et carte sd
vector<string> fichiers;
bool listeFichiers();
bool lectureFichier(int choix);
vector<string> fichiersMp3;
bool listeFichiersMp3();
// --------------------------------
bool jack();           // gestion du jack
DigitalIn jackPin(D7); // prise
// ------ en lien avec le macht
Thread *show;
Thread t1;
Timeout timerMatch;
void runMatch();
int tempsRestant();
void forceFinMatch();
volatile bool flagForceFinMatch = false;
// ------ Ticker pour mise à jour CarteSD
Ticker carteSDUpdateTicker;
void updateCarteSD();
int countStrategyFiles();
volatile bool flagUpdateCarteSD = false;
DigitalOut led1(PG_6);
DigitalOut led2(PD_4);
DigitalOut led3(PD_5);

int choix = -1;
uint32_t idprisencompte;
uint32_t idprisencompte2;
int selasc = 1;

void run_show(void);

int main()

{

    printf("\n Show debut de l'écran de stratégie ->  \n");

    char buf[100];                                    // tab de char de 100 // pour ecrire des message
    threadSD.registerCANControl(threadCAN);           // enregistrement de la communication can pour la carte sd
    threadCAN.registerIds(0x01, 0x7FF, canProcessRx); // enregistrement des id can pour la strategie
    int secReboot = 60;                               // delai avant reboot si pas de carte sd

    // verif de la presence carte sd

    // while (1)
    // {
    //     int flag;
    //     flag = threadSD.status(); // lecture du statut du flag
    //     if (flag & ThreadSD::FLAG_NO_CARD)
    //     {
    //         sprintf(buf, "Reboot dans %2ds", secReboot); // ecriture du message dans buf
    //         ihm.sdMsg("Carte SD absente", buf);
    //     }
    //     else
    //     {
    //         sprintf(buf, "Un fichier trouvé");
    //     }
    //     if (flag & ThreadSD::FLAG_READY)
    //         break;
    //     ThisThread::sleep_for(1s);
    //     if (secReboot-- <= 0)
    //         NVIC_SystemReset();
    // }

    // fin de la verif  -----------

    readConfig(); // lecture du de carte sd avec les dossier et parametre

    //  donne le nombre et le chemin des fichier stratégie :
    if (!listeFichiers())
    {
        printf("Dossier %s non trouvé\n", config["Dossiers"]["strategie"].c_str());
    }
    else if (fichiers.size() == 0)
    {
        printf("Aucun fichier trouvé\n");
    }
    else
    {
        int nb = fichiers.size();
        if (nb == 1)
        {
            printf("Un fichier trouvé\n");
        }
        else
        {
            printf("%d fichiers trouvés\n", nb);
        }
    }
    // --------------------

    // fin de gestion des fichier et de la carte sd
    // a la fin de gestion de la carte sd nous avons ranger les fichier txt dans stratégie

ihm.show(fichiers); // onglet avec les fichier de strategie affichier
ihm.ActionneurInit();
ThisThread::sleep_for(1ms);
led1 = 0;
led2 = 0;
led3 = 0;

// Démarrage du Ticker pour mise à jour CarteSD toutes les 2 secondes
carteSDUpdateTicker.attach(callback(updateCarteSD), 2s);

// Mise à jour initiale immédiate de l'état de la carte SD
int flag = threadSD.status();
if (flag & ThreadSD::FLAG_READY)
{
    ihm.updateCarteSDStatus(true, countStrategyFiles());
}
else
{
    ihm.updateCarteSDStatus(false, 0);
}

typedef enum
{
    multi_init,
    show_run_page,
    test
} type_etat;
static type_etat etat;
    etat = multi_init;

    while (1)
    {
        // Mise à jour périodique de l'onglet CarteSD
        if (flagUpdateCarteSD)
        {
            flagUpdateCarteSD = false;
            int flag = threadSD.status();
            if (flag & ThreadSD::FLAG_READY)
            {
                // Carte SD détectée et prête
                ihm.updateCarteSDStatus(true, countStrategyFiles());
            }
            else if (flag & ThreadSD::FLAG_NO_CARD)
            {
                // Carte SD non détectée
                ihm.updateCarteSDStatus(false, 0);
            }
        }

        switch (etat)
        {

        case multi_init:

            if (ihm.departClicked())
            {
                printf("Debut de l'ecran strat \n\n");
                etat = show_run_page;
            }

            else if (ihm.Test_ventouse())
            {

                ihm.showButtonascenceurBox();
                ThisThread::sleep_for(5s);
                selasc = selected_ascenceur;
                ihm.showButtonascenceurBoxClose();

                if (selasc == 1)
                {

                    idprisencompte = VENT_AV;
                    idprisencompte2 = 0;
                    ihm.showButtonSelectionBox();
                    ThisThread::sleep_for(5s);
                    choix_niveau = selected_level;

                    etat = test;
                }
                else if (selasc == 2)
                {

                    idprisencompte = VENT_AR;
                    idprisencompte2 = 0;
                    ihm.showButtonSelectionBox();
                    ThisThread::sleep_for(5s);
                    choix_niveau = selected_level;

                    etat = test;
                }
                else if (selasc == 3)
                {

                    idprisencompte = VENT_AV;
                    idprisencompte2 = VENT_AR;
                    ihm.showButtonSelectionBox();
                    ThisThread::sleep_for(5s);
                    choix_niveau = selected_level;

                    etat = test;
                }
                else if (selasc == 4)
                {
                    idprisencompte = 0;
                    idprisencompte2 = 0;
                    choix_niveau = 0;
                    etat = multi_init;
                    NVIC_SystemReset();
                }
            }
            else if (ihm.construction_niveaux_2())
            {
                ihm.showButtonascenceurBox();
                ThisThread::sleep_for(5s);
                selasc = selected_ascenceur;
                ihm.showButtonascenceurBoxClose();

                if (selasc == 1) // avant
                {

                    idprisencompte = IDCAN_Construire_avant;
                    idprisencompte2 = 0;
                    ihm.showButtonSelectionBox();
                    ThisThread::sleep_for(5s);
                    choix_niveau = selected_level;

                    etat = test;
                }
                else if (selasc == 2)
                {

                    idprisencompte = IDCAN_Construire_arriere;
                    idprisencompte2 = 0;
                    ihm.showButtonSelectionBox();
                    ThisThread::sleep_for(5s);
                    choix_niveau = selected_level;

                    etat = test;
                }
                else if (selasc == 3)
                {

                    idprisencompte = IDCAN_Construire_avant;
                    idprisencompte2 = IDCAN_Construire_arriere;
                    ihm.showButtonSelectionBox();
                    ThisThread::sleep_for(5s);
                    choix_niveau = selected_level;

                    etat = test;
                }
                else if (selasc == 4)
                {
                    idprisencompte = 0;
                    idprisencompte2 = 0;
                    choix_niveau = 0;
                    etat = multi_init;
                    NVIC_SystemReset();
                }
            }
            else if (ihm.Niveaux_base())
            {
                ihm.showButtonascenceurBox();
                ThisThread::sleep_for(5s);
                selasc = selected_ascenceur;
                ihm.showButtonascenceurBoxClose();

                if (selasc == 1) // avant
                {

                    idprisencompte = IDCAN_NIV_BASE_AV;
                    idprisencompte2 = 0;
                    ihm.showButtonSelectionBox();
                    ThisThread::sleep_for(5s);
                    choix_niveau = selected_level;

                    etat = test;
                }
                else if (selasc == 2)
                {

                    idprisencompte = IDCAN_NIV_BASE_ARR;
                    idprisencompte2 = 0;
                    ihm.showButtonSelectionBox();
                    ThisThread::sleep_for(5s);
                    choix_niveau = selected_level;

                    etat = test;
                }
                else if (selasc == 3)
                {

                    idprisencompte = IDCAN_NIV_BASE_AV;
                    idprisencompte2 = IDCAN_NIV_BASE_ARR;
                    ihm.showButtonSelectionBox();
                    ThisThread::sleep_for(5s);
                    choix_niveau = selected_level;

                    etat = test;
                }
                else if (selasc == 4)
                {
                    idprisencompte = 0;
                    idprisencompte2 = 0;
                    choix_niveau = 0;
                    etat = multi_init;
                    NVIC_SystemReset();
                }
            }
            else if (ihm.Position_init())
            {

                ihm.showButtonascenceurBox();
                ThisThread::sleep_for(5s);
                selasc = selected_ascenceur;
                ihm.showButtonascenceurBoxClose();

                if (selasc == 1) // avant
                {

                    idprisencompte = POS_avant;
                    idprisencompte2 = 0;
                    ihm.showButtonSelectionBox();
                    ThisThread::sleep_for(5s);
                    choix_niveau = selected_level;

                    etat = test;
                }
                else if (selasc == 2)
                {

                    idprisencompte = POS_arriere;
                    idprisencompte2 = 0;
                    ihm.showButtonSelectionBox();
                    ThisThread::sleep_for(5s);
                    choix_niveau = selected_level;

                    etat = test;
                }
                else if (selasc == 3)
                {

                    idprisencompte = POS_avant;
                    idprisencompte2 = POS_arriere;
                    ihm.showButtonSelectionBox();
                    ThisThread::sleep_for(5s);
                    choix_niveau = selected_level;

                    etat = test;
                }
                else if (selasc == 4) // bouton annuler
                {
                    idprisencompte = 0;
                    idprisencompte2 = 0;
                    choix_niveau = 0;
                    etat = multi_init;
                    NVIC_SystemReset();
                }
            }
            else if (ihm.lacherflag())
            {
                ihm.showButtonascenceurBox();
                ThisThread::sleep_for(5s);
                selasc = selected_ascenceur;
                ihm.showButtonascenceurBoxClose();

                if (selasc == 1) // avant
                {

                    idprisencompte = IDCAN_LACHE_AV;
                    idprisencompte2 = 0;
                    ihm.showButtonSelectionBox();
                    ThisThread::sleep_for(5s);
                    choix_niveau = selected_level;

                    etat = test;
                }
                else if (selasc == 2)
                {

                    idprisencompte = IDCAN_LACHE_AR;
                    idprisencompte2 = 0;
                    ihm.showButtonSelectionBox();
                    ThisThread::sleep_for(5s);
                    choix_niveau = selected_level;

                    etat = test;
                }
                else if (selasc == 3)
                {

                    idprisencompte = IDCAN_LACHE_AV;
                    idprisencompte2 = IDCAN_LACHE_AR;
                    ihm.showButtonSelectionBox();
                    ThisThread::sleep_for(5s);
                    choix_niveau = selected_level;

                    etat = test;
                }
                else if (selasc == 4) // bouton annuler
                {
                    idprisencompte = 0;
                    idprisencompte2 = 0;
                    choix_niveau = 0;
                    etat = multi_init;
                    NVIC_SystemReset();
                }
            }
            else if (ihm.autretest())
            {
                threadCAN.sendAck(TEST_BRAS_1, 8);
            }
            break;

        case show_run_page:

            choix = ihm.choixStrategie();
            lectureFichier(choix);

            if (choix == -1)
            {

                // ihm.msgBoxRecalageInit("strat par default") ;
                ihm.msgBoxmatchshow("Strat par default");
                show = new Thread;
                show->start(run_show);
                etat = multi_init;
            }

            else
            {
                //  ihm.msgBoxRecalageInit(fichiers[choix].c_str()) ;
                ihm.msgBoxmatchshow(fichiers[choix].c_str());
                show = new Thread;
                show->start(run_show);
                etat = multi_init;
            }
            break;

        case test:

            ihm.showButtonSelectionBoxClose();
            printf("\n Test effectuer  : ID envoyer - >  0x%x ", idprisencompte);
            ihm.msgBoxInit("C.R.A.C 2025", "\n\nTest en cours\nVeuillez patienter", true);
            threadCAN.sendAck(idprisencompte, choix_niveau);
            //    threadCAN.sendAck(idprisencompte2 , choix_niveau) ;
            ThisThread::sleep_for(3s);
            ihm.msgBoxClose();

            etat = multi_init;

            break;
        }

        ThisThread::sleep_for(10ms);
    }
}

void run_show()
{
    color = hyou;
    threadCAN.send(simulateur);
    printf("\n\n Envoie pour le simulateur\n");
    printf("\n\n La stratégie est %s  \n", fichiers[choix].c_str());
    printf("\n Couleur de départ : %d\n", color);
    gameEtat = ETAT_GAME_LOAD_NEXT_INSTRUCTION;
    while (machineStrategie())
        ;
    ihm.msgBoxmatchshowclose();
    printf("\n\n Fermeture de l'onglet msg \n ");
    NVIC_SystemReset();
}
bool listeFichiers()
{
    // Vide la liste de fichiers
    fichiers.clear();
    // Attend que la carte SD soit prête
    threadSD.waitReady();
    // Se déplace dans le dossier des stratégies et liste les fichiers présents
    string reply = threadSD.cdName(config["Dossiers"]["strategie"].c_str());
    // Vérifie que le dossier des stratégies existe
    if (reply.find(config["Dossiers"]["strategie"].c_str()) != 0)
    {
        return false;
    }
    // Récupère le résultat sous la forme /chemin*dossier1*dossier2*dossier3:fichier1:fichier2:fichier3?   * pour dossier  : pour fichier  ? pour fin
    // Enlève le ? à la fin
    if (!reply.empty())
    {
        reply.pop_back();
    }
    stringstream txtStream(reply);
    string item;
    // Ignore tous les dossiers
    if (getline(txtStream, item, ':'))
    {
        while (getline(txtStream, item, ':'))
        {
            // Range chaque nom de fichier dans un tableau de string
            fichiers.push_back(item);
        }
    }
    return true;
}

bool lectureFichier(int choix)
{
    string ficStrat;
    string ligne;
    if (choix < 0)
    {
        // Que faire si choix == -1 ????
        return false;
    }
    ficStrat = "/sd" + config["Dossiers"]["strategie"] + "/" + fichiers[choix];
    // ifstream monFlux(ficStrat);  // Ouverture d'un fichier en lecture
    // if (monFlux) {
    //     // Tout est prêt pour la lecture.
    //     while (getline(monFlux, ligne)) {  // On lit une ligne complète
    //         // printf("%s\n", ligne.c_str());
    //         listeInstructions.ajout(ligne.c_str());
    //         // debug_Instruction(listeInstructions.derniere());
    //     }
    //     monFlux.close();
    //     return true;
    // }
    FILE *f = fopen(ficStrat.c_str(), "r"); // Ouverture d'un fichier en lecture
    if (f)
    {
        char data;
        while (fread(&data, 1, 1, f))
        {
            if ((data == '\r') || (data == '\n'))
            {
                if (ligne.length() > 0)
                {
                    printf("%s\n", ligne.c_str());
                    listeInstructions.ajout(ligne.c_str());
                    // debug_Instruction(listeInstructions.derniere());
                    ligne.clear();
                }
            }
            else
            {
                ligne += data;
            }
        }
        fclose(f);
        return true;
    }
    // ERREUR: Impossible d'ouvrir le fichier en lecture
    // On fait la même chose que pour choix == -1 ????
    return false;
}
// bool lectureFichier(int choix) {
//     string ficStrat;
//     if (choix < 0) {
//         // Que faire si choix == -1 ????
//         return false;
//     }
//     ficStrat = "/sd" + config["Dossiers"]["strategie"] + "/" + fichiers[choix];
//     ifstream monFlux(ficStrat);  // Ouverture d'un fichier en lecture
//     if (monFlux) {
//         // Tout est prêt pour la lecture.
//         string ligne;
//         while (getline(monFlux, ligne)) {  // On lit une ligne complète
//             printf("%s\n", ligne.c_str());
//             listeInstructions.ajout(ligne.c_str());
//             // debug_Instruction(listeInstructions.derniere());
//             ThisThread::sleep_for(1ms);
//         }
//         monFlux.close();
//         return true;
//     }
//     // ERREUR: Impossible d'ouvrir le fichier en lecture
//     // On fait la même chose que pour choix == -1 ????
//     return false;
// }
bool jack()
{
    return ((jackPin.read()) || getFlag(JACK, true));
}

// ancien code ------------------------------------------------------------

void runRecalage()
{
    if (machineRecalageInit())
    {
        while (machineRecalage())
            ;
    }
}
void runMatch()
{
    printf("debut du match\n");
    gameEtat = ETAT_GAME_LOAD_NEXT_INSTRUCTION;
    while (machineStrategie())
        ;
    printf("\n\n Fermeture de l'onglet msg \n ");
    ihm.msgBoxmatchshowclose();
}
void forceFinMatch()
{
    flagForceFinMatch = true;
}
int tempsRestant()
{
    return std::chrono::duration_cast<std::chrono::seconds>(timerMatch.remaining_time()).count();
}

// Fonction appelée toutes les 2 secondes par le Ticker
void updateCarteSD()
{
    flagUpdateCarteSD = true;
}

// Fonction helper pour compter les fichiers de stratégie
int countStrategyFiles()
{
    return fichiers.size();
}
