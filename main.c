#include "common.h"
#include "logfile_input.h"
#include "leasefile_input.h"
#include "oui_maclookup.h"


void PrintHelp()
{

    printf("usage: leaselist [options] [path] [path] ...\n");
    printf("  -M <path>      - path to oui.txt file containing mac-address to vendor mapping. Supplying this activates host vendor output\n");
    printf("  -h             - print lists of historical Names/IP-Addresses of hosts\n");
    printf("  -n             - show still-active leases\n");
    printf("  -now           - show still-active leases\n");
    printf("  -t             - show leases given today\n");
    printf("  -today         - show leases given today\n");
    printf("  -w             - show leases given this week\n");
    printf("  -week          - show leases given this week\n");
    printf("  -help          - print this help\n");
    printf("  --help         - print this help\n");
    printf("  -?             - print this help\n");

    exit(0);
}



ListNode *ParseCommandLine(int argc, char *argv[])
{
    CMDLINE *CMD;
    ListNode *InputFiles;
    const char *item;

    InputFiles=ListCreate();
    CMD=CommandLineParserCreate(argc, argv);

    item=CommandLineNext(CMD);
    while (item)
    {
        if (strcmp(item, "-M")==0) OUIMacFile=CopyStr(OUIMacFile, CommandLineNext(CMD));
        else if (strcmp(item, "-h")==0) Flags |= FLAG_HISTORY;
        else if (strcmp(item, "-n")==0) Flags |= FLAG_NOW;
        else if (strcmp(item, "-now")==0) Flags |= FLAG_NOW;
        else if (strcmp(item, "-t")==0) Flags |= FLAG_TODAY;
        else if (strcmp(item, "-today")==0) Flags |= FLAG_TODAY;
        else if (strcmp(item, "-w")==0) Flags |= FLAG_WEEK;
        else if (strcmp(item, "-week")==0) Flags |= FLAG_WEEK;
        else if (strcmp(item, "-help")==0) PrintHelp();
        else if (strcmp(item, "--help")==0) PrintHelp();
        else if (strcmp(item, "-?")==0) PrintHelp();
        else ListAddNamedItem(InputFiles, item, NULL);
        item=CommandLineNext(CMD);
    }

    return(InputFiles);
}



void OutputHost(THost *Host, int IsRecent, ListNode *OUIMacList)
{
    char *Tempstr=NULL, *First=NULL, *Last=NULL, *Token=NULL;

    if (IsRecent) First=MCopyStr(First, "~e~w", GetDateStrFromSecs("%Y-%m-%dT%H:%M:%S", Host->FirstSeen, NULL), "~0", NULL);
    else First=CopyStr(First, GetDateStrFromSecs("~c%Y-%m-%dT%H:%M:%S~0", Host->FirstSeen, NULL));

    if (IsRecent) Last=MCopyStr(Last, "~e~w", GetDateStrFromSecs("%Y-%m-%dT%H:%M:%S", Host->LastSeen, NULL), "~0", NULL);
    else Last=CopyStr(Last, GetDateStrFromSecs("~m%Y-%m-%dT%H:%M:%S~0", Host->LastSeen, NULL));

    Tempstr=FormatStr(Tempstr, "%s   %s  ~e%s~0 %6s:%-15s ~e~c%-20s~0", First, Last, Host->MAC, Host->Dev, Host->IP, Host->Name);
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
    char *Tempstr=NULL;
    ListNode *InputFiles=NULL, *Hosts=NULL, *OUIMacList=NULL, *Curr;
    THost *Host;
    time_t Now, Age;
    int IsRecent=FALSE;

    //this app doesn't need much permissions, so set some strict security
    //if the underlying libuseful supports it
    ProcessApplyConfig("nosu w^x security=untrusted");

    CurrYear=CopyStr(CurrYear, GetDateStr("%Y", NULL));
    LeaseFilesPath=CopyStr(LeaseFilesPath, "/var/state/dhcp/dhcpd.leases:/var/lib/dhcpd/dhcpd.leases");
    InputFiles=ParseCommandLine(argc, argv);

    Now=GetTime(0);
    Hosts=MapCreate(4096, LIST_FLAG_CACHE);
    if (StrValid(OUIMacFile)) OUIMacList=OUIMacFileLoad(OUIMacFile);


    if (ListSize(InputFiles) > 0)
    {
        Curr=ListGetNext(InputFiles);
        while (Curr)
        {
            switch (DetectFileType(Curr->Tag))
	    {
		case FT_LEASE_FILE: LeaseFilesFind(Curr->Tag, Hosts); break;
		case FT_DHCPLOG_FILE: LogFileRead(Curr->Tag, Hosts); break;
		default: printf("Unknown File Type: %s\n", Curr->Tag);
            }

            Curr=ListGetNext(Curr);
        }
    }
    else LeaseFilesFind(LeaseFilesPath, Hosts);


    Curr=ListGetNext(Hosts);
    while (Curr)
    {
        Host=(THost *) Curr->Item;

        Age=Now - Host->LastSeen;

        if (IncludeInOutput(Age))
        {
            if (Age < (3600 * 24)) IsRecent=TRUE;
            else IsRecent=FALSE;

            OutputHost(Host, IsRecent, OUIMacList);
        }

        Curr=ListGetNext(Curr);
    }

    Destroy(Tempstr);
}
