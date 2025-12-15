#ifndef __CONFIG_H
#define __CONFIG_H

#include <mIni.h>

extern mINI::INIStructure config;

bool readConfig();
bool writeConfig();

#endif
