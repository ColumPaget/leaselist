#include "common.h"

int Flags=0;
char *LeaseFilesPath=NULL;
char *CurrYear=NULL;

THost *HostCreate(const char *MAC, const char *Name, time_t When)
{
    THost *Host;

    Host=(THost *) calloc(1, sizeof(THost));
    Host->Dev=CopyStr(Host->Dev, "");
    Host->MAC=CopyStr(Host->MAC, MAC);
    Host->Name=CopyStr(Host->Name, Name);
    Host->IP=CopyStr(Host->IP, "");
    Host->FirstSeen=When;
    Host->LastSeen=When;

    return(Host);
}


void HostDestroy(void *p_Host)
{
    THost *Host;

    Host=(THost *) p_Host;
    Destroy(Host->IP);
    Destroy(Host->Dev);
    Destroy(Host->MAC);
    Destroy(Host->Name);
    free(Host);
}


static char *HostListUpdateStringValue(char *RetStr, const char *Value, int MoreRecent)
{
    if (! StrValid(Value)) return(RetStr);

    if (StrValid(RetStr))
    {
        if (Flags & FLAG_HISTORY)
        {
            if (! InStringList(RetStr, Value, ",")) RetStr=MCatStr(RetStr, ",", Value, NULL);
        }
        else if (MoreRecent) RetStr=CopyStr(RetStr, Value);
    }
    else RetStr=CopyStr(RetStr, Value);

    return(RetStr);
}



void HostListAdd(ListNode *Hosts, THost *Host)
{
    THost *Exist=NULL;
    ListNode *Node;
    int MoreRecent=FALSE;

    if (! StrValid(Host->MAC)) return;
    Node=ListFindNamedItem(Hosts, Host->MAC);
    if (Node) Exist=(THost *) Node->Item;

    if (Exist)
    {
        if ( (Exist->FirstSeen==0) || (Host->FirstSeen < Exist->FirstSeen) ) Exist->FirstSeen=Host->FirstSeen;
        if ( (Exist->LastSeen==0) || (Host->LastSeen > Exist->LastSeen) )
        {
            Exist->LastSeen=Host->LastSeen;
            MoreRecent=TRUE;
        }

        Exist->Name=HostListUpdateStringValue(Exist->Name, Host->Name, MoreRecent);
        Exist->Dev=HostListUpdateStringValue(Exist->Dev, Host->Dev, MoreRecent);
        Exist->IP=HostListUpdateStringValue(Exist->IP, Host->IP, MoreRecent);
        HostDestroy(Host);
    }
    else ListAddNamedItem(Hosts, Host->MAC, Host);
}


void OutputFileProgress(STREAM *S)
{
    size_t pos;

    pos=STREAMTell(S);
    fprintf(stderr, "\rloading: %30s % 6d%%        ", GetBasename(S->Path), (int) (pos * 100 / S->Size));
}

