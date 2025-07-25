#include "Container.h"
#include "libUseful.h"
#include <glob.h>

#include <sys/mount.h>

#ifdef HAVE_UNSHARE
#define _GNU_SOURCE
#include <sched.h>
#endif


//things get a bit recursive below
static int ContainerJoinNamespace(const char *Namespace, int type);



static void InitSigHandler(int sig)
{
}


static void ContainerInitProcess(int tunfd, int linkfd, pid_t Child, int RemoveRootDir)
{
    struct sigaction sa;

    //this process is init, the child will carry on execution
    //if (chroot(".") == -1) RaiseError(ERRFLAG_ERRNO, "chroot", "failed to chroot to curr directory");
    ProcessSetTitle("init");

    close(0);
    close(1);

    memset(&sa,0,sizeof(sa));
    sa.sa_handler=InitSigHandler;
    sa.sa_flags=SA_NOCLDSTOP;
    sigaction(SIGCHLD, &sa,NULL);


    /*
    FileSystemUnMount("/proc","rmdir");
    if (RemoveRootDir) FileSystemUnMount("/","recurse,rmdir");
    else
    {
    FileSystemUnMount("/","subdirs,rmdir");
    FileSystemUnMount("/","recurse");
    }
    */

    //must do proc after the fork so that CLONE_NEWPID takes effect
    //mkdir("/proc",0755);
    //mount("", "/proc", "proc", 0, "hidepid=2");

    //FileSystemMount("","/proc","proc","");

    while (waitpid(-1,NULL,0) != -1);
}


//if we are even unsharing our PIDS namespace, then we will need a
//new 'init' process to look after pids in the namespace
//(this is mostly just to reap exited/zombie processes)
static pid_t ContainerLaunchInit(int Flags, const char *Dir)
{
    pid_t child, parent;

    //as we are going to create an init for a namespace it needs to be session leader
    //setsid();

    //fork off a process that will be our 'init' process
    child=fork();
    if (child == 0)
    {
        setsid();
        child=fork();
        if (child !=0)
        {
            if ((! (Flags & PROC_ISOCUBE)) && StrValid(Dir)) ContainerInitProcess(-1, -1, child, FALSE);
            else ContainerInitProcess(-1, -1, child, TRUE);
            //ContainerInitProcess should never return, but we'll have this here anyway
            _exit(0);
        }
    }
    else _exit(0);

    return(child);
}




static int ContainerUnsharePID(int Flags, const char *Namespace, const char *Dir)
{
    pid_t pid, init_pid=0;

#ifdef HAVE_UNSHARE
#ifdef CLONE_NEWPID

    // NEWPID requires NEWNS which creates a new mount namespace, because we need to remount /roc
    // within the new PID container
    if (StrValid(Namespace)) ContainerJoinNamespace(Namespace, CLONE_NEWPID | CLONE_NEWNS);
    else unshare(CLONE_NEWPID | CLONE_NEWNS);

    //if we are given a namespace we assume there is already an init for it
    //otherwise launch and init from the NEWPID process
    if (! StrValid(Namespace))
    {
        init_pid=ContainerLaunchInit(Flags, Dir);
        setpgid(init_pid, init_pid);
    }

    return(TRUE);

#endif
#else
    RaiseError(0, "namespaces", "containers/unshare unavailable");
#endif
    return(FALSE);
}


static void ContainerSetHostname(const char *Namespace, const char *HostName)
{
    int val, result;

#ifdef HAVE_UNSHARE
#ifdef CLONE_NEWUTS
    if (StrValid(Namespace)) ContainerJoinNamespace(Namespace, CLONE_NEWUTS);
    else
    {
        unshare(CLONE_NEWUTS);
        val=StrLen(HostName);
        if (val != 0) result=sethostname(HostName, val);
        else result=sethostname("container", 9);
        if (result != 0) RaiseError(ERRFLAG_ERRNO, "ContainerNamespace", "Failed to sethostname for container.");
    }
#endif
#else
    RaiseError(0, "namespaces", "containers/unshare unavailable");
#endif
}



static void ContainerSetEnvs(const char *Envs)
{
    char *Name=NULL, *Value=NULL;
    const char *ptr;

#ifdef HAVE_CLEARENV
    clearenv();
#endif

    setenv("LD_LIBRARY_PATH","/lib:/usr/lib",TRUE);

    ptr=GetNameValuePair(Envs, ",","=", &Name, &Value);
    while (ptr)
    {
        setenv(Name, Value, TRUE);
        ptr=GetNameValuePair(ptr, ",","=", &Name, &Value);
    }
    Destroy(Name);
    Destroy(Value);
}


