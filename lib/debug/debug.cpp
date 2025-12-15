#include "debug.h"

void debugInstruction(Instruction instruction)
{
    printf("\n********* Debug instruction *********\n");
    
    printf("* Line      => %d\n", instruction.lineNumber); // numéro de ligne 
    printf("* Type      => %s\n", instructionTypeToString(instruction.order)); // mv lin / mv turn / action 
    printf("* Direction => %s\n", instructionDirectionToString(instruction.direction));
    
    printf("* Arg1      => %d\n", instruction.arg1);
    printf("* Arg2      => %d\n", instruction.arg2);
    printf("* Arg3      => %d\n", instruction.arg3);
    
    printf("* Recalage  => %s\n", instructionPrecisionOuRecalageToString(instruction.precision));
    printf("* NextAction=> %s\n", instructionNextActionTypeToString(instruction.nextActionType));
    printf("* JumpAction=> %s\n", instructionNextActionJumpTypeToString(instruction.jumpAction));
    
    printf("* JumpTimeOrX   => %d\n", instruction.JumpTimeOrX);
    printf("* JumpY         => %d\n", instruction.JumpY);
    printf("* nextLineOK    => %d\n", instruction.nextLineOK);
    printf("* nextLineError => %d\n", instruction.nextLineError);
    
    printf("*************************************\n\n");
}

const char* instructionTypeToString(enum EnumInstructionType type)
{
    switch(type)
    {
        case MV_COURBURE:       return "Courbure";
        case MV_LINE:           return "Ligne";
        case MV_TURN:           return "Rotation";
        case MV_XYT:            return "Position XYT";
        case MV_RECALAGE:       return "Recalage";
        case ACTION:            return "Action";
        case PINCE:             return "Pince";
        case POSITION:          return "Positon init" ; 
        default:                return "Inconnue";
    }    
}
const char* instructionDirectionToString(enum EnumInstructionDirection type)
{
    switch(type)
    {
        case BACKWARD:      return "en arriere";
        case FORWARD:       return "en avant";
        case RELATIVE:      return "relatif";
        case ABSOLUTE:      return "absolu";
        case ASC_ARR :      return "face arriere" ; 
        case ASC_AV :       return "face avant " ; 
        default:            return "absent";
    }    
}
const char* instructionPrecisionOuRecalageToString(enum EnumInstructionPrecisionOuRecalage type)
{
    switch(type)
    {
        case PRECISION:     return "correction position en fin de mouvement";
        case RECALAGE_X:    return "recalage en X";
        case RECALAGE_Y:    return "recalage en Y";
        default:            return "absent";
    }    
}
const char* instructionNextActionTypeToString(enum EnumInstructionNextActionType type)
{
    switch(type)
    {
        case JUMP:          return "jump";
        case WAIT:          return "attente fin instruction";
        case ENCHAINEMENT:  return "enchainement";
        default:            return "absent";
    }    
}
const char* instructionNextActionJumpTypeToString(enum EnumInstructionNextActionJumpType type)
{
    switch(type)
    {
        case JUMP_TIME:         return "attente temps";
        case JUMP_POSITION:     return "attente position";
        default:                return "absent";
    }    
}
