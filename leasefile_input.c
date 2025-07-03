#include "leasefile_input.h"


static void LeaseFileGetVendorID(THost *Host, const char *Input)
{
    char *Token=NULL;
    const char *ptr;

    ptr=GetToken(Input, "\\S", &Token, 0);

    if (CompareStr(Token, "vendor-class-identifier")==0)
    {
        ptr=GetToken(ptr, "\\S", &Token, 0);
        ptr=GetToken(ptr, ";", &(Host->VendorID), GETTOKEN_QUOTES);
    }


    Destroy(Token);
}

static time_t LeaseFileGetTime(THost *Host, const char *Input)
{
    time_t When;
    char *Token=NULL, *Date=NULL, *Time=NULL;
    const char *ptr;

    ptr=GetToken(Input, "\\S", &Token, 0);
    ptr=GetToken(ptr, "\\S", &Date, 0);
    ptr=GetToken(ptr, ";", &Time, 0);

    Token=MCopyStr(Token, Date, "T", Time, NULL);
    When=DateStrToSecs("%Y/%m/%dT%H:%M:%S", Token, "GMT");

    Destroy(Token);
    Destroy(Date);
    Destroy(Time);

    return(When);
}




static THost *LeaseFileParseLine(ListNode *Hosts, THost *Host, const char *Input)
{
    char *Tempstr=NULL, *Token=NULL;
    const char *ptr;

    ptr=GetToken(Input, "\\S", &Token, 0);

    if (CompareStr(Token, "lease") == 0)
    {
        if (Host) HostListAdd(Hosts, Host);
        Host=(THost *) HostCreate(NULL, NULL, 0);
        ptr=GetToken(ptr, "\\S", &(Host->IP), 0);
    }
    else if (Host)
    {
        if (CompareStr(Token, "hardware") == 0)
        {
            ptr=GetToken(ptr, "\\S", &Token, 0); //'ethernet'
            ptr=GetToken(ptr, "\\S|;", &(Host->MAC), GETTOKEN_MULTI_SEP);
            //now we have a mac, we can add the host to the hostlist
        }
        else if (CompareStr(Token, "set")==0) LeaseFileGetVendorID(Host, ptr);
        else if (CompareStr(Token, "starts")==0) Host->FirstSeen=LeaseFileGetTime(Host, ptr);
        else if (CompareStr(Token, "ends")==0) Host->LastSeen=LeaseFileGetTime(Host, ptr);
        else if (CompareStr(Token, "client-hostname")==0) ptr=GetToken(ptr, ";", &(Host->Name), GETTOKEN_QUOTES);
    }

    Destroy(Tempstr);
    Destroy(Token);

    return(Host);
}


int LeaseFileRead(const char *Path, ListNode *Hosts)
{
    char *Tempstr=NULL, *Token=NULL;
    const char *ptr;
    THost *Host=NULL;
    STREAM *S;

    S=STREAMOpen(Path, "r");
    if (S)
    {
        Tempstr=STREAMReadLine(Tempstr, S);
        while (Tempstr)
        {
            OutputFileProgress(S);

            StripLeadingWhitespace(Tempstr);
            StripTrailingWhitespace(Tempstr);
            Host=LeaseFileParseLine(Hosts, Host, Tempstr);
            Tempstr=STREAMReadLine(Tempstr, S);
        }

        if (Host) HostListAdd(Hosts, Host);
    }
    STREAMClose(S);
    fprintf(stderr, "\n");

    Destroy(Tempstr);
    Destroy(Token);

    return(ListSize(Hosts));
}


void LeaseFilesFind(const char *Paths, ListNode *Hosts)
{
    char *Token=NULL;
    const char *ptr;

    ptr=GetToken(Paths, ":", &Token, 0);
    while (ptr)
    {
        LeaseFileRead(Token, Hosts);
        ptr=GetToken(ptr, ":", &Token, 0);
    }

    Destroy(Token);

}