static int ContainerJoinNamespace(const char *Namespace, int type)
{
    char *Tempstr=NULL;
    struct stat Stat;
    glob_t Glob;
    int i, fd, result=FALSE;

#ifdef HAVE_UNSHARE
#ifdef HAVE_SETNS
    stat(Namespace,&Stat);
    if (S_ISDIR(Stat.st_mode))
    {
        Tempstr=MCopyStr(Tempstr,Namespace,"/*",NULL);
        glob(Tempstr,0,0,&Glob);
        if (Glob.gl_pathc ==0) RaiseError(ERRFLAG_ERRNO, "namespaces", "namespace dir %s empty", Tempstr);
        for (i=0; i < Glob.gl_pathc; i++)
        {
            fd=open(Glob.gl_pathv[i],O_RDONLY);
            if (fd > -1)
            {
                result=TRUE;
                setns(fd, type);
                close(fd);
            }
            else RaiseError(ERRFLAG_ERRNO, "namespaces", "couldn't open namespace %s", Glob.gl_pathv[i]);
        }
    }
    else
    {
        fd=open(Namespace,O_RDONLY);
        if (fd > -1)
        {
            result=TRUE;
            setns(fd, type);
            close(fd);
        }
        else RaiseError(ERRFLAG_ERRNO, "namespaces", "couldn't open namespace %s", Namespace);
    }
#else
    RaiseError(0, "namespaces", "setns unavailable");
#endif
    RaiseError(0, "namespaces", "setns unavailable");
#endif

    Destroy(Tempstr);
    return(result);
}



static void ContainerPrepareFilesys(const char *Config, const char *Dir, int Flags)
{
    pid_t pid;
    char *Tempstr=NULL, *Name=NULL, *Value=NULL;
    char *ROMounts=NULL, *RWMounts=NULL;
    char *Links=NULL, *PLinks=NULL, *FileClones=NULL;
    const char *ptr, *tptr;
    struct stat Stat;


    ptr=GetNameValuePair(Config,"\\S","=",&Name,&Value);
    while (ptr)
    {
        if (strcasecmp(Name,"+mnt")==0) ROMounts=MCatStr(ROMounts,",",Value,NULL);
        else if (strcasecmp(Name,"+mnt")==0) ROMounts=MCatStr(ROMounts,",",Value,NULL);
        else if (strcasecmp(Name,"mnt")==0) ROMounts=CopyStr(ROMounts,Value);
        else if (strcasecmp(Name,"+wmnt")==0) RWMounts=MCatStr(RWMounts,",",Value,NULL);
        else if (strcasecmp(Name,"wmnt")==0) RWMounts=CopyStr(RWMounts,Value);
        else if (strcasecmp(Name,"+link")==0) Links=MCatStr(Links,",",Value,NULL);
        else if (strcasecmp(Name,"link")==0) Links=CopyStr(Links,Value);
        else if (strcasecmp(Name,"+plink")==0) PLinks=MCatStr(PLinks,",",Value,NULL);
        else if (strcasecmp(Name,"plink")==0) PLinks=CopyStr(PLinks,Value);
        else if (strcasecmp(Name,"pclone")==0) FileClones=CopyStr(FileClones,Value);
        ptr=GetNameValuePair(ptr,"\\S","=",&Name,&Value);
    }

    pid=getpid();

    if (StrValid(Dir)) Tempstr=FormatStr(Tempstr,Dir,pid);
    else Tempstr=FormatStr(Tempstr,"%d.container",pid);

    mkdir(Tempstr,0755);
    if (Flags & PROC_ISOCUBE)	FileSystemMount("",Tempstr,"tmpfs","");
    if (chdir(Tempstr) !=0) RaiseError(ERRFLAG_ERRNO, "ContainerPrepareFilesys", "failed to chdir to %s", Tempstr);

    //always make a tmp directory
    mkdir("tmp",0777);

    ptr=GetToken(ROMounts,",",&Value,GETTOKEN_QUOTES);
    while (ptr)
    {
        FileSystemMount(Value,"","bind","ro perms=755");
        ptr=GetToken(ptr,",",&Value,GETTOKEN_QUOTES);
    }

    ptr=GetToken(RWMounts,",",&Value,GETTOKEN_QUOTES);
    while (ptr)
    {
        FileSystemMount(Value,"","bind","perms=777");
        ptr=GetToken(ptr,",",&Value,GETTOKEN_QUOTES);
    }

    ptr=GetToken(Links,",",&Value,GETTOKEN_QUOTES);
    while (ptr)
    {
        if (link(Value,GetBasename(Value)) !=0)
            ptr=GetToken(ptr,",",&Value,GETTOKEN_QUOTES);
    }

    ptr=GetToken(PLinks,",",&Value,GETTOKEN_QUOTES);
    while (ptr)
    {
        tptr=Value;
        if (*tptr=='/') tptr++;
        MakeDirPath(tptr,0755);
        if (link(Value, tptr) != 0) RaiseError(ERRFLAG_ERRNO, "ContainerPrepareFilesys", "Failed to link Value tptr.");
        ptr=GetToken(ptr,",",&Value,GETTOKEN_QUOTES);
    }

    ptr=GetToken(FileClones,",",&Value,GETTOKEN_QUOTES);
    while (ptr)
    {
        tptr=Value;
        if (*tptr=='/') tptr++;
        MakeDirPath(tptr,0755);
        stat(Value, &Stat);
        if (S_ISCHR(Stat.st_mode) || S_ISBLK(Stat.st_mode)) mknod(tptr, Stat.st_mode, Stat.st_rdev);
        else
        {
            FileCopy(Value, tptr);
            chmod(tptr, Stat.st_mode);
        }
        ptr=GetToken(ptr,",",&Value,GETTOKEN_QUOTES);
    }


    Destroy(Name);
    Destroy(Value);
    Destroy(Tempstr);
    Destroy(ROMounts);
    Destroy(RWMounts);
    Destroy(Links);
    Destroy(PLinks);
    Destroy(FileClones);
}



