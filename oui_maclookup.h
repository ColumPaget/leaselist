#ifndef LEASELIST_OUI_H
#define LEASELIST_OUI_H

#include "common.h"

//list of mac-address prefixes and the manufactuers they are booked to
extern char *OUIMacFile;


ListNode *OUIMacFileLoad(const char *OUIMacFile);

#endif
