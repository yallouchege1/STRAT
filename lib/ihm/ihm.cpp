#include <ihm.h>
#include <mbed.h>
#include <threadCAN.h>
#include <instruction.h>

int choix_niveau = 0 ; 
couleurDepart hyou ; 
Ihm::Ihm(ThreadLvgl *t) {
    m_threadLvgl = t;

    t->lock();

    lv_style_init(&styleTitre);
    lv_style_set_text_font(&styleTitre, FONT_LARGE);
    tabView = lv_tabview_create(lv_scr_act(), LV_DIR_TOP, 70);
    lv_obj_set_style_text_font(lv_scr_act(), FONT_NORMAL, 0);
    lv_obj_t *tab_btns = lv_tabview_get_tab_btns(tabView);
    lv_obj_t *label = lv_label_create(tab_btns);
    lv_label_set_text(label, "CRAC2026");
    lv_obj_align(label, LV_ALIGN_TOP_LEFT, 337, 0);

    // Ajout de l'onglet CarteSD
    tabCarteSD = lv_tabview_add_tab(tabView, "CarteSD");
    carteSDInit(tabCarteSD);

    t->unlock();
}

void Ihm::carteSDInit(lv_obj_t *parent) {
    // Titre
    lv_obj_t *titre = lv_label_create(parent);
    lv_label_set_text(titre, LV_SYMBOL_SD_CARD " Détection Carte SD");
    lv_obj_set_style_text_font(titre, FONT_LARGE, 0);
    lv_obj_align(titre, LV_ALIGN_TOP_MID, 0, 20);

    // Spinner (indicateur de chargement animé)
    spinnerCarteSD = lv_spinner_create(parent, 1000, 60);
    lv_obj_set_size(spinnerCarteSD, 120, 120);
    lv_obj_align(spinnerCarteSD, LV_ALIGN_CENTER, 0, -20);

    // Label de statut principal
    labelCarteSDStatus = lv_label_create(parent);
    lv_label_set_text(labelCarteSDStatus, LV_SYMBOL_REFRESH " En attente de la carte SD...");
    lv_obj_set_style_text_font(labelCarteSDStatus, FONT_NORMAL, 0);
    lv_obj_align(labelCarteSDStatus, LV_ALIGN_CENTER, 0, 80);

    // Label pour le nombre de fichiers
    labelCarteSDFileCount = lv_label_create(parent);
    lv_label_set_text(labelCarteSDFileCount, "");
    lv_obj_set_style_text_font(labelCarteSDFileCount, FONT_NORMAL, 0);
    lv_obj_align(labelCarteSDFileCount, LV_ALIGN_CENTER, 0, 130);
}

void Ihm::updateCarteSDStatus(bool detected, int fileCount) {
    m_threadLvgl->lock();

    if (detected) {
        // Masquer le spinner quand la carte SD est détectée
        lv_obj_add_flag(spinnerCarteSD, LV_OBJ_FLAG_HIDDEN);

        // Mettre à jour le message de statut
        lv_label_set_text(labelCarteSDStatus, LV_SYMBOL_OK " Carte SD détectée");
        lv_obj_set_style_text_color(labelCarteSDStatus, lv_palette_main(LV_PALETTE_GREEN), 0);

        // Afficher le nombre de fichiers
        char buf[100];
        sprintf(buf, LV_SYMBOL_FILE " %d fichier%s trouvé%s",
                fileCount,
                (fileCount > 1) ? "s" : "",
                (fileCount > 1) ? "s" : "");
        lv_label_set_text(labelCarteSDFileCount, buf);
        lv_obj_set_style_text_color(labelCarteSDFileCount, lv_palette_main(LV_PALETTE_BLUE), 0);
    } else {
        // Afficher le spinner quand la carte SD n'est pas détectée
        lv_obj_clear_flag(spinnerCarteSD, LV_OBJ_FLAG_HIDDEN);

        // Mettre à jour le message de statut
        lv_label_set_text(labelCarteSDStatus, LV_SYMBOL_WARNING " Carte SD non détectée - En attente...");
        lv_obj_set_style_text_color(labelCarteSDStatus, lv_palette_main(LV_PALETTE_ORANGE), 0);

        // Effacer le compteur de fichiers
        lv_label_set_text(labelCarteSDFileCount, "");
    }

    m_threadLvgl->unlock();
}

