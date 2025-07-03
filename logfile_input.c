#include "logfile_input.h"


/*
2025-04-10 10:03:13.0 dhcpd@ daemon.info DHCPACK on 10.2.255.76 to 44:33:4c:cc:33:16 (wlan0) via eth2
2025-04-10 10:03:15.0 dhcpd@ daemon.info DHCPINFORM from 192.168.5.252 via eth3: not authoritative for subnet 192.168.5.0
2025-04-10 10:03:31.0 dhcpd@ daemon.info DHCPINFORM from 192.168.5.253 via eth3: not authoritative for subnet 192.168.5.0
2025-04-10 10:03:41.0 dhcpd@ daemon.info DHCPREQUEST for 192.168.5.249 from 80:5e:0c:46:79:ad (SIP-T42U) via eth3
2025-04-10 10:03:41.0 dhcpd@ daemon.info DHCPACK on 192.168.5.249 to 80:5e:0c:46:79:ad (SIP-T42U) via eth3
2025-04-10 10:03:54.0 dhcpd@ daemon.info DHCPINFORM from 192.168.5.251 via eth3: not authoritative for subnet 192.168.5.0
2025-04-10 10:03:55.0 dhcpd@ daemon.info DHCPREQUEST for 10.2.255.143 from 38:87:d5:35:30:a4 (JulianLaptop) via eth2
2025-04-10 10:03:55.0 dhcpd@ daemon.info DHCPACK on 10.2.255.143 to 38:87:d5:35:30:a4 (JulianLaptop) via eth2
2025-04-10 10:03:56.0 dhcpd@ daemon.info DHCPREQUEST for 10.1.121.97 from 44:a5:6e:61:ba:e8 (AxiomLANPOE) via eth1
2025-04-10 14:05:35.0 dhcpd@ daemon.info DHCPOFFER on 10.2.255.75 to 8a:1d:ba:3b:35:dd (Julian-s-A34) via eth2


2017-03-12T08:44:52.421114+01:00, Linuxx, info, dhcpd: DHCPREQUEST for 10.0.0.63 from 68:ab:35:59:9c:a1 via 10.0.0.1
2017-03-12T08:44:52.421174+01:00, Linuxx, info, dhcpd: DHCPACK on 10.0.0.63 to 68:ab:35:59:9c:a1 via 10.0.0.1
*/

int IsDate(const char *Token)
{
    const char *ptr;
    int SepCount=0;

    for (ptr=Token; *ptr != '\0'; ptr++)
    {
        switch (*ptr)
        {
        case '-':
            SepCount++;
            break;
        case '/':
            SepCount++;
            break;
        case '_':
            SepCount++;
            break;


        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            break;

        default:
            return(FALSE);
            break;
        }

    }

    if (SepCount==2) return(TRUE);

    return(FALSE);
}


int IsTime(const char *Token)
{
    const char *ptr;
    int SepCount=0;

    for (ptr=Token; *ptr != '\0'; ptr++)
    {
        switch (*ptr)
        {
        case ':':
            SepCount++;
            break;

        case '.':
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            break;

        default:
            return(FALSE);
            break;
        }

    }

    if (SepCount==2) return(TRUE);

    return(FALSE);
}


int IsDateTime(const char *Input)
{
    char *Token=NULL;
    const char *ptr;
    int RetVal=FALSE;

    if (strchr(Input, 'T'))
    {
        ptr=GetToken(Input, "T", &Token, 0);
        if (IsDate(Token))
        {
            ptr=GetToken(ptr, ".", &Token, 0);
            if (IsTime(Token)) RetVal=TRUE;
        }
    }

    return(RetVal);
}


