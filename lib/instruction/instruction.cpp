#include <stdio.h>
#include <instruction.h>

ListeInstructions listeInstructions;

EnumInstructionType decodeInstructionType(char type)
{
    switch(type)
    {
        case 'B': return MV_BEZIER;
        case 'C': return MV_COURBURE;
        case 'L': return MV_LINE;
        case 'T': return MV_TURN;
        case 'X': return MV_XYT;
        case 'R': return MV_RECALAGE;
        case 'A': return ACTION;
        case 'H': return PINCE;
        case 'P': return POSITION;
        default:  return UNKNOWN;
    }
}

EnumInstructionDirection decodeInstructionDirection(char type)
{
    switch(type)
    {
        case 'B': return BACKWARD;
        case 'F': return FORWARD;
        case 'R': return RELATIVE;
        case 'A': return ABSOLUTE;
        case 'L': return LEFT;
        case  'AV' : return ASC_AV ; 
        case  'ARR' : return ASC_ARR ; 
        default:  return NODIRECTION;
    } 
}

EnumInstructionPrecisionOuRecalage decodeInstructionPrecisionOuRecalage(char type)
{
    switch(type)
    {
        case 'P': return PRECISION;
        case 'X': return RECALAGE_X;
        case 'Y': return RECALAGE_Y;
        case 'T': return RECALAGE_T;
        default:  return NOPRECISION;
    } 
}

EnumInstructionNextActionType decodeInstructionNextActionType(char type)
{
    switch(type)
    {
        case 'J': return JUMP;
        case 'W': return WAIT;
        case 'E': return ENCHAINEMENT;
        case 'M': return MECANIQUE;
        case 'C': return CAPTEUR;
        default:  return NONEXTACTION;
    } 
}

EnumInstructionNextActionJumpType decodeInstructionNextActionJumpType(char type)
{
    switch(type)
    {
        case 'T': return JUMP_TIME;
        case 'P': return JUMP_POSITION;
        default:  return NONEXTACTIONJUMPTYPE;
    } 
}

/****************************************************************************************/
/* FUNCTION NAME: stringToInstruction                                                   */
/* DESCRIPTION  : Conversion d'une ligne du fichier de strat en instruction             */
/****************************************************************************************/
Instruction ListeInstructions::stringToInstruction(const char *line) {
    Instruction instruction;
    
    char instructionOrder;
    char instructionDirection;
    char instructionPrecision;
    char instructionNextActionType;
    char instructionJumpAction;
    int errorCode = 0;
    /*
    Info sur la fonction sscanf
    %d -> Entier signé
    %u -> Entier non signé
    %c -> char
    */
  errorCode = sscanf(line, "%hd,%c,%c,%hu,%hu,%hd,%c,%c,%c,%hu,%hu,%hd,%hd,%hu,%hu,%hd",
        &instruction.lineNumber,
        &instructionOrder,
        &instructionDirection,
        &instruction.arg1,
        &instruction.arg2,
        &instruction.arg3,
        &instructionPrecision,
        &instructionNextActionType,
        &instructionJumpAction,
        &instruction.JumpTimeOrX,
        &instruction.JumpY,
        &instruction.nextLineOK,
        &instruction.nextLineError,
        &instruction.arg4,
        &instruction.arg5,
        &instruction.arg6
    );
    /*
    if(errorCode != 13) {
        errorInstructionLoop();//L'instruction n'est pas bonne !!  
    }*/
    
    instruction.order           = decodeInstructionType(instructionOrder);
    instruction.direction       = decodeInstructionDirection(instructionDirection);
    instruction.precision       = decodeInstructionPrecisionOuRecalage(instructionPrecision);
    instruction.nextActionType  = decodeInstructionNextActionType(instructionNextActionType);
    instruction.jumpAction      = decodeInstructionNextActionJumpType(instructionJumpAction);
    
    
    return instruction;
}

void ListeInstructions::suivante()
{
    if (!liste[actuelle].nextLineOK || (liste[actuelle].nextLineOK == liste[actuelle].lineNumber)) {
        actuelle++;
    } else {
        actuelle = liste[actuelle].nextLineOK;
    }
}

bool ListeInstructions::fin()
{

    return ((size_t(actuelle) >= liste.size()) || (actuelle == 255));
}