void Ihm::show(const vector<string> fichiers) {
    m_threadLvgl->lock();

   // Ajoute un onglet nommé "Show" à l'objet tabView
tabMatch = lv_tabview_add_tab(tabView, "Show");

// Déclaration de la variable label utilisée pour les boutons
lv_obj_t *label;

// Configuration de la grille
static lv_coord_t col_dsc[] = {LV_GRID_FR(5), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};

/* Configuration de la grille
 * Ligne 1 : Hauteur fixe de 30 pixels
 * Ligne 2 : 2 unités de l'espace disponible
 * Ligne 3 : 1 unité de l'espace disponible */
static lv_coord_t row_dsc[] = {30, LV_GRID_FR(2), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};

/* Création d'un conteneur pour la grille dans l'onglet tabMatch */
lv_obj_t *cont = lv_obj_create(tabMatch);
lv_obj_center(cont);  // Centre le conteneur dans l'onglet
lv_obj_update_layout(tabMatch);  // Met à jour la disposition pour s'assurer que les dimensions sont correctes

// Définit la taille du conteneur pour occuper toute la largeur et hauteur disponibles
lv_obj_set_size(cont, lv_obj_get_content_width(tabMatch), lv_obj_get_content_height(tabMatch));


// Applique la description de la grille au conteneur (colonnes et lignes définies précédemment)
lv_obj_set_grid_dsc_array(cont, col_dsc, row_dsc);

/* Création du titre de l'interface */
lv_obj_t *titre = lv_label_create(cont);  // Crée un label dans le conteneur
lv_obj_set_style_text_font(titre, &liberation_24, 0);  // Définit la police du texte
lv_label_set_text(titre, LV_SYMBOL_SD_CARD "Choix Stratégique ");  // Définit le texte avec une icône SD card

// Place le titre dans la grille : Colonne 0 (1 colonne), Ligne 0 (1 ligne)
lv_obj_set_grid_cell(titre, LV_GRID_ALIGN_STRETCH, 0, 1,
                     LV_GRID_ALIGN_STRETCH, 0, 1);

/* Création d'un roller (sélecteur déroulant) */
roller = lv_roller_create(cont);  // Crée un roller dans le conteneur

// Configure les options du roller avec la liste de fichiers
matchRollerSetOptions(fichiers, false);

// Applique la police de texte au roller
lv_obj_set_style_text_font(roller, &liberation_24, 0);

// Définit l'option sélectionnée par défaut (1ère option) sans animation
lv_roller_set_selected(roller, 1, LV_ANIM_OFF);

// Place le roller dans la grille : Colonne 0 (1 colonne), Ligne 1-2 (2 lignes)
lv_obj_set_grid_cell(roller, LV_GRID_ALIGN_STRETCH, 0, 1,
                     LV_GRID_ALIGN_STRETCH, 1, 2);

/* Création d'un bouton checkable "Couleur" */
couleur = lv_btn_create(cont);  // Crée un bouton dans le conteneur

// Crée un label pour le bouton et définit le texte
label = lv_label_create(couleur);
lv_label_set_text(label, "Couleur");

// Rend le bouton "checkable" (sélectionnable/désélectionnable)
lv_obj_add_flag(couleur, LV_OBJ_FLAG_CHECKABLE);

// Définit la couleur du bouton : Vert par défaut, Bleu quand il est activé
lv_obj_set_style_bg_color(couleur, lv_color_make(0, 92, 230),LV_STATE_CHECKED );  // Bleu
lv_obj_set_style_bg_color(couleur, lv_color_make(255, 255, 0), LV_STATE_DEFAULT);  // Jaune

// Centre le label dans le bouton
lv_obj_center(label);

// Place le bouton "Couleur" dans la grille : Colonne 1 (1 colonne), Ligne 0-1 (2 lignes)
lv_obj_set_grid_cell(couleur, LV_GRID_ALIGN_STRETCH, 1, 1,
                     LV_GRID_ALIGN_STRETCH, 0, 2);

/* Création d'un bouton "GO" */
depart = lv_btn_create(cont);  // Crée un bouton dans le conteneur

// Crée un label pour le bouton et définit le texte
label = lv_label_create(depart);
lv_label_set_text(label, "GO");

// Définit la couleur de fond du bouton (rouge)
lv_obj_set_style_bg_color(depart, lv_palette_main(LV_PALETTE_RED), LV_STATE_DEFAULT);

// Centre le label dans le bouton
lv_obj_center(label);

// Place le bouton "GO" dans la grille : Colonne 1 (1 colonne), Ligne 2 (1 ligne)
lv_obj_set_grid_cell(depart, LV_GRID_ALIGN_STRETCH, 1, 1,
                     LV_GRID_ALIGN_STRETCH, 2, 1);

// Ajoute un gestionnaire d'événements pour le bouton "GO" lors d'un clic
lv_obj_add_event_cb(depart, Ihm::eventHandler, LV_EVENT_CLICKED, this);

// Déverrouille le thread LVGL pour permettre d'autres opérations
m_threadLvgl->unlock();

}