int MonthName(const char *Input)
{
    const char *DayNames[]= {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec",NULL};
    int i;

    for (i=0; DayNames[i] !=NULL; i++)
    {
        if (strcasecmp(DayNames[i], Input)==0) return(i+1);
    }

    return(0);
}


const char *ExtractDateTime(const char *RestOfLine, const char *Input, char **Date, char **Time)
{
    char *Token=NULL;
    const char *ptr, *line;
    int Month;

    line=RestOfLine;

    Month=MonthName(Input);
    if (Month > 0)
    {
        line=GetToken(line, "\\S", &Token, 0);
        *Date=FormatStr(*Date, "%s-%02d-%02d", CurrYear, Month, atoi(Token));
    }
    else if (IsDate(Input)) *Date=CopyStr(*Date, Input);
    else if (IsTime(Input)) *Time=CopyStr(*Time, Input);
    else if (IsDateTime(Input))
    {
        ptr=GetToken(Input, "T", Date, 0);
        ptr=GetToken(ptr, ".", Time, 0);
    }

    Destroy(Token);

    return(line);
}



const char *LogFileProcessDHCP(const char *Input, const char *Date, const char *Time, ListNode *Hosts)
{
    char *Token=NULL, *IP=NULL, *MAC=NULL, *Name=NULL, *Dev=NULL;
    const char *ptr;
    ListNode *Node;
    THost *Host;
    time_t When;

// DHCPACK on 10.2.255.143 to 38:87:d5:35:30:a4 (JulianLaptop) via eth2
    if (StrValid(Date) && StrValid(Time))
    {
        Token=MCopyStr(Token, Date, "T", Time, NULL);
        When=DateStrToSecs("%Y-%m-%dT%H:%M:%S", Token, NULL);
    }

    ptr=GetToken(Input, "\\S", &Token, 0);

    if (strcmp(Token, "on")==0)
    {
        ptr=GetToken(ptr, "\\S", &IP, 0);
        ptr=GetToken(ptr, "\\S", &Token, 0);
    }

//for some weird reason we sometimes get the IP address for the network card the
//request came in on here...
    while (StrValid(ptr) && (strcmp(Token, "to") != 0) && (strcmp(Token, "from") != 0) ) ptr=GetToken(ptr, "\\S", &Token, 0);

    ptr=GetToken(ptr, "\\S", &MAC, 0);
    if (strcount(MAC, ':') != 5)
    {
        IP=CopyStr(IP, MAC);
        ptr=GetToken(ptr, "\\S", &MAC, 0);
        StripStartEndChars(MAC, "(", ")");
    }

    ptr=GetToken(ptr, "\\S", &Name, 0);
    StripStartEndChars(Name, "(", ")");

    if (strcmp(Name, "via")==0) Name=CopyStr(Name, "");
    else ptr=GetToken(ptr, "\\S", &Token, 0); //this should be 'via'
    ptr=GetToken(ptr, "\\S", &Dev, 0);

    Host=HostCreate(MAC, Name, When);
    Host->IP=CopyStr(Host->IP, IP);
    Host->Dev=CopyStr(Host->Dev, Dev);
    if (When < Host->FirstSeen) Host->FirstSeen=When;
    if (When > Host->LastSeen) Host->LastSeen=When;

    HostListAdd(Hosts, Host);

    Destroy(Token);
    Destroy(Name);
    Destroy(MAC);
    Destroy(Dev);
    Destroy(IP);

    return(ptr);
}

void LogFileParseDHCPHost(ListNode *Hosts, const char *Line)
{
    char *Tempstr=NULL, *Date=NULL, *Time=NULL, *DateTime=NULL, *Token=NULL;
    const char *ptr;

    ptr=GetToken(Line, "\\S", &Token, 0);
    while (ptr)
    {
        if (strcmp(Token, "DHCPACK")==0) ptr=LogFileProcessDHCP(ptr, Date, Time, Hosts);
        else if (strcmp(Token, "DHCPOFFER")==0) ptr=LogFileProcessDHCP(ptr, Date, Time, Hosts);
        else if (strcmp(Token, "DHCPREQUEST")==0) ptr=LogFileProcessDHCP(ptr, Date, Time, Hosts);
//else if (strcmp(Token, "DHCINFORM")==0) ptr=LogFileProcessDHCP(ptr, Date, Time, Hosts);
        else ptr=ExtractDateTime(ptr, Token, &Date, &Time);

        ptr=GetToken(ptr, "\\S", &Token, 0);
    }

    Destroy(Tempstr);
    Destroy(Token);
    Destroy(DateTime);
    Destroy(Date);
    Destroy(Time);
}


int LogFileRead(const char *Path, ListNode *Hosts)
{
    STREAM *S;
    char *Tempstr=NULL;
    int RetVal=FALSE;

    fprintf(stderr, "LFR: %s\n", Path);

    S=STREAMOpen(Path, "r");
    if (S)
    {
        RetVal=TRUE;
        Tempstr=STREAMReadLine(Tempstr, S);
        while (Tempstr)
        {
            OutputFileProgress(S);
            LogFileParseDHCPHost(Hosts, Tempstr);
            Tempstr=STREAMReadLine(Tempstr, S);
        }

        if (S->in_fd==0) STREAMDestroy(S);
        else STREAMClose(S);
        fprintf(stderr, "\n");
    }

    Destroy(Tempstr);

    return(RetVal);
}


