#ifndef LEASELIST_INPUT_H
#define LEASELIST_INPUT_H

#include "common.h"

int LeaseFileRead(const char *Path, ListNode *Hosts);
void LeaseFilesFind(const char *Paths, ListNode *Hosts);


#endif

