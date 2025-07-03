#include "common.h"
#include "logfile_input.h"
#include "leasefile_input.h"
#include "oui_maclookup.h"
#include "macnamesfile.h"
#include "settings.h"




void OutputHost(THost *Host, time_t Age, ListNode *OUIMacList)
{
    char *Tempstr=NULL, *First=NULL, *Last=NULL, *Token=NULL;
    char *Name=NULL, *TimePrefix=NULL;
    const char *ptr;


    if (Age < 3600) TimePrefix=CopyStr(TimePrefix, "~w~e");
    else if (Age < (3600 * 24)) TimePrefix=CopyStr(TimePrefix, "~w");
    else TimePrefix=CopyStr(TimePrefix, "~c");


    First=MCopyStr(First, TimePrefix, GetDateStrFromSecs("%Y-%m-%dT%H:%M:%S", Host->FirstSeen, NULL), "~0", NULL);

    Last=MCopyStr(Last, TimePrefix, GetDateStrFromSecs("%Y-%m-%dT%H:%M:%S", Host->LastSeen, NULL), "~0", NULL);

    Name=CopyStr(Name, Host->Name);
    if (StrValid(Host->VendorID))
    {
        if (StrValid(Name)) Name=CatStr(Name, " - ");
        Name=CatStr(Name, Host->VendorID);
    }

    ptr=MacNamesFind(Host->MAC);
    if (StrValid(ptr))
    {
        Name=MCatStr(Name, " - ~m", ptr, "~0", NULL);
    }

    Tempstr=FormatStr(Tempstr, "%s   %s  ~e%s~0 %6s:%-15s ~e~c%-20s~0", First, Last, Host->MAC, Host->Dev, Host->IP, Name);
    if (OUIMacList)
    {
        Token=CopyStrLen(Token, Host->MAC, 8);
        Tempstr=MCatStr(Tempstr, "   ~e~b", GetVar(OUIMacList, Token), "~0", NULL);
    }
    Tempstr=CatStr(Tempstr, "\n");

    TerminalPutStr(Tempstr, NULL);

    Destroy(Tempstr);
    Destroy(First);
    Destroy(Last);
    Destroy(Name);
    Destroy(Token);
}


int IncludeInOutput(time_t Age)
{
    if ( (Flags & FLAG_NOW) && (Age > 10)) return(FALSE);
    if ( (Flags & FLAG_TODAY) && (Age > DAYSECS)) return(FALSE);
    if ( (Flags & FLAG_WEEK) && (Age > DAYSECS * 7)) return(FALSE);

    return(TRUE);
}


#define FT_UNKNOWN      0
#define FT_DHCPLOG_FILE 1
#define FT_LEASE_FILE   2

int DetectFileType(const char *Path)
{
    char *Tempstr=NULL, *Token=NULL;
    const char *ptr;
    int FileType=FT_UNKNOWN;
    STREAM *S;

    S=STREAMOpen(Path, "r");
    if (S)
    {
        Tempstr=STREAMReadLine(Tempstr, S);
        while (Tempstr)
        {
            ptr=GetToken(Tempstr, "\\S", &Token, 0);
            if (StrValid(Token))
            {
                if (strcmp(Token, "lease")==0)
                {
                    FileType=FT_LEASE_FILE;
                    break;
                }
                else if (strstr(Tempstr, "DHCPACK"))
                {
                    FileType=FT_DHCPLOG_FILE;
                    break;
                }
                else if (strstr(Tempstr, "DHCPOFFER"))
                {
                    FileType=FT_DHCPLOG_FILE;
                    break;
                }
                else if (strstr(Tempstr, "DHCPINFORM"))
                {
                    FileType=FT_DHCPLOG_FILE;
                    break;
                }
            }

            Tempstr=STREAMReadLine(Tempstr, S);
        }
        STREAMClose(S);
    }

    Destroy(Tempstr);
    Destroy(Token);

    return(FileType);
}



main(int argc, char *argv[])
{
    char *Tempstr=NULL, *Path=NULL;
    ListNode *Hosts=NULL, *OUIMacList=NULL, *Curr;
    const char *ptr;
    THost *Host;
    time_t Now, Age;


    SettingsInit();
    //this app doesn't need much permissions, so set some strict security
    //if the underlying libuseful supports it
    ProcessApplyConfig("nosu w^x security=untrusted");

    CurrYear=CopyStr(CurrYear, GetDateStr("%Y", NULL));
    ParseCommandLine(argc, argv);

    OUIMacList=OUIMacFileLoad(Settings->OUIMacFiles);
    MacNamesLoad(Settings->NamesFiles);

    Now=GetTime(0);
    Hosts=MapCreate(4096, LIST_FLAG_CACHE);


    if (StrValid(Settings->InputFiles))
    {
        ptr=GetToken(Settings->InputFiles, ":", &Path, 0);
        while (ptr)
        {
            switch (DetectFileType(Path))
            {
            case FT_LEASE_FILE:
                LeaseFilesFind(Path, Hosts);
                break;
            case FT_DHCPLOG_FILE:
                LogFileRead(Path, Hosts);
                break;
            default:
                printf("Unknown File Type: %s\n", Curr->Tag);
            }

            ptr=GetToken(ptr, ":", &Path, 0);
        }
    }
    else LeaseFilesFind(Settings->LeaseFiles, Hosts);


    Curr=ListGetNext(Hosts);
    while (Curr)
    {
        Host=(THost *) Curr->Item;

        Age=Now - Host->LastSeen;

        if (IncludeInOutput(Age))
        {
            OutputHost(Host, Age, OUIMacList);
        }

        Curr=ListGetNext(Curr);
    }

    Destroy(Tempstr);
    Destroy(Path);
}
