#ifndef LEASELIST_SETTINGS_H
#define LEASELIST_SETTINGS_H

#include "common.h"

typedef struct
{
int Flags;
char *InputFiles;
char *OUIMacFiles;
char *NamesFiles;
char *LeaseFiles;
} TSettings;

extern TSettings *Settings;

void SettingsInit();

void PrintHelp();
void ParseCommandLine(int argc, char *argv[]);


#endif

