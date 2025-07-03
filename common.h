#ifndef LEASELIST_COMMON_H
#define LEASELIST_COMMON_H

#ifdef USE_LIBUSEFUL_BUNDLED
#include "libUseful-bundled/libUseful.h"
#else
#include "libUseful-5/libUseful.h"
#endif

#define FLAG_NOW     1
#define FLAG_TODAY   2
#define FLAG_WEEK  4
#define FLAG_HISTORY 8


typedef struct
{
    char *Name;
    char *VendorID;
    char *MAC;
    char *IP;
    char *Dev;
    time_t FirstSeen;
    time_t LastSeen;
} THost;

extern int Flags;
extern char *LeaseFilesPath;
extern char *CurrYear;

THost *HostCreate(const char *MAC, const char *Name, time_t When);
void HostDestroy(void *p_Host);
void HostListAdd(ListNode *Hosts, THost *Host);
void OutputFileProgress(STREAM *S);

#endif