#ifdef HAVE_UNSHARE
#ifdef CLONE_NEWUSER

static void ContainerNewUserNamespace(pid_t myPid, uid_t myUid, gid_t myGid)
{
    char *Path=NULL, *Tempstr=NULL;

    unshare(CLONE_NEWUSER);


//map our user number (0) in the namespace to our real user id outside it
    Path=FormatStr(Path, "/proc/%d/uid_map", myPid);
    Tempstr=FormatStr(Tempstr, "%ld %ld 1\n", myUid, myUid);
    FileWrite(Path, Tempstr);


//map our group number (0) in the namespace to our real gid outside it
    Path=FormatStr(Path, "/proc/%d/setgroups", myPid);
    FileWrite(Path, "deny");

//map our user number (0) in the namespace to our real user id outside it
    Path=FormatStr(Path, "/proc/%d/gid_map", myPid);
    Tempstr=FormatStr(Tempstr, "%ld %ld 1\n", myGid, myGid);
    FileWrite(Path, Tempstr);


    Destroy(Tempstr);
    Destroy(Path);
}

#endif
#endif

//uid and gid here are the EXTERNAL uid and gid outside of the container
static void ContainerNamespace(const char *Namespace, const char *HostName, const char *Dir, int Flags, uid_t uid, gid_t gid)
{
#ifdef HAVE_UNSHARE

    int val, result;


//if we're not root, then we'll have to set up a user namespace
    if (uid !=0) ContainerNewUserNamespace(getpid(), uid, gid);

    if (Flags & PROC_CONTAINER_FS)
    {
#ifdef CLONE_NEWNS
        if (StrValid(Namespace)) ContainerJoinNamespace(Namespace, CLONE_NEWNS);
        else unshare(CLONE_NEWNS);

//make container invisible to outside processes, apparently
//  mount("none", "/", 0, MS_PRIVATE | MS_REC, NULL);
#endif
    }

#ifdef CLONE_NEWNET
    if (StrValid(Namespace)) ContainerJoinNamespace(Namespace, CLONE_NEWNET);
    else if (Flags & PROC_CONTAINER_NET) unshare(CLONE_NEWNET);
#endif


#ifdef CLONE_NEWIPC
    if (StrValid(Namespace)) ContainerJoinNamespace(Namespace, CLONE_NEWIPC);
    else if (Flags & PROC_CONTAINER_IPC) unshare(CLONE_NEWIPC);
#endif

    if (Flags & PROC_CONTAINER_PID) ContainerUnsharePID(Flags, Namespace, Dir);
    if (StrValid(HostName)) ContainerSetHostname(Namespace, HostName);



#else
    RaiseError(0, "namespaces", "containers/unshare unavailable");
#endif
}



static int ContainerParseConfig(const char *Config, char **HostName, char **Dir, char **Namespace, char **ChRoot, char **Envs)
{
    char *Tempstr=NULL, *Name=NULL, *Value=NULL;
    const char *ptr;
    int Flags=0;

    ptr=GetNameValuePair(Config,"\\S","=",&Name,&Value);
    while (ptr)
    {
        if (strcasecmp(Name,"hostname")==0) *HostName=CopyStr(*HostName, Value);
        else if (strcasecmp(Name,"dir")==0) *Dir=CopyStr(*Dir, Value);
        else if (strcasecmp(Name,"nonet")==0) Flags |= PROC_CONTAINER_NET;
        else if (strcasecmp(Name,"-net")==0) Flags |= PROC_CONTAINER_NET;
        else if (strcasecmp(Name,"+net")==0) Flags &= ~PROC_CONTAINER_NET;
        else if (strcasecmp(Name,"-pid")==0) Flags |= PROC_CONTAINER_PID;
        else if (strcasecmp(Name,"nopid")==0) Flags |= PROC_CONTAINER_PID;
        //else if (strcasecmp(Name,"jailsetup")==0) SetupScript=CopyStr(SetupScript, Value);
        else if (
            (strcasecmp(Name,"ns")==0) ||
            (strcasecmp(Name,"namespace")==0)
        )
        {
            *Namespace=CopyStr(*Namespace, Value);
            Flags |= PROC_CONTAINER_FS;
        }
        else if (strcasecmp(Name,"container")==0)
        {
            if (StrValid(Value)) *ChRoot=CopyStr(*ChRoot, Value);
            Flags |= PROC_CONTAINER_FS;
        }
        else if (strcasecmp(Name,"container-net")==0)
        {
            if (StrValid(Value)) *ChRoot=CopyStr(*ChRoot, Value);
            Flags |= PROC_CONTAINER_FS | PROC_CONTAINER_NET;
        }
        else if (strcasecmp(Name,"isocube")==0)
        {
            if (StrValid(Value)) *ChRoot=CopyStr(*ChRoot, Value);
            Flags |= PROC_ISOCUBE | PROC_CONTAINER_FS;
        }
        else if (strcasecmp(Name,"setenv")==0)
        {
            Tempstr=QuoteCharsInStr(Tempstr, Value, ",");
            *Envs=MCatStr(*Envs, Tempstr, ",",NULL);
        }

        ptr=GetNameValuePair(ptr,"\\S","=",&Name,&Value);
    }

    Destroy(Tempstr);
    Destroy(Name);
    Destroy(Value);

    return(Flags);
}


int ContainerApplyConfig(int Flags, const char *Config)
{
    char *HostName=NULL, *SetupScript=NULL, *Namespace=NULL, *Envs=NULL;
    char *Dir=NULL, *ChRoot=NULL;
    char *Name=NULL, *Value=NULL;
    char *Tempstr=NULL;
    const char *ptr;
    int RetVal=TRUE, result;
    pid_t child;
    uid_t external_uid;
    gid_t external_gid;


    external_uid=getuid();
    external_gid=getgid();


    Flags |= ContainerParseConfig(Config, &HostName, &Dir, &Namespace, &ChRoot, &Envs);

    if (Flags)
    {
        ContainerNamespace(Namespace, HostName, Dir, Flags, external_uid, external_gid);

        if (Flags & PROC_CONTAINER_FS)
        {
            if (! StrValid(ChRoot))
            {
                ChRoot=CopyStr(ChRoot, Dir);
                Dir=CopyStr(Dir,"");
            }
            ContainerPrepareFilesys(Config, ChRoot, Flags);

            //we do not call CredsStoreOnFork here becausee it's assumed that we want to take the creds store with us, as
            //these forks are in order to change aspects of our program, rather than spawn a new process

            if (StrValid(SetupScript))
            {
                if (system(SetupScript) < 1) RaiseError(ERRFLAG_ERRNO, "ContainerApplyConfig", "failed to exec %s", SetupScript);
            }
        }

#ifdef __linux__
#ifdef MS_BIND
        mkdir("proc", 0755);
        if (external_uid==0) result=mount("", "proc", "proc", 0, NULL);
        else result=mount("/proc", "proc", NULL, MS_REC | MS_BIND, NULL);
#endif
#endif

        //ContainerSetEnvs(Envs);

        if (Flags & PROC_CONTAINER_FS)
        {
            if (chroot(".") == -1)
            {
                RaiseError(ERRFLAG_ERRNO, "ContainerApplyConfig", "failed to chroot to curr directory");
                RetVal=FALSE;
            }
        }


        //'RetVal' is our 'so far so good' value
        if (RetVal)
        {
            LibUsefulSetupAtExit();
            LibUsefulFlags |= LU_CONTAINER;

            if (StrValid(Dir))
            {
                if (chdir(Dir) !=0) RaiseError(ERRFLAG_ERRNO, "ContainerApplyConfig", "failed to chdir to %s", Dir);
            }
        }

        if ( setuid(external_uid) != 0) RaiseError(ERRFLAG_ERRNO, "ContainerApplyConfig", "failed to switch user to %d", external_uid);
        if ( setgid(external_gid) != 0) RaiseError(ERRFLAG_ERRNO, "ContainerApplyConfig", "failed to switch group to %d", external_gid);
    }

    Destroy(Tempstr);
    Destroy(SetupScript);
    Destroy(HostName);
    Destroy(Namespace);
    Destroy(Name);
    Destroy(Value);
    Destroy(ChRoot);
    Destroy(Dir);

    return(RetVal);
}