void Ihm::matchRollerSetOptions(const vector<string> fichiers, bool lock) {
    string choix;
    for (auto f : fichiers)
        choix += "\n" + f;
    if (lock)
        m_threadLvgl->lock();
    lv_roller_set_options(roller, choix.c_str(), LV_ROLLER_MODE_NORMAL);
    lv_tabview_set_act(tabView, 1, LV_ANIM_ON);
    if (lock)
        m_threadLvgl->unlock();
}
void Ihm::ActionneurInit() {
    m_threadLvgl->lock();

   /*Column 1: 1 unit from the remaining free space
     *Column 2: 1 unit from the remaining free space*/
    static lv_coord_t col[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};

    /*Row 1: 30 pixels
     *Row 2: 1 unit from the remaining free space
     *Row 3: 1 unit from the remaining free space*/
    static lv_coord_t row[] = {30, LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};


    tabActionneur = lv_tabview_add_tab(tabView, "Test Actionneur");
    /*Create a container with grid*/
    lv_obj_t *container = lv_obj_create(tabActionneur);
    lv_obj_center(container);
    lv_obj_update_layout(tabActionneur);
    lv_obj_set_size(container, lv_obj_get_content_width(tabActionneur), lv_obj_get_content_height(tabActionneur));
    lv_obj_set_grid_dsc_array(container, col, row);

    lv_obj_t *titreActionneur = lv_label_create(container);
    lv_obj_set_style_text_font(titreActionneur, &liberation_24, 0);
    lv_label_set_text(titreActionneur, LV_SYMBOL_EDIT " Test/preglage");  // LV_SYMBOL_ENVELOPE LV_SYMBOL_DRIVE LV_SYMBOL_EDIT
    lv_obj_set_grid_cell(titreActionneur, LV_GRID_ALIGN_STRETCH, 0, 1,
                         LV_GRID_ALIGN_STRETCH, 0, 1);

    /* ========================================
     * ANCIENS BOUTONS - RÉFÉRENCE
     * Ne pas supprimer, garder comme template
     * ======================================== */

    /* ---------------------------------------------------------
    // Bouton "Pos Init"
    Pos_init = lv_btn_create(container);
    lv_obj_t *label = lv_label_create( Pos_init);
    lv_label_set_text(label, "Pos Init ");
    lv_obj_add_flag( Pos_init, LV_OBJ_FLAG_CHECKABLE);
    lv_obj_set_style_bg_color( Pos_init, lv_palette_main(LV_PALETTE_LIGHT_BLUE), LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color( Pos_init, lv_palette_main(LV_PALETTE_DEEP_PURPLE), LV_STATE_CHECKED);
    lv_obj_center(label);
    lv_obj_set_grid_cell( Pos_init, LV_GRID_ALIGN_STRETCH, 0, 1,
                         LV_GRID_ALIGN_STRETCH, 1, 1);
    lv_obj_add_event_cb( Pos_init, Ihm::eventHandler, LV_EVENT_CLICKED, this);
    */ // ------------------------------------------------------------

    /* ------------------------------------------------------------
    // Bouton "Test Ventouse"
    // Création d'un bouton
    testventouse = lv_btn_create(container);

    // Création d'un label (texte) pour le bouton
    label = lv_label_create(testventouse );

    // Définit le texte
    lv_label_set_text(label, "Test Ventouse  ");

    // Rend le bouton "checkable"  basculer  (coché/décoché)
    lv_obj_add_flag(testventouse, LV_OBJ_FLAG_CHECKABLE);

    // Définit la couleur de fond
    lv_obj_set_style_bg_color(testventouse, lv_palette_main(LV_PALETTE_LIGHT_BLUE), LV_STATE_DEFAULT);

    // Définit la couleur de fond est "coché" en violet foncé
    lv_obj_set_style_bg_color(testventouse ,  lv_palette_main(LV_PALETTE_DEEP_PURPLE), LV_STATE_CHECKED);

    // Centre le texte au centre
    lv_obj_center(label);

    // Positionne le bouton dans une grille :
    lv_obj_set_grid_cell(testventouse, LV_GRID_ALIGN_STRETCH, 0, 1,
                         LV_GRID_ALIGN_STRETCH, 2, 1);

    // Activer un événements au bouton 'Gradiniveaux1' :
    lv_obj_add_event_cb(testventouse, Ihm::eventHandler, LV_EVENT_CLICKED, this);
    */ // --------------------------------------------------------------------------

    /* ----------------------------------------------------------------------
    // Bouton "Test Construction"
    Gradinniveaux2 = lv_btn_create(container);
    label = lv_label_create(Gradinniveaux2);
    lv_label_set_text(label, "Test Construction");
    lv_obj_add_flag(Gradinniveaux2, LV_OBJ_FLAG_CHECKABLE);
    lv_obj_set_style_bg_color(Gradinniveaux2, lv_palette_main(LV_PALETTE_LIGHT_BLUE), LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(Gradinniveaux2, lv_palette_main(LV_PALETTE_DEEP_PURPLE), LV_STATE_CHECKED);
    lv_obj_center(label);
    lv_obj_set_grid_cell(Gradinniveaux2, LV_GRID_ALIGN_STRETCH, 1, 1,
                         LV_GRID_ALIGN_STRETCH, 1, 1);
    lv_obj_add_event_cb(Gradinniveaux2, Ihm::eventHandler, LV_EVENT_CLICKED, this);
    */ // ----------------------------------------------------------------------

    /* ----------------------------------------------------------------------
    // Bouton "Test Niveaux B"
    niveaux_base= lv_btn_create(container);
    label = lv_label_create(niveaux_base);
    lv_label_set_text(label, "Test Niveaux B");
    lv_obj_add_flag(niveaux_base, LV_OBJ_FLAG_CHECKABLE);
    lv_obj_set_style_bg_color(niveaux_base, lv_palette_main(LV_PALETTE_LIGHT_BLUE), LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(niveaux_base, lv_palette_main(LV_PALETTE_DEEP_PURPLE), LV_STATE_CHECKED);
    lv_obj_center(label);
    lv_obj_set_grid_cell(niveaux_base, LV_GRID_ALIGN_STRETCH, 1, 1,
                         LV_GRID_ALIGN_STRETCH, 2, 1);
    lv_obj_add_event_cb(niveaux_base, Ihm::eventHandler, LV_EVENT_CLICKED, this);
    */ // ----------------------------------------------------------------------------

    /* ----------------------------------------------------------------------------
    // Bouton "Lâcher"
    lacher = lv_btn_create(container);
    label = lv_label_create(lacher);
    lv_label_set_text(label, " Test Lâcher");
    lv_obj_add_flag(lacher, LV_OBJ_FLAG_CHECKABLE);
    lv_obj_set_style_bg_color(lacher, lv_palette_main(LV_PALETTE_LIGHT_BLUE), LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(lacher, lv_palette_main(LV_PALETTE_DEEP_PURPLE), LV_STATE_CHECKED);
    lv_obj_center(label);
    lv_obj_set_grid_cell(lacher, LV_GRID_ALIGN_STRETCH, 2, 1, LV_GRID_ALIGN_STRETCH, 1, 1);
    lv_obj_add_event_cb(lacher, Ihm::eventHandler, LV_EVENT_CLICKED, this);
    */ // ----------------------------------------------------------------------------

    /* ----------------------------------------------------------------------------
    // Bouton "Autre"
    autre = lv_btn_create(container);
    label = lv_label_create(autre);
    lv_label_set_text(label, "Test jpo");
    lv_obj_add_flag(autre, LV_OBJ_FLAG_CHECKABLE);
    lv_obj_set_style_bg_color(autre, lv_palette_main(LV_PALETTE_LIGHT_BLUE), LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(autre, lv_palette_main(LV_PALETTE_DEEP_PURPLE), LV_STATE_CHECKED);
    lv_obj_center(label);
    lv_obj_set_grid_cell(autre, LV_GRID_ALIGN_STRETCH, 2, 1, LV_GRID_ALIGN_STRETCH, 2, 1);
    lv_obj_add_event_cb(autre, Ihm::eventHandler, LV_EVENT_CLICKED, this);
    */ // ----------------------------------------------------------------------------

    /* ========================================
     * NOUVEAUX BOUTONS
     * ======================================== */

    // Bouton "Test Ventouses" - Ouvre le menu de navigation 3 niveaux
    btnTestVentouses = lv_btn_create(container);
    lv_obj_t *label = lv_label_create(btnTestVentouses);
    lv_label_set_text(label, "Test Ventouses");
    lv_obj_add_flag(btnTestVentouses, LV_OBJ_FLAG_CHECKABLE);
    lv_obj_set_style_bg_color(btnTestVentouses, lv_palette_main(LV_PALETTE_LIGHT_BLUE), LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(btnTestVentouses, lv_palette_main(LV_PALETTE_DEEP_PURPLE), LV_STATE_CHECKED);
    lv_obj_center(label);
    lv_obj_set_grid_cell(btnTestVentouses, LV_GRID_ALIGN_STRETCH, 0, 1,
                         LV_GRID_ALIGN_STRETCH, 1, 1);
    lv_obj_add_event_cb(btnTestVentouses, Ihm::eventHandler, LV_EVENT_CLICKED, this);

    m_threadLvgl->unlock();
}
void Ihm::configRollerSetOptions(const vector<string> fichiers, bool lock) {
    string choix = "";
    for (auto f : fichiers)
        choix += f + "\n";
    if (choix != "") choix.pop_back();
    if (lock)
        m_threadLvgl->lock();
    lv_roller_set_options(configRoller, choix.c_str(), LV_ROLLER_MODE_NORMAL);
    if (lock)
        m_threadLvgl->unlock();
}
void Ihm::configStopPlaying() {
    lv_state_t etat = lv_obj_get_state(configPlay);
    if (etat & LV_STATE_CHECKED) {
        lv_obj_clear_state(configPlay, LV_STATE_CHECKED);
        lv_label_set_text(labelPlay, LV_SYMBOL_PLAY);
    }
}
void Ihm::eventHandler(lv_event_t *e) {
    Ihm *ihm = static_cast<Ihm *>(lv_event_get_user_data(e));
    lv_obj_t *emetteur = lv_event_get_current_target(e);

   
    if (emetteur == ihm->testventouse && lv_obj_has_state(emetteur, LV_STATE_CHECKED))
    {
        ihm->flags.set(IHM_FLAG_Testventouse); // Active le flag général lié au bouton
    }

    else if (  emetteur == ihm->Gradinniveaux2  && lv_obj_has_state(emetteur , LV_STATE_CHECKED))
    {
        ihm->flags.set(IHM_FLAG_Gradin_niveaux_2) ;  // je met le flag en lien avec le l'appuie sur le bouton a 1 
     
     
        
    }
    else if ( emetteur == ihm->niveaux_base  && lv_obj_has_state(emetteur , LV_STATE_CHECKED)) 
    {
        ihm->flags.set ( IHM_FLAG_Niveaux_base) ;  // je met le flag en lien avec le l'appuie sur le bouton a 1 
    }
    else if (emetteur == ihm ->  Pos_init && lv_obj_has_state(emetteur , LV_STATE_CHECKED) )
    {
         ihm->flags.set(IHM_FLAG__Position_init) ;
    }
    else if (emetteur == ihm -> autre  && lv_obj_has_state(emetteur , LV_STATE_CHECKED) )
    {
         ihm->flags.set(IHM_FLAG__autre) ;
    }
    else if ( emetteur == ihm-> depart) 
    {
        ihm -> departCouleur = lv_obj_get_state(ihm->couleur) ; 
         ihm -> departStrategie = int(lv_roller_get_selected(ihm->roller))-1  ;
         ihm->flags.set(IHM_FLAG_DEPART) ;  
    }

    else if ( emetteur == ihm-> lacher)
    {
        ihm -> flags.set( IHM_FLAG_Lacher) ;
    }

    // Gestion du bouton Test Ventouses - Ouvre le niveau 1
    else if (emetteur == ihm->btnTestVentouses && lv_obj_has_state(emetteur, LV_STATE_CHECKED))
    {
        ihm->showVentousesNiveau1();
    }

    else if (emetteur == ihm->msgBoxRecalage & lv_msgbox_get_active_btn_text(emetteur)!= nullptr && strcmp(lv_msgbox_get_active_btn_text(emetteur), "Ok") == 0)
    {

            ihm->flags.set(IHM_FLAG__autre);
            lv_msgbox_close(emetteur);

    }
   
 
    

}
bool Ihm::getFlag(IhmFlag f, bool clearIfSet) {
    if (flags.get() & f) {
        if (clearIfSet)
            flags.clear(f);
        return true;
    }
    return false;
}

void Ihm::msgBoxRecalageInit(const string &strategie) {
    m_threadLvgl->lock();
    static const char *btns[] = {"Ok", ""};
    string str = "Stratégie : " + strategie;
    str += string("\nCouleur : ") + ((departCouleur == 0) ? string("Jaune\n") : string("BLEU\n"));
    msgBoxRecalage = lv_msgbox_create(NULL, "Run\n", str.c_str(), btns, true);
    lv_obj_set_size(msgBoxRecalage, 740, 420);
    lv_obj_center(msgBoxRecalage);
    lv_obj_set_style_bg_color(msgBoxRecalage, (departCouleur == 0) ? lv_color_make(255, 255, 0) : lv_color_make(0, 92, 230), 0);
    lv_obj_set_style_text_color(msgBoxRecalage, lv_color_white(), 0);
    lv_obj_t *boxbtns = lv_msgbox_get_btns(msgBoxRecalage);
    lv_obj_update_layout(msgBoxRecalage);
    lv_obj_set_width(boxbtns, lv_obj_get_content_width(msgBoxRecalage));
    lv_obj_set_height(boxbtns, lv_obj_get_content_height(msgBoxRecalage) - lv_obj_get_y(boxbtns));
    lv_obj_align(boxbtns, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_add_event_cb(msgBoxRecalage, Ihm::eventHandler, LV_EVENT_VALUE_CHANGED, this);
    m_threadLvgl->unlock();
}

void Ihm::msgBoxRecalageClose() {
    m_threadLvgl->lock();
    lv_msgbox_close(msgBoxRecalage);
    msgBoxRecalage = nullptr;
    m_threadLvgl->unlock();
}

void Ihm::msgBoxJackInit() {
    m_threadLvgl->lock();
    static const char *btns[] = {"Jack simulé", ""};
    msgBoxJack = lv_msgbox_create(NULL, "\n" LV_SYMBOL_WARNING " Attention au départ " LV_SYMBOL_WARNING "\n", "Tirer le jack pour démarrer !\n", btns, true);
    lv_obj_set_size(msgBoxJack, 740, 420);
    lv_obj_center(msgBoxJack);
    lv_obj_t *titre = lv_msgbox_get_title(msgBoxJack);
    lv_obj_set_style_text_color(titre, lv_color_make(255, 0, 0), 0);
    lv_obj_t *boxbtns = lv_msgbox_get_btns(msgBoxJack);
    lv_obj_update_layout(msgBoxJack);
    lv_obj_set_width(boxbtns, lv_obj_get_content_width(msgBoxJack));
    lv_obj_set_height(boxbtns, lv_obj_get_content_height(msgBoxJack) - lv_obj_get_y(boxbtns));
    lv_obj_align(boxbtns, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_add_event_cb(msgBoxJack, eventHandler, LV_EVENT_VALUE_CHANGED, this);
    lv_obj_add_event_cb(msgBoxJack, eventHandler, LV_EVENT_DELETE, this);
    m_threadLvgl->unlock();
}

void Ihm::msgBoxJackClose() {
    m_threadLvgl->lock();
    lv_msgbox_close(msgBoxJack);
    // Efface le flag de fermeture de la message box
    jackAnnuleClicked();
    msgBoxJack = nullptr;
    m_threadLvgl->unlock();
}

void Ihm::msgBoxInit(const char *titre, const char *msg, bool boutonAnnuler) {
    m_threadLvgl->lock();
    static const char *btns[] = {""};
    msgBox = lv_msgbox_create(NULL, titre, msg, btns, boutonAnnuler);
    lv_obj_set_size(msgBox, 740, 420);
    lv_obj_center(msgBox);
    lv_obj_t *titreObj = lv_msgbox_get_title(msgBox);
    lv_obj_set_style_text_color(titreObj, lv_color_make(255, 0, 0), 0);
    lv_obj_add_event_cb(msgBox, eventHandler, LV_EVENT_DELETE, this);
    m_threadLvgl->unlock();
}

void Ihm::msgBoxClose() {
    m_threadLvgl->lock();
    lv_msgbox_close(msgBox);
    // Efface le flag de fermeture de la message box
    msgBoxCancelClicked();
    msgBox = nullptr;
    m_threadLvgl->unlock();
}

void Ihm::msgBoxMessage(const char *msg) {
    if (msgBox) {
        m_threadLvgl->lock();
        lv_obj_t *txt = lv_msgbox_get_text(msgBox);
        lv_label_set_text(txt, msg);
        m_threadLvgl->unlock();
    }
}

void Ihm::msgBoxmatchshow( const string &strategie)
{
    m_threadLvgl->lock();
    static const char *btns[] = {""};
    string str = "Stratégie : " + strategie;
    str += string("\nCouleur : ") + ((departCouleur == 0) ? string("Jaune\n") : string("BLEU\n"));
    if ( departCouleur == 1 )
    {
       hyou = BLEU ; 
    }
    else
    {
        hyou = Jaune ; 
    }
    msgBoxmatchmessage = lv_msgbox_create(NULL, "Run\n", str.c_str(), btns, true);
    lv_obj_set_size(msgBoxmatchmessage, 740, 420);
    lv_obj_center(msgBoxmatchmessage);
    lv_obj_set_style_bg_color(msgBoxmatchmessage, (departCouleur == 0) ? lv_color_make(255, 255, 0) : lv_color_make(0, 92, 230), 0);
    lv_obj_set_style_text_color(msgBoxmatchmessage, lv_color_white(), 0);
    lv_obj_t *boxbtns = lv_msgbox_get_btns(msgBoxmatchmessage);
    lv_obj_update_layout(msgBoxmatchmessage);
    lv_obj_add_event_cb(msgBoxmatchmessage, Ihm::eventHandler, LV_EVENT_VALUE_CHANGED, this);
    m_threadLvgl->unlock();

} 

void Ihm::msgBoxmatchshowclose () 
{
    m_threadLvgl->lock();
    if (msgBoxmatchmessage != NULL) {
        lv_obj_del(msgBoxmatchmessage);
        msgBoxmatchmessage = NULL;
    }
    m_threadLvgl->unlock();
}

static lv_obj_t *msgBoxSelection;
static lv_obj_t *msgBoxSelectionascenceur; 
static lv_obj_t *msgBoxmatchmessage ; 
static int selected_level = 0;  // Variable stockant le choix
// Fonction pour fermer la boîte de sélection
static int selected_ascenceur = 0 ; 
void Ihm::showButtonSelectionBoxClose() {
    if (msgBoxSelection != NULL) {
        lv_obj_del(msgBoxSelection);
        msgBoxSelection = NULL;
    }
}

// Fonction appelée quand un bouton est pressé
static void selection_event_handler(lv_event_t * e) {
    lv_obj_t *btn = lv_event_get_target(e);
    const char *btn_txt = lv_msgbox_get_active_btn_text(msgBoxSelection);

    if (strcmp(btn_txt, "Niveaux 1") == 0) {
        selected_level = 1;
    } else if (strcmp(btn_txt, "Niveaux 2") == 0) {
        selected_level = 2;
    } else if (strcmp(btn_txt, "Stable") == 0) {
        selected_level = 3;
    } else if (strcmp(btn_txt, "Tout") == 0) {
        selected_level = 4;
    } else if (strcmp(btn_txt, "Aucun") == 0) {
        selected_level = 0;
    }
    else if (strcmp(btn_txt, "Ventouse bas") == 0  )
    {
        selected_level = 1;
    }
    else if (strcmp(btn_txt, " Ventouse haut") == 0 )
    {
        selected_level = 2;
    }
    else if (strcmp(btn_txt, "Tt Ventouse" ) == 0 )
    {
        selected_level = 3;
    }




}

// Fonction pour afficher la boîte de sélection
void Ihm::showButtonSelectionBox() {
    static const char *btns[] = {
        "Niveaux 1", "Niveaux 2", "Stable", "Tout", "\n",  // Première ligne
        "Ventouse bas"," Ventouse haut", "Tt Ventouse", "Aucun", ""  // Deuxième ligne avec dernier élément vide
    };

    msgBoxSelection = lv_msgbox_create(lv_scr_act(), "Sélectionner un niveau", "Choisissez un paramètre et attendez un délai de 5s :", btns, true);
    lv_obj_set_size(msgBoxSelection, 740, 420);
    lv_obj_add_event_cb(msgBoxSelection, selection_event_handler, LV_EVENT_VALUE_CHANGED, NULL);

    // Centrage
    lv_obj_center(msgBoxSelection);

    // Récupération du conteneur des boutons
    lv_obj_t *btnmatrix = lv_msgbox_get_btns(msgBoxSelection);
    if (btnmatrix) {
        lv_obj_set_width(btnmatrix, lv_pct(90)); // Ajuste la largeur à 90% de la msgbox
        lv_obj_set_flex_flow(btnmatrix, LV_FLEX_FLOW_ROW_WRAP); // Organisation en ligne avec retour automatique
        lv_obj_set_style_pad_row(btnmatrix, 20, 0);  // Augmente l'espacement vertical entre les boutons
        lv_obj_set_style_pad_column(btnmatrix, 15, 0); // Augmente l'espacement horizontal
        lv_obj_set_style_min_width(btnmatrix, 160, 0); // Augmente la largeur minimale des boutons
        lv_obj_set_style_min_height(btnmatrix, 80, 0); // Augmente la hauteur minimale des boutons
    }
}





static void selection_ascenseur_event_handler (lv_event_t * e )
{
    lv_obj_t *btn = lv_event_get_target(e);
    const char *btn_txt = lv_msgbox_get_active_btn_text(msgBoxSelectionascenceur);

    if (strcmp(btn_txt, "Avant") == 0) {
        selected_ascenceur = 1;
    } else if (strcmp(btn_txt,  "Arriére") == 0) {
        selected_ascenceur = 2;
    } else if (strcmp(btn_txt, "Les Deux") == 0) {
        selected_ascenceur = 3;
    } else if ( strcmp(btn_txt,"annuler")==0){
        selected_ascenceur = 4 ;
    }

    
    
}

void Ihm::showButtonascenceurBoxClose() {
    if (msgBoxSelectionascenceur != NULL) {
        lv_obj_del(msgBoxSelectionascenceur);
        msgBoxSelectionascenceur = NULL;
    }
}

void Ihm::showButtonascenceurBox(){
    static const char *btns[] = {"Avant", "Arriére" , "Les Deux","annuler", ""}; // Dernier élément vide

    msgBoxSelectionascenceur  = lv_msgbox_create(lv_scr_act(), "Sélectionner une partie du robot", "Une fois selectionnée attendre un delai de 5s :", btns, true);
    lv_obj_set_size(msgBoxSelectionascenceur, 740, 420);
    lv_obj_add_event_cb(msgBoxSelectionascenceur, selection_ascenseur_event_handler, LV_EVENT_VALUE_CHANGED, NULL);

    lv_obj_center(msgBoxSelectionascenceur);
    // Récupération du conteneur des boutons
    lv_obj_t *btnmatrix = lv_msgbox_get_btns(msgBoxSelectionascenceur);
    if (btnmatrix) {
        lv_obj_set_width(btnmatrix, lv_pct(90)); // Ajuste la largeur à 90% de la msgbox
        lv_obj_set_flex_flow(btnmatrix, LV_FLEX_FLOW_ROW_WRAP); // Organisation en ligne avec retour automatique
        lv_obj_set_style_pad_row(btnmatrix, 10, 0);  // Espacement vertical entre les boutons
        lv_obj_set_style_pad_column(btnmatrix, 10, 0); // Espacement horizontal
    }

}

/* ============================================================================
 * SYSTÈME DE TEST VENTOUSES - NAVIGATION 3 NIVEAUX
 * ============================================================================ */

// Variables statiques pour stocker le contexte de navigation
static Ihm *ventousesIhmContext = nullptr;

// Callback pour les boutons du NIVEAU 1 (Gauche/Droite/Les deux)
static void ventousesNiveau1EventHandler(lv_event_t *e) {
    if (ventousesIhmContext == nullptr) return;

    lv_obj_t *btn = lv_event_get_target(e);
    int cote = (int)(intptr_t)lv_event_get_user_data(e);

    ventousesIhmContext->ventousesCote = cote;
    ventousesIhmContext->showVentousesNiveau2(cote);
}

// Callback pour les boutons du NIVEAU 2 (Ventouse 1/2/3/4/Annuler)
static void ventousesNiveau2EventHandler(lv_event_t *e) {
    if (ventousesIhmContext == nullptr) return;

    lv_obj_t *btn = lv_event_get_target(e);
    int action = (int)(intptr_t)lv_event_get_user_data(e);

    if (action == -1) {
        // Annuler - Retour à l'onglet Test Actionneur
        ventousesIhmContext->closeVentousesMenu();
    } else {
        // Sélection d'une ventouse (1, 2, 3, ou 4)
        ventousesIhmContext->ventousesNumero = action;
        ventousesIhmContext->showVentousesNiveau3(ventousesIhmContext->ventousesCote, action);
    }
}

// Callback pour les boutons du NIVEAU 3 (Attraper/Lâcher/Les deux/Annuler)
static void ventousesNiveau3EventHandler(lv_event_t *e) {
    if (ventousesIhmContext == nullptr) return;

    lv_obj_t *btn = lv_event_get_target(e);
    int action = (int)(intptr_t)lv_event_get_user_data(e);

    if (action == -1) {
        // Annuler - Retour au niveau 2
        ventousesIhmContext->showVentousesNiveau2(ventousesIhmContext->ventousesCote);
    } else {
        // Action finale (Attraper=0, Lâcher=1, Les deux=2)
        ventousesIhmContext->ventousesAction = action;
        ventousesIhmContext->executeVentouse(
            ventousesIhmContext->ventousesCote,
            ventousesIhmContext->ventousesNumero,
            action
        );
    }
}

// NIVEAU 1: Sélection du côté (Gauche/Droite/Les deux)
void Ihm::showVentousesNiveau1() {
    m_threadLvgl->lock();

    ventousesIhmContext = this;

    // Créer le container pour le menu
    if (ventousesContainer != nullptr) {
        lv_obj_del(ventousesContainer);
    }

    ventousesContainer = lv_obj_create(tabActionneur);
    lv_obj_set_size(ventousesContainer, lv_obj_get_content_width(tabActionneur),
                    lv_obj_get_content_height(tabActionneur));
    lv_obj_center(ventousesContainer);

    // Configuration de la grille (2 colonnes)
    static lv_coord_t col[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    static lv_coord_t row[] = {50, LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    lv_obj_set_grid_dsc_array(ventousesContainer, col, row);

    // Titre
    lv_obj_t *titre = lv_label_create(ventousesContainer);
    lv_obj_set_style_text_font(titre, FONT_LARGE, 0);
    lv_label_set_text(titre, "Test Ventouses - Sélection côté");
    lv_obj_set_grid_cell(titre, LV_GRID_ALIGN_CENTER, 0, 2, LV_GRID_ALIGN_CENTER, 0, 1);

    // Bouton "Gauche"
    lv_obj_t *btnGauche = lv_btn_create(ventousesContainer);
    lv_obj_t *labelGauche = lv_label_create(btnGauche);
    lv_label_set_text(labelGauche, "Gauche");
    lv_obj_center(labelGauche);
    lv_obj_set_style_bg_color(btnGauche, lv_palette_main(LV_PALETTE_LIGHT_BLUE), LV_STATE_DEFAULT);
    lv_obj_set_grid_cell(btnGauche, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 1, 1);
    lv_obj_add_event_cb(btnGauche, ventousesNiveau1EventHandler, LV_EVENT_CLICKED, (void*)0);

    // Bouton "Droite"
    lv_obj_t *btnDroite = lv_btn_create(ventousesContainer);
    lv_obj_t *labelDroite = lv_label_create(btnDroite);
    lv_label_set_text(labelDroite, "Droite");
    lv_obj_center(labelDroite);
    lv_obj_set_style_bg_color(btnDroite, lv_palette_main(LV_PALETTE_LIGHT_BLUE), LV_STATE_DEFAULT);
    lv_obj_set_grid_cell(btnDroite, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, 1, 1);
    lv_obj_add_event_cb(btnDroite, ventousesNiveau1EventHandler, LV_EVENT_CLICKED, (void*)1);

    // Bouton "Les deux"
    lv_obj_t *btnLesDeux = lv_btn_create(ventousesContainer);
    lv_obj_t *labelLesDeux = lv_label_create(btnLesDeux);
    lv_label_set_text(labelLesDeux, "Les deux");
    lv_obj_center(labelLesDeux);
    lv_obj_set_style_bg_color(btnLesDeux, lv_palette_main(LV_PALETTE_GREEN), LV_STATE_DEFAULT);
    lv_obj_set_grid_cell(btnLesDeux, LV_GRID_ALIGN_STRETCH, 0, 2, LV_GRID_ALIGN_STRETCH, 2, 1);
    lv_obj_add_event_cb(btnLesDeux, ventousesNiveau1EventHandler, LV_EVENT_CLICKED, (void*)2);

    m_threadLvgl->unlock();
}

// NIVEAU 2: Sélection de la ventouse (1/2/3/4/Annuler)
void Ihm::showVentousesNiveau2(int cote) {
    m_threadLvgl->lock();

    // Supprimer l'ancien container
    if (ventousesContainer != nullptr) {
        lv_obj_del(ventousesContainer);
    }

    ventousesContainer = lv_obj_create(tabActionneur);
    lv_obj_set_size(ventousesContainer, lv_obj_get_content_width(tabActionneur),
                    lv_obj_get_content_height(tabActionneur));
    lv_obj_center(ventousesContainer);

    // Configuration de la grille (2 colonnes, 3 lignes)
    static lv_coord_t col[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    static lv_coord_t row[] = {50, LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    lv_obj_set_grid_dsc_array(ventousesContainer, col, row);

    // Titre avec indication du côté sélectionné
    lv_obj_t *titre = lv_label_create(ventousesContainer);
    lv_obj_set_style_text_font(titre, FONT_LARGE, 0);
    const char *coteStr = (cote == 0) ? "Gauche" : (cote == 1) ? "Droite" : "Les deux";
    char titreTxt[100];
    sprintf(titreTxt, "Ventouses %s - Numéro", coteStr);
    lv_label_set_text(titre, titreTxt);
    lv_obj_set_grid_cell(titre, LV_GRID_ALIGN_CENTER, 0, 2, LV_GRID_ALIGN_CENTER, 0, 1);

    // Boutons Ventouse 1, 2, 3, 4
    const char *labels[] = {"Ventouse 1", "Ventouse 2", "Ventouse 3", "Ventouse 4"};
    for (int i = 0; i < 4; i++) {
        lv_obj_t *btn = lv_btn_create(ventousesContainer);
        lv_obj_t *label = lv_label_create(btn);
        lv_label_set_text(label, labels[i]);
        lv_obj_center(label);
        lv_obj_set_style_bg_color(btn, lv_palette_main(LV_PALETTE_LIGHT_BLUE), LV_STATE_DEFAULT);

        int col_pos = i % 2;
        int row_pos = 1 + (i / 2);
        lv_obj_set_grid_cell(btn, LV_GRID_ALIGN_STRETCH, col_pos, 1, LV_GRID_ALIGN_STRETCH, row_pos, 1);
        lv_obj_add_event_cb(btn, ventousesNiveau2EventHandler, LV_EVENT_CLICKED, (void*)(intptr_t)(i + 1));
    }

    // Bouton "Annuler"
    lv_obj_t *btnAnnuler = lv_btn_create(ventousesContainer);
    lv_obj_t *labelAnnuler = lv_label_create(btnAnnuler);
    lv_label_set_text(labelAnnuler, "Annuler");
    lv_obj_center(labelAnnuler);
    lv_obj_set_style_bg_color(btnAnnuler, lv_palette_main(LV_PALETTE_RED), LV_STATE_DEFAULT);
    lv_obj_set_grid_cell(btnAnnuler, LV_GRID_ALIGN_STRETCH, 0, 2, LV_GRID_ALIGN_STRETCH, 3, 1);
    lv_obj_add_event_cb(btnAnnuler, ventousesNiveau2EventHandler, LV_EVENT_CLICKED, (void*)-1);

    m_threadLvgl->unlock();
}

// NIVEAU 3: Sélection de l'action (Attraper/Lâcher/Les deux/Annuler)
void Ihm::showVentousesNiveau3(int cote, int numero) {
    m_threadLvgl->lock();

    // Supprimer l'ancien container
    if (ventousesContainer != nullptr) {
        lv_obj_del(ventousesContainer);
    }

    ventousesContainer = lv_obj_create(tabActionneur);
    lv_obj_set_size(ventousesContainer, lv_obj_get_content_width(tabActionneur),
                    lv_obj_get_content_height(tabActionneur));
    lv_obj_center(ventousesContainer);

    // Configuration de la grille (2 colonnes)
    static lv_coord_t col[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    static lv_coord_t row[] = {50, LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    lv_obj_set_grid_dsc_array(ventousesContainer, col, row);

    // Titre avec récapitulatif
    lv_obj_t *titre = lv_label_create(ventousesContainer);
    lv_obj_set_style_text_font(titre, FONT_LARGE, 0);
    const char *coteStr = (cote == 0) ? "Gauche" : (cote == 1) ? "Droite" : "Les deux";
    char titreTxt[100];
    sprintf(titreTxt, "%s - V%d - Action", coteStr, numero);
    lv_label_set_text(titre, titreTxt);
    lv_obj_set_grid_cell(titre, LV_GRID_ALIGN_CENTER, 0, 2, LV_GRID_ALIGN_CENTER, 0, 1);

    // Bouton "Attraper"
    lv_obj_t *btnAttraper = lv_btn_create(ventousesContainer);
    lv_obj_t *labelAttraper = lv_label_create(btnAttraper);
    lv_label_set_text(labelAttraper, "Attraper");
    lv_obj_center(labelAttraper);
    lv_obj_set_style_bg_color(btnAttraper, lv_palette_main(LV_PALETTE_GREEN), LV_STATE_DEFAULT);
    lv_obj_set_grid_cell(btnAttraper, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 1, 1);
    lv_obj_add_event_cb(btnAttraper, ventousesNiveau3EventHandler, LV_EVENT_CLICKED, (void*)0);

    // Bouton "Lâcher"
    lv_obj_t *btnLacher = lv_btn_create(ventousesContainer);
    lv_obj_t *labelLacher = lv_label_create(btnLacher);
    lv_label_set_text(labelLacher, "Lâcher");
    lv_obj_center(labelLacher);
    lv_obj_set_style_bg_color(btnLacher, lv_palette_main(LV_PALETTE_ORANGE), LV_STATE_DEFAULT);
    lv_obj_set_grid_cell(btnLacher, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, 1, 1);
    lv_obj_add_event_cb(btnLacher, ventousesNiveau3EventHandler, LV_EVENT_CLICKED, (void*)1);

    // Bouton "Les deux"
    lv_obj_t *btnLesDeux = lv_btn_create(ventousesContainer);
    lv_obj_t *labelLesDeux = lv_label_create(btnLesDeux);
    lv_label_set_text(labelLesDeux, "Les deux");
    lv_obj_center(labelLesDeux);
    lv_obj_set_style_bg_color(btnLesDeux, lv_palette_main(LV_PALETTE_BLUE), LV_STATE_DEFAULT);
    lv_obj_set_grid_cell(btnLesDeux, LV_GRID_ALIGN_STRETCH, 0, 2, LV_GRID_ALIGN_STRETCH, 2, 1);
    lv_obj_add_event_cb(btnLesDeux, ventousesNiveau3EventHandler, LV_EVENT_CLICKED, (void*)2);

    // Bouton "Annuler"
    lv_obj_t *btnAnnuler = lv_btn_create(ventousesContainer);
    lv_obj_t *labelAnnuler = lv_label_create(btnAnnuler);
    lv_label_set_text(labelAnnuler, "Annuler");
    lv_obj_center(labelAnnuler);
    lv_obj_set_style_bg_color(btnAnnuler, lv_palette_main(LV_PALETTE_RED), LV_STATE_DEFAULT);
    lv_obj_set_grid_cell(btnAnnuler, LV_GRID_ALIGN_STRETCH, 0, 2, LV_GRID_ALIGN_STRETCH, 3, 1);
    lv_obj_add_event_cb(btnAnnuler, ventousesNiveau3EventHandler, LV_EVENT_CLICKED, (void*)-1);

    m_threadLvgl->unlock();
}

// Fonction d'exécution finale - Appelée quand l'utilisateur valide une action
void Ihm::executeVentouse(int cote, int numero, int action) {
    // Convertir les codes en chaînes pour le debug
    const char *coteStr = (cote == 0) ? "gauche" : (cote == 1) ? "droite" : "les_deux";
    const char *actionStr = (action == 0) ? "attraper" : (action == 1) ? "lacher" : "les_deux";

    printf("[Test Ventouses] Exécution : Côté=%s, Numéro=%d, Action=%s\n",
           coteStr, numero, actionStr);

    // TODO: Ajouter ici l'envoi de commandes CAN ou autres actions matérielles
    // Exemple:
    // if (action == 0 || action == 2) { // Attraper ou Les deux
    //     commandeVentouseAttraper(cote, numero);
    // }
    // if (action == 1 || action == 2) { // Lâcher ou Les deux
    //     commandeVentouseLacher(cote, numero);
    // }

    // Retour à l'onglet Test Actionneur après l'exécution
    closeVentousesMenu();
}

// Ferme le menu et retourne à l'onglet Test Actionneur
void Ihm::closeVentousesMenu() {
    m_threadLvgl->lock();

    if (ventousesContainer != nullptr) {
        lv_obj_del(ventousesContainer);
        ventousesContainer = nullptr;
    }

    // Décocher le bouton Test Ventouses
    if (btnTestVentouses != nullptr) {
        lv_obj_clear_state(btnTestVentouses, LV_STATE_CHECKED);
    }

    ventousesIhmContext = nullptr;

    m_threadLvgl->unlock();
}




