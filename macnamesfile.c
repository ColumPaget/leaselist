#include "macnamesfile.h"

ListNode *MacNames=NULL;

int MacNamesLoadFile(const char *Path)
{
    char *Tempstr=NULL, *Token=NULL;
    const char *ptr;
    STREAM *S;
    int count=-1;

    if (! MacNames) MacNames=MapCreate(4096, 0);

    S=STREAMOpen(Path, "r");
    if (S)
    {
        count=0;
        Tempstr=STREAMReadLine(Tempstr, S);
        while (Tempstr)
        {
            StripTrailingWhitespace(Tempstr);
            ptr=GetToken(Tempstr, "\\S", &Token, 0);
            SetVar(MacNames, Token, ptr);
            count++;
            Tempstr=STREAMReadLine(Tempstr, S);
        }
        STREAMClose(S);
    }

    Destroy(Tempstr);
    Destroy(Token);

    return(count);
}


const char *MacNamesFind(const char *Mac)
{
    return(GetVar(MacNames, Mac));
}


void MacNamesLoad(const char *Paths)
{
    char *Path=NULL;
    const char *ptr;

    ptr=GetToken(Paths, ":", &Path, 0);
    while (ptr)
    {
        MacNamesLoadFile(Path);
        ptr=GetToken(ptr, ":", &Path, 0);
    }

    Destroy(Path);
}
