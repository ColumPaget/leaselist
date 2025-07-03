#include "settings.h"


TSettings *Settings=NULL;


void PrintHelp()
{

    printf("usage: leaselist [options] [path] [path] ...\n");
    printf("  -M <path>      - path to oui.txt file containing mac-address to vendor mapping. Supplying this activates host vendor output\n");
    printf("  -N <path>      - path to file containing mac-address to custom name. defaults to '/etc/macnames.conf'\n");
    printf("  -h             - print lists of historical Names/IP-Addresses of hosts\n");
    printf("  -n             - show still-active leases\n");
    printf("  -now           - show still-active leases\n");
    printf("  -t             - show leases given today\n");
    printf("  -today         - show leases given today\n");
    printf("  -w             - show leases given this week\n");
    printf("  -week          - show leases given this week\n");
    printf("  -version       - print version\n");
    printf("  --version      - print version\n");
    printf("  -help          - print this help\n");
    printf("  --help         - print this help\n");
    printf("  -?             - print this help\n");

    exit(0);
}


void PrintVersion()
{
printf("leaselist: %s\n", PACKAGE_VERSION);
exit(0);
}



void ParseCommandLine(int argc, char *argv[])
{
    CMDLINE *CMD;
    const char *item;

    CMD=CommandLineParserCreate(argc, argv);

    item=CommandLineNext(CMD);
    while (item)
    {
        if (strcmp(item, "-M")==0) Settings->OUIMacFiles=CopyStr(Settings->OUIMacFiles, CommandLineNext(CMD));
        else if (strcmp(item, "-N")==0) Settings->NamesFiles=CopyStr(Settings->NamesFiles, CommandLineNext(CMD));
        else if (strcmp(item, "-h")==0) Settings->Flags |= FLAG_HISTORY;
        else if (strcmp(item, "-n")==0) Settings->Flags |= FLAG_NOW;
        else if (strcmp(item, "-now")==0) Settings->Flags |= FLAG_NOW;
        else if (strcmp(item, "-t")==0) Settings->Flags |= FLAG_TODAY;
        else if (strcmp(item, "-today")==0) Settings->Flags |= FLAG_TODAY;
        else if (strcmp(item, "-w")==0) Settings->Flags |= FLAG_WEEK;
        else if (strcmp(item, "-week")==0) Settings->Flags |= FLAG_WEEK;
        else if (strcmp(item, "-help")==0) PrintHelp();
        else if (strcmp(item, "--help")==0) PrintHelp();
        else if (strcmp(item, "-?")==0) PrintHelp();
        else if (strcmp(item, "-version")==0) PrintVersion();
        else if (strcmp(item, "--version")==0) PrintVersion();
        else Settings->InputFiles=MCatStr(Settings->InputFiles, item, ":", NULL);
        item=CommandLineNext(CMD);
    }

}



void SettingsInit()
{
    Settings=(TSettings *) calloc(1,sizeof(TSettings));

    Settings->NamesFiles=CopyStr(Settings->NamesFiles, "/etc/macnames.conf");
    Settings->LeaseFiles=CopyStr(Settings->LeaseFiles, "/var/state/dhcp/dhcpd.leases:/var/lib/dhcpd/dhcpd.leases");

}
