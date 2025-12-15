#ifndef __INSTRUCTION_H
#define __INSTRUCTION_H

#include <vector>


typedef enum
{
    NO_ASSIETTE,
    HG_ASS_VERTE_CARRE,
    BG_ASS_BLEU_CARRE,
    HC_ASS_BLEU,
    HC_ASS_VERT,
    BC_ASS_BLEU,
    BC_ASS_VERT,
    HD_ASS_BLEU,
    HD_ASS_VERT,
    BD_ASS_BLEU,
    BD_ASS_VERT,
}Assiette;

typedef enum
    {
        Jaune = 0,
        BLEU = 1
    }couleurDepart;


enum EnumInstructionType {
    MV_BEZIER,      // B
    MV_COURBURE,  // C -> Courbure
    MV_LINE,      // L -> Ligne droite
    MV_TURN,      // T -> Rotation sur place
    MV_XYT,       // X -> Aller à
    MV_RECALAGE,  // R -> Recalage bordure
    ACTION,       // A -> Action
    PINCE,          // H
    UNKNOWN,        // Erreur, instruction inconnue
    POSITION       // P
};

enum EnumInstructionDirection {
    NODIRECTION,  // N -> Parametre absent
    BACKWARD,
    FORWARD,
    RELATIVE,
    ABSOLUTE,
    LEFT,
    RIGHT , 
    ASC_ARR , 
    ASC_AV
};

enum EnumInstructionPrecisionOuRecalage {
    NOPRECISION,  // N -> Parametre absent
    PRECISION,    // P -> Precision, verifier la position à la fin du mouvement et refaire un XYT si erreur > 1cm
    RECALAGE_X,   // X -> Recalage en X, indique un recalage sur l'axe X
    RECALAGE_Y,   // Y -> Recalage en Y, indique un recalage sur l'axe Y
    RECALAGE_T
};

enum EnumInstructionNextActionType {
    NONEXTACTION,  // N -> Parametre absent
    JUMP,
    WAIT,
    ENCHAINEMENT,
    MECANIQUE,
    CAPTEUR
};

enum EnumInstructionNextActionJumpType {
    NONEXTACTIONJUMPTYPE,  // N -> Parametre absent
    JUMP_TIME,
    JUMP_POSITION
};

typedef struct
{
    short lineNumber;                  // Numéro de la ligne
    EnumInstructionType order;           // Type de l'instruction
    EnumInstructionDirection direction;  // BackWard ou Forward || Relative ou Absolu

    unsigned short arg1;
    unsigned short arg2;
    signed short arg3;

    EnumInstructionPrecisionOuRecalage precision;
    EnumInstructionNextActionType nextActionType;
    EnumInstructionNextActionJumpType jumpAction;
    unsigned short JumpTimeOrX;
    unsigned short JumpY;
    unsigned short nextLineOK;
    unsigned short nextLineError;

    unsigned short arg4;
    unsigned short arg5;
    signed short arg6;
} Instruction;

struct S_Dodge_queue {
    unsigned short nb;  // Nombre d action en file dattente [0,9]
    Instruction inst[10];
};

EnumInstructionType decodeInstructionType(char type);
EnumInstructionDirection decodeInstructionDirection(char type);
EnumInstructionPrecisionOuRecalage decodeInstructionPrecisionOuRecalage(char type);
EnumInstructionNextActionType decodeInstructionNextActionType(char type);
EnumInstructionNextActionJumpType decodeInstructionNextActionJumpType(char type);


class ListeInstructions {
   private:
    std::vector<Instruction> liste;
    int actuelle;
    /****************************************************************************************/
    /* FUNCTION NAME: stringToInstruction                                                   */
    /* DESCRIPTION  : Conversion d'une ligne du fichier de strat en instruction             */
    /****************************************************************************************/
    static Instruction stringToInstruction(const char *line);

   public:
    ListeInstructions() {
        liste.clear();
        liste.reserve(150);
        actuelle = 0;
    }
    ~ListeInstructions() {
        liste.clear();
        actuelle = 0;
    }
    const Instruction &enCours() { return liste[actuelle]; }
    const Instruction &derniere() { return liste.back(); }
    void ajout(const char *ligne) { liste.push_back(stringToInstruction(ligne)); }
    void vaLigne(int ligne) { actuelle = ligne; }
    void debut() { actuelle = 0; }
    void suivante();
    bool fin();
    int size(){return liste.size();}
    void clear(){liste.clear(); actuelle = 0;}
};

extern ListeInstructions listeInstructions;

#endif