#include "oui_maclookup.h"

//list of mac-address prefixes and the manufactuers they are booked to
char *OUIMacFile=NULL;


ListNode *OUIMacFileLoad(const char *OUIMacFile)
{
    STREAM *S;
    char *Tempstr=NULL, *MAC=NULL, *Token=NULL;
    const char *ptr;
    ListNode *MacList=NULL;


    S=STREAMOpen(OUIMacFile, "r");
    if (S)
    {
        MacList=MapCreate(4096, LIST_FLAG_CACHE);
        Tempstr=STREAMReadLine(Tempstr, S);
        while (Tempstr)
        {
            StripTrailingWhitespace(Tempstr);

            OutputFileProgress(S);
            if (StrValid(Tempstr) && (! isspace(Tempstr[0])))
            {
                ptr=GetToken(Tempstr, "\\S", &MAC, 0);
                if (strchr(MAC, '-'))
                {
                    strlwr(MAC);
                    strrep(MAC, '-', ':');
                    ptr=GetToken(ptr, "\\S", &Token, 0);
                    while (isspace(*ptr)) ptr++;

                    if (StrValid(ptr)) SetVar(MacList, MAC, ptr);
                }
            }

            Tempstr=STREAMReadLine(Tempstr, S);
        }

        STREAMClose(S);
        fprintf(stderr, "\n");
    }

    Destroy(Tempstr);
    Destroy(Token);
    Destroy(MAC);

    return(MacList);
}

