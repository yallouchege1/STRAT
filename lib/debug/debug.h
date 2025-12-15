#ifndef CRAC_DEBUG
#define CRAC_DEBUG
//debug
#include "global.h"
#include <instruction.h>

void debugInstruction(Instruction instruction);

const char* instructionTypeToString(enum EnumInstructionType type);

const char* instructionDirectionToString(enum EnumInstructionDirection type);

const char* instructionPrecisionOuRecalageToString(enum EnumInstructionPrecisionOuRecalage type);

const char* instructionNextActionTypeToString(enum EnumInstructionNextActionType type);

const char* instructionNextActionJumpTypeToString(enum EnumInstructionNextActionJumpType type);




#endif