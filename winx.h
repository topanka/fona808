#ifndef __WINX_H_INCLUDED__
#define __WINX_H_INCLUDED__

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <signal.h>
#include <math.h>
#include <stdarg.h>
#include <limits.h>
#include <stdint.h>

#define WIN_PATH_SEPARATOR         '\\'
#define UNX_PATH_SEPARATOR         '/'
#define DISKBUFSIZE                4096
#define FACCESS_RW                 0x06
#define FACCESS_EXISTS             0x00

#define MAX_CHILD_NUM              1048
#define MIN_CHILD_NUM              3

#define _SIN(x)                    ((x == NULL) ? "_(null)_\0" : x)

#define NCA_PTT_ANY                0
#define NCA_PTT_UNKNOWN            ((unsigned long)(-1L))
#define NCA_PTT_RECVFROMCLI        1
#define NCA_PTT_SLIDER             2
#define NCA_PTT_BACKUP             3
#define NCA_PTT_ECL                4
#define NCA_PTT_BGTBLSIGNER        5
#define NCA_PTT_RLE					       6
#define NCA_PTT_HEARTBEAT          7
#define NCA_PTT_SPC				         8

#define NCA_FSTATCH_MODE           1
#define NCA_FSTATCH_MTIME				   2
#define NCA_FSTATCH_CTIME          3
#define NCA_FSTATCH_SIZE           4
#define NCA_FSTATCH_UID            5
#define NCA_FSTATCH_GID            6

#define NCA_WS_ANY			           0
#define NCA_WS_STARTING            1
#define NCA_WS_STARTED             2
#define NCA_WS_RUNNING             3
#define NCA_WS_IDLE                4
#define NCA_WS_EXITING             5
#define NCA_WS_ABORTING            6

#define NCA_FTYPE_INVALID          0
#define NCA_FTYPE_DIR              1
#define NCA_FTYPE_FILE             2
#define NCA_FTYPE_LINK             3
#define NCA_FTYPE_PIPE             4
#define NCA_FTYPE_SOCKET           5
#define NCA_FTYPE_BLOCKDEV         6
#define NCA_FTYPE_CHARDEV          7

#define NCA_FFET_NOORDER           0          //FindFile Enum Type
#define NCA_FFET_DIRSFIRST         1
#define NCA_FFET_FILESFIRST        2

#define NCA_FFET_DIRSNOW           0x00000100UL
#define NCA_FFET_FILESNOW          0x00000200UL

#define NCA_ECP_NAMEPART           "ecp"

#define NCAAPHS_CREATED            1
#define NCAAPHS_PINIT              2
#define NCAAPHS_CINIT              3

#define NCA_TMM_TOTAL              0
#define NCA_TMM_ODBC               1
#define NCA_TMM_NCADB              2
#define NCA_TMM_CRYPTO             3
#define NCA_TMM_PKCS11             4
#define NCA_TMM_COMM               5
#define NCA_TMM_WAIT               6
#define NCA_TMM_D1                 7
#define NCA_TMM_D2                 8
#define NCA_TMM_D3                 9
#define NCA_TMM_D4                 10
#define NCA_TMM_MAXNUM             11

#define NCA_NCACON_NAME						 "ncacon$"

#if defined(WIN32)

#include <conio.h>
#include <io.h>
#include <direct.h>
#include <fcntl.h>
#include <process.h>
#include <share.h>
#include <sys/stat.h>
#define NOCRYPT
#include <windows.h>
#undef NOCRYPT
#include <sql.h>
#include <sqlext.h>
#include <odbcinst.h>
#include <sys/timeb.h>
#include "fnmatch.h"
#include "ncawinsp.h"

#define snprintf                   _snprintf
#define vsnprintf                  _vsnprintf
														 
#define OS_STR                     "WinNT"
#define OS_NULLDEV                 "nul"

#define NCA_SHM_BASENAME           "Global\\nca_shm_name"
#define NCA_SHM_BASEVALUE          22000

#define MUTEX_NCADB_BASE           "Global\\mtx_name_ncadb"
#define MUTEX_NCAP11SESS_BASE      "Global\\mtx_name_ncap11sess"
#define MUTEX_NCACONF_BASE         "Global\\mtx_name_ncaconfig"
#define MUTEX_NCASESSION_BASE      "Global\\mtx_name_ncasession"
#define MUTEX_NCALGF_BASE          "Global\\mtx_name_ncalgf"
#define MUTEX_NCALGA_BASE          "Global\\mtx_name_ncalga"
#define MUTEX_NCAPASSWORD_BASE     "Global\\mtx_name_ncapassword"
#define MUTEX_NCAAUTOCRL_BASE      "Global\\mtx_name_ncaautocrl"
#define MUTEX_NCATT_BASE           "Global\\mtx_name_ncatt"
#define MUTEX_SSIDEF_BASE          "Global\\mtx_name_ncassidef"
#define MUTEX_LOG2_BASE            "Global\\mtx_name_ncalog2"
#define MUTEX_NCALOAD_BASE         "Global\\mtx_name_ncaload"
#define MUTEX_LIPC_BASE            "Global\\mtx_name_ncalipc"
#define MUTEX_P11INITIALIZE_BASE   "Global\\mtx_name_p11init"
#define MUTEX_NCARLEMSGS_BASE      "Global\\mtx_name_ncarlemsgs"
#define MUTEX_NCARAND_BASE				 "Global\\mtx_name_ncarand"
#define MUTEX_NCAGC_BASE           "Global\\mtx_name_ncagc"
#define MUTEX_NCASPC_BASE          "Global\\mtx_name_ncaspc"

#define NCANEWLIC_EXE_NAME         "ncanewlc.exe"
#define NCAINIT_EXE_NAME           "ncainit.exe"
#define NCASERVICE_EXE_NAME        "ncaadm.exe"
#define NCATOOLS_EXE_NAME 	       "ncatools.exe"

#define INVALID_MUTEX              NULL
#define INVALID_MRSW_MUTEX         {0}
#define MUTEXWAIT_FAILED           (WAIT_FAILED)
#define MUTEXWAIT_TIMEOUTED        (WAIT_TIMEOUT)
#define MUTEXWAIT_INFINITE         (INFINITE)
#define MUTEX_EXISTS               (ERROR_ALREADY_EXISTS)

#define PATH_SEPARATOR             WIN_PATH_SEPARATOR

#define FOM_RA_BIN                 "a+b"
#define FOM_AO_BIN                 "ab"
#define FOM_RO_BIN                 "rb"
#define FOM_WO_BIN                 "wb"
#define FOM_RW_BIN                 "r+b"
#define FOM_RWD_BIN                "w+b"

#define FOM_RA_TXT                 "a+t"
#define FOM_RO_TXT                 "rt"
#define FOM_WO_TXT                 "wt"
#define FOM_RW_TXT                 "r+t"
#define FOM_RWD_TXT                "w+t"

#define NEW_LINE_CHAR              "\r\n"

#define	O_NDELAY									 0

#define DPM_OWNER_ONLY             (_S_IREAD | _S_IWRITE)
#define FPM_OWNER_ONLY             (_S_IREAD | _S_IWRITE)
#define DPM_ORW_GR                 (_S_IREAD | _S_IWRITE)
#define FPM_ORW_GR                 (_S_IREAD | _S_IWRITE)

#define FNM_DEF_FLAGS              (FNM_NOESCAPE)

#define NCA_LASTONE_PROCNUM        2

#define nca_x_getsockerr()         ((long)WSAGetLastError())
#define nca_x_getsyserr()          ((long)GetLastError())
#define nca_x_sleep(x)             Sleep(1000L*(x))
#define nca_x_msleep(x)            Sleep((DWORD)x)

#define TLS_DT											__declspec (thread) 

#define SERPORT_HANDLE						 HANDLE

typedef unsigned short mode_t;
typedef long uid_t;
typedef long gid_t;
typedef long ssize_t;
typedef __int64 fsoff_t;
typedef DWORD MUTEX_RV;
typedef __int64 int64_t;
typedef unsigned long pid_t;

typedef HANDLE NLMTX_HANDLE;

typedef struct tagNLSHMSTRUCT {
	               unsigned long size;
                 unsigned long shmkey;
								 HANDLE hMF;
								 void *ptr;
               } NLSHMSTRUCT;

typedef struct tagCHLDEXITSTRUCT {
	               HANDLE hth;
	               unsigned long thid;
								 HANDLE exevent;
								 unsigned long value;
               } CHLDEXITSTRUCT;

typedef struct tagCHILDSSTRUCT {
	               int ch_num;
	               CHLDEXITSTRUCT chex[MAX_CHILD_NUM];
               } CHILDSSTRUCT;

typedef struct tagMRSWM {
   HANDLE hMutexNoWriter;
   HANDLE hEventNoReaders;
   HANDLE hSemNumReaders;
} MRSWM;

typedef struct tagNCANPHS {
	HANDLE ph;
	char wp[MAX_PATH+1];
	char pn[MAX_PATH+1];
	BOOL created;
	unsigned int rrcount;
} NCANPHS;

typedef struct tagDS4T {
	void *data;
	HANDLE hEvent;
} DS4T;

typedef struct tagSPCINT {
    time_t debounce;                  /* last event time for debounce */
    struct termios oldtio;
    struct termios newtio;
} SPCINT;

typedef struct tagNCASPCS {
	int ized;
	SERPORT_HANDLE fdx;
	char *device;
	SPCINT spci;
	speed_t default_speed;
	cc_t timer_read;
	OVERLAPPED overlapped;
	long syserr;
} NCASPCS;

NLMTX_HANDLE nca_x_createmutex(char *mtxname, int excl, unsigned int id);
NLMTX_HANDLE nca_x_openmutex(char *mtxname, unsigned int id);
NLMTX_HANDLE nca_x_createprivatemutex(void);

int nca_x_createmutex_mrsw(char *mtxname, int excl, unsigned int id, MRSWM *mrswm);
int nca_x_openmutex_mrsw(char *mtxname, unsigned int id, MRSWM *mrswm);
int nca_x_closemutex_mrsw(MRSWM *mrswm);
int nca_x_isvalidmutex_mrsw(MRSWM *mrswm);
DWORD nca_x_waitformutex_rw(MRSWM *mrswm);
DWORD nca_x_waitformutex_rw_tmo(MRSWM *mrswm, long tmo);
int nca_x_releasemutex_rw(MRSWM *mrswm);
DWORD nca_x_waitformutex_ro(MRSWM *mrswm);
int nca_x_releasemutex_ro(MRSWM *mrswm);
DWORD nca_x_switchmutex_rw2ro(MRSWM *mrswm);

int nca_x_mkdir(char *dirname, int mode);
int nca_x_gettimeofday(struct timeval *tp, void *tzp);

#elif defined(UNIX)

#include <ctype.h>
#include <dirent.h>
#if defined(SOLARIS)
#include <sys/mntent.h>
#include <sys/mnttab.h>
#include <sys/vfs.h>
#elif defined(FREEBSD)
#include <sys/statvfs.h>
#else
#include <mntent.h>
#include <sys/vfs.h>
#endif
#include <pwd.h>
#include <grp.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/utsname.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <sql.h>
#include <sqlext.h>
#include <odbcinst.h>
#include <pthread.h>
#include <termios.h>

/* Enable GNU extensions in fnmatch.h.  */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE	1
#endif
#include <fnmatch.h>

#if defined(HPUX)
#define OS_STR                     "HP-UX"
#elif defined(LINUX)
#define OS_STR                     "Linux"
#define MOUNTPOINT(mnt)            mnt->mnt_dir
#define ENDMNTENT_SUCCESSFULL      1
#ifdef _SEM_SEMUN_UNDEFINED
   union semun
   {
     int val;                           //value for SETVAL
     struct semid_ds *buf;              //buffer for IPC_STAT & IPC_SET
     unsigned short int *array;         //array for GETALL & SETALL
																				//linux specific
     struct seminfo *__buf;             //buffer for IPC_INFO
   };
#define HAS_UNION_SEMUN                 //because of perl
#endif
#elif defined(SOLARIS)
#define OS_STR                     "Solaris"
#define setmntent                  fopen
#define endmntent                  fclose
#define MOUNTPOINT(mnt)            mnt->mnt_mountp
#define ENDMNTENT_SUCCESSFULL      0
#ifndef	FNM_CASEFOLD
#define FNM_CASEFOLD               (FNM_IGNORECASE)  /* Compare without regard to case.  */
#endif
   union semun
   {
     int val;                           //value for SETVAL
     struct semid_ds *buf;              //buffer for IPC_STAT & IPC_SET
     ushort_t *array;                   //array for GETALL & SETALL
   };
#define HAS_UNION_SEMUN                 //because of perl
#elif defined(OSF)
#define OS_STR                     "Osf"
#elif defined(FREEBSD)
#define OS_STR                     "FreeBSD"
#else
#define OS_STR                     "Unknown"
#endif

/* Address indicating an error return.  */
#ifndef INADDR_NONE
#define INADDR_NONE                ((in_addr_t)0xffffffff)
#endif

#define OS_NULLDEV                 "/dev/null"

#define SOCKET_ERROR               (-1)
#define INVALID_SOCKET             (-1)

#define PATH_SEPARATOR             UNX_PATH_SEPARATOR

#define FOM_RA_BIN                 "a+"
#define FOM_AO_BIN                 "a"
#define FOM_RO_BIN                 "r"
#define FOM_WO_BIN                 "w"
#define FOM_RW_BIN                 "r+"
#define FOM_RWD_BIN                "w+"

#define FOM_RA_TXT                 "a+"
#define FOM_RO_TXT                 "r"
#define FOM_WO_TXT                 "w"
#define FOM_RW_TXT                 "r+"
#define FOM_RWD_TXT                "w+"

#define NEW_LINE_CHAR              "\n"

#define DPM_OWNER_ONLY             S_IRWXU
#define FPM_OWNER_ONLY             (S_IRUSR | S_IWUSR)
#define DPM_ORW_GR                 (S_IRWXU | S_IXGRP | S_IRGRP)
#define FPM_ORW_GR                 (S_IRUSR | S_IWUSR | S_IRGRP)

#define NCA_LASTONE_PROCNUM        1

#define nca_x_getsockerr()         ((long)errno)
#define nca_x_getsyserr()          ((long)errno)
#define nca_x_sleep(x)             sleep(x)
#define nca_x_msleep(x)            usleep(1000UL*x)

#define TLS_DT

#ifndef stricmp
#define stricmp                    strcasecmp
#endif

#ifndef strnicmp
#define strnicmp                   strncasecmp
#endif

#define chsize(fp,len)             ftruncate(fp,len)
#define closesocket(sock)          close(sock)

#define nca_x_gettimeofday         gettimeofday

#ifndef MAX_PATH
#define MAX_PATH 255
#endif

#define MUTEX_NCADB_BASE           ((key_t)23000)
#define MUTEX_NCACONF_BASE         ((key_t)23050)
#define MUTEX_NCASESSION_BASE      ((key_t)23100)
#define MUTEX_NCALGF_BASE          ((key_t)23150)
#define MUTEX_NCALGA_BASE          ((key_t)23200)
#define MUTEX_NCAPASSWORD_BASE     ((key_t)23250)
#define MUTEX_NCAAUTOCRL_BASE      ((key_t)23300)
#define MUTEX_NCATT_BASE           ((key_t)23350)
#define MUTEX_SSIDEF_BASE          ((key_t)23400)
#define MUTEX_LOG2_BASE            ((key_t)23450)
#define MUTEX_NCALOAD_BASE         ((key_t)23500)
#define MUTEX_LIPC_BASE            ((key_t)23550)
#define MUTEX_P11INITIALIZE_BASE   ((key_t)23600)
#define MUTEX_NCARLEMSGS_BASE			 ((key_t)23650)
#define MUTEX_NCAGC_BASE           ((key_t)23700)
#define MUTEX_NCASPC_BASE          ((key_t)23750)

#define INVALID_MUTEX              (-1)
#define INVALID_MRSW_MUTEX         (INVALID_MUTEX)
#define MUTEXWAIT_FAILED           (-1)
#define MUTEXWAIT_TIMEOUTED        (1)
#define MUTEXWAIT_INFINITE         (-1)
#define MUTEX_EXISTS               (EEXIST)
#define INVALID_HANDLE_VALUE			 (-1)

#define SHM_SESSION_KEY_BASE       ((key_t)24000)
#define SHM_NCAGPROCNUM_KEY_BASE   ((key_t)24050)
#define SHM_CHS_KEY_BASE           ((key_t)24100)
#define SHM_LOGMD_KEY_BASE         ((key_t)24150)
#define SHM_SSIDEF_KEY_BASE        ((key_t)24200)
#define SHM_AUTOCRL_KEY_BASE       ((key_t)24250)
#define SHM_LOGINATTEMPT_KEY_BASE  ((key_t)24300)
#define SHM_GPWD_KEY_BASE          ((key_t)24350)
#define SHM_GNCATT_KEY_BASE        ((key_t)24400)
#define SHM_NCALOAD_KEY_BASE       ((key_t)24450)
#define SHM_NCARLEMSGS_KEY_BASE		 ((key_t)24500)

#define NCANEWLIC_EXE_NAME         "ncanewlc"
#define NCAINIT_EXE_NAME           "ncainit"
#define NCASERVICE_EXE_NAME        "ncaadm"
#define NCATOOLS_EXE_NAME 	       "ncatools"

#define FNM_DEF_FLAGS              (0)

#define SERPORT_HANDLE							int

#define __stdcall

typedef int NLMTX_HANDLE;

typedef struct tagNLSHMSTRUCT {
                 unsigned long size;
                 key_t shmkey;
                 int shmid;
                 void *ptr;
               } NLSHMSTRUCT;

typedef struct tagCHLDEXITSTRUCT {
	               pid_t pid;
								 int exv;
								 unsigned long value;
								 unsigned long ws;
								 unsigned long ulen;
								 unsigned char udat[32];
               } CHLDEXITSTRUCT;

typedef struct tagCHILDSSTRUCT {
	               int ch_num;
	               CHLDEXITSTRUCT chex[MAX_CHILD_NUM];
               } CHILDSSTRUCT;

typedef struct tagNCANPHS {
	int ph;
	char pn[MAX_PATH+1];
	int ph2;
	char pn2[MAX_PATH+1];
	int created;
	unsigned int rrcount;
} NCANPHS;

typedef NLMTX_HANDLE MRSWM;
typedef int SOCKET;
typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr    *LPSOCKADDR;
typedef off_t fsoff_t;
typedef int MUTEX_RV;
//typedef long long int64_t;

typedef struct tagSPCINT {
    time_t debounce;                  /* last event time for debounce */
    struct termios oldtio;
    struct termios newtio;
} SPCINT;

typedef struct tagNCASPCS {
	int ized;
	SERPORT_HANDLE fdx;
	SPCINT spci;
	char *device;
	speed_t default_speed;
	cc_t timer_read;
	long syserr;
} NCASPCS;

char *strupr(char *ss);
char *strlwr(char *ss);

NLMTX_HANDLE nca_x_createmutex(key_t mtxname, int excl, unsigned int id);
NLMTX_HANDLE nca_x_openmutex(key_t mtxkey, unsigned int id);

int nca_x_createmutex_mrsw(key_t mtxname, int excl, unsigned int id, MRSWM *mrswm);
int nca_x_openmutex_mrsw(key_t mtxname, unsigned int id, MRSWM *mrswm);
int nca_x_closemutex_mrsw(MRSWM *mrswm);
int nca_x_isvalidmutex_mrsw(MRSWM *mrswm);
int nca_x_waitformutex_rw(MRSWM *mrswm);
int nca_x_releasemutex_rw(MRSWM *mrswm);
int nca_x_waitformutex_ro(MRSWM *mrswm);
int nca_x_releasemutex_ro(MRSWM *mrswm);

void child_die(void);
void child_exit(void);
int nca_x_getprocnumex(unsigned long ptt);
int nca_x_pidofme(unsigned long pid);
int nca_x_mkdir(char *dirname, mode_t mode);
int nca_createpidfile(char *pidfn, pid_t pid);
int nca_sendsig2pidfn(char *pidfn, int sig);
int nca_x_kill(int sig);

void nca_set_childsighandlers(void);
void nca_close_parenthandlers(void);

#else

#error "Unknown compiler directive defined"

#endif

typedef struct tagNCADMS {
	char file[16];
	unsigned long line;
	unsigned long flen;
} NCADMS;

typedef struct tagFSTATSTRUCT {
	char fname[MAX_PATH+1];   // file name
	time_t tscan;							// scan time
	mode_t fsst_mode;						// file mode/protection information
	time_t fsst_mtime;					// time of last modifcation
	time_t fsst_ctime;					// time of creation/last change
	fsoff_t fsst_size;						// total size, in bytes
	uid_t fsst_uid;							// user ID of owner
	gid_t fsst_gid;							// grou ID of owner
} FSTATSTRUCT;

typedef struct tagTLSDATA {
	NCANPHS *nph;
} TLSDATA;

typedef struct tagNCAAPHS {
	int init;
	int pipe1[2];
	int pipe2[2];
	int rph;
	int wph;
	int created;
	unsigned int rrcount;
} NCAAPHS;

typedef struct tagNCATMM {
  struct timeval tv;
	double m;
	unsigned int enter;
} NCATMM;

#if defined(WIN32)
extern __declspec (thread) NCATMM g_ncatmm[];
#else
extern NCATMM g_ncatmm[];
#endif

typedef unsigned long (__stdcall *thread_function)(void *initval);
typedef int (*childexit_ft)(pid_t thid, unsigned long ptt, int exv, void *udat, unsigned long ulen);
typedef int (*processfile_ft)(int ftype, char *fname, void *userdata);
typedef int (*processfileex_ft)(int ftype, FSTATSTRUCT *fss, int lastitem, void *userdata);
typedef int (*print_ft)(void *arg, char *fmt, ...);

int nca_x_closemutex(NLMTX_HANDLE mtx);
MUTEX_RV nca_x_waitformutex(NLMTX_HANDLE mtx);
MUTEX_RV nca_x_waitformutex_tmo(NLMTX_HANDLE mtx, long tmo);
int nca_x_releasemutex(NLMTX_HANDLE mtx);

unsigned long nca_x_getlasterror(void);
int nca_x_setenv(char **envp, char *env, char *val);
int nca_x_initsocket(void);
int nca_x_cleanupsocket(void);
unsigned long  nca_x_getthid(void);
unsigned long  nca_x_getspecthid(void);
int nca_x_setuserrights(char *uname, char *gname);
void nca_x_waitforexit(int die);
void nca_x_waitforchild(int waitforall, childexit_ft childexit_f);
void nca_x_printprocessstatus(void *arg, print_ft print_f);
int nca_x_setptws(unsigned long ws);
int nca_x_setptudat(unsigned long thid, void *udat, unsigned long ulen);
int nca_x_getptudat(unsigned long ptt, unsigned long ws, long idx,
										unsigned long *thid,void *udat, unsigned long *ulen);
int nca_x_getprocnum(void);
int nca_x_issubprocess(unsigned long thid, unsigned long ptval);
int nca_x_childcleaner(childexit_ft childexit_f);
unsigned long nca_x_beginthread(thread_function startaddr, unsigned long ptval, void *initval);
void *nca_x_allocshm(NLSHMSTRUCT *shm, int excl, unsigned long size, unsigned int id);
void *nca_x_openshm(NLSHMSTRUCT *shm, unsigned long size, unsigned int id);
int nca_x_freeshm(NLSHMSTRUCT *shm);
int nca_x_initmp(unsigned int id);
int nca_x_closemp(void);
int nca_x_purgedir(char *dirname, int delhome);
int nca_x_setdirrights(char *dirname, int drights, int frights);
int nca_x_copydir(char *dirname1, char *dirname2);
int nca_x_getnextfile(char *dirname, char fname[MAX_PATH+1], unsigned long *idx);
int nca_x_processfiles(char *dirname, long deep, processfile_ft processfile_f, void *userdata);
int nca_x_processfiles_ex(char *dirname, char *pattern, int enumtype,
													processfileex_ft processfileex_f, void *userdata);
int nca_x_diskfreespace(char *dir, unsigned long *mbavail, unsigned long *mbtotal,
												long *inavail, long *intotal);
int nca_x_isabsolutepath(char *path);
int nca_x_getdevname(char *dir, char dev[MAX_PATH+1]);
int nca_x_samefile(char *fname1, char *fname2);
int nca_x_getprectime(int millis, time_t *tv_sec, long *tv_msec);
int nca_x_getfstat(char *fname, FSTATSTRUCT *fss);
int nca_x_diffstat(FSTATSTRUCT *fss);
int nca_x_fnmatch(const char *pattern, const char *string, int flags);
int nca_x_npipecreatebypid(char *path, char *np, NCANPHS *nph, unsigned int rrcount, print_ft print_f);
int nca_x_npipeopen(char *pname, NCANPHS *nph, unsigned int rrcount);
int nca_x_npipeclose(NCANPHS *nph);
int nca_x_npipewrite(NCANPHS *nph, char *data, unsigned long dlen);
int nca_x_npipewritewait(NCANPHS *nph, char *data, unsigned long dlen, long sec);
int nca_x_npiperead(NCANPHS *nph, char *data, unsigned long dlen);
int nca_x_npipereadwait(NCANPHS *nph, char *data, unsigned long dlen, long sec);
int nca_x_npipegeth(NCANPHS **nph);
int nca_x_apipecreate(NCAAPHS *aph, unsigned int rrcount);
int nca_x_apipepinit(NCAAPHS *aph);
int nca_x_apipecinit(NCAAPHS *aph);
int nca_x_apipeclose(NCAAPHS *aph);
int nca_x_apipewrite(NCAAPHS *aph, char *data, unsigned long dlen);
int nca_x_apipewritewait(NCAAPHS *aph, char *data, unsigned long dlen, long sec);
int nca_x_apiperead(NCAAPHS *aph, char *data, unsigned long dlen);
int nca_x_apipereadwait(NCAAPHS *aph, char *data, unsigned long dlen, long sec);

int	nca_x_setTLS(TLSDATA *tls);
int nca_x_getch(int *ch);

int nca_x_blocksignals(void);
int nca_x_resetsignals(void);
int nca_x_processsignals(void);
int nca_x_ismyhomedir(void);
int nca_x_setcwd2modulepath(void);
int nca_x_osversion(char *vd, unsigned long vl);
int nca_x_spopen(NCASPCS *spc);
int nca_x_spclose(NCASPCS *spc);
int nca_x_spwrite(NCASPCS *spc, unsigned char *data, unsigned long dlen);
int nca_x_spread(NCASPCS *spc, char *data, unsigned long dlen, long sec);
int nca_x_spwaitonread(NCASPCS *spc, long sec);
int nca_x_spsetRTS(NCASPCS *spc);
int nca_x_spclrRTS(NCASPCS *spc);
int nca_x_spgetCTS(NCASPCS *spc, int *state);
int nca_x_spisreopenerror(NCASPCS *spc);
int nca_x_rename(char *oldpath, char *newpath);
int nca_x_lockfile(FILE *fp);
int nca_x_unlockfile(FILE *fp);
int nca_x_logsyserr(long sec, FILE *fp);

int nca_x_allocthreaddbgmh(void);
int nca_x_releasethreaddbgmh(void);
int nca_x_addptrdbgmh(char *ptr);
int nca_x_delptrdbgmh(char *ptr);
int nca_x_logdiffptrdbgmh(int loc);

int	nca_y_runbeforefork(void);
int	nca_y_runafterfork(unsigned long thid);
int	nca_y_forkinit(void);
int	nca_y_forkcleanup(void);
int	nca_y_createmainpid(void);
int	nca_y_background(void);
int nca_y_getDBpwd(char **user, char **passwd);
int nca_y_LogToEventViewer(unsigned int type, int msgnum, va_list marker);

int nca_tmm_clean(unsigned int idx);
int nca_tmm_cleanall(void);
int nca_tmm_enter(unsigned int idx);
int nca_tmm_leave(unsigned int idx);
int nca_tmm_log(unsigned int idx);

#endif /* __WINX_H_INCLUDED__ */
