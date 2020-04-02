#include "winx.h"

static CHILDSSTRUCT g_chs={0};

typedef struct tagDBGMH {
	unsigned long num;
	char **ptr;
} DBGMH;

__declspec (thread) DBGMH g_dbgmh={0,NULL};
__declspec (thread) NCATMM g_ncatmm[NCA_TMM_MAXNUM]={0};

static DWORD g_indexTLS=(DWORD)(-1);

/*******************************************************************/
int nca_x_setenv(char **envp, char *env, char *val)
{
	BOOL rval;

	*envp=NULL;
	if(env == NULL) return(-1);
	rval=SetEnvironmentVariable(env,val);
	return(rval == TRUE ? 0 : -1);
}
/*******************************************************************/
int nca_x_initsocket(void)
{
	int status;
  WSADATA Data;

	status=WSAStartup(MAKEWORD(1,1),&Data);
	return(status);
}
/*******************************************************************/
int nca_x_cleanupsocket(void)
{
	int status;

  status=WSACleanup();
	return(status);
}
/*******************************************************************/
unsigned long  nca_x_getthid(void)
{
	unsigned long thid;

	thid=(unsigned long)GetCurrentThreadId();
	return(thid);
}
/*******************************************************************/
unsigned long  nca_x_getspecthid(void)
{
	unsigned long thid;

	thid=(unsigned long)GetCurrentThreadId();
	return(thid);
}
/*******************************************************************/
int nca_x_createobjname(char *prefix, char *postfix, unsigned int id,
												char *objname, unsigned long len)
{
	int rval=-1;
	unsigned int id2,l;

	if((prefix == NULL) || (postfix == NULL) ||
		 (objname == NULL) || (len == 0L)) return(rval);
	id2=id;
	l=1;
	while((id2/=10) > 0) l++;
	if((unsigned long)(rval=(int)(strlen(prefix)+strlen(postfix)+l)) >= len) return(rval);
	sprintf(objname,"%s%s%u",prefix,postfix,id);
	return(0);
}
/*******************************************************************/
HANDLE nca_x_createmutex(char *mtxname, int excl, unsigned int id)
{
	HANDLE mtx=INVALID_MUTEX;
	SECURITY_DESCRIPTOR sd={0};
	SECURITY_ATTRIBUTES sa={0};
	char objname[128];

	if(nca_x_createobjname(mtxname,"mtx",id,objname,sizeof(objname)) != 0) return(mtx);
	if(!InitializeSecurityDescriptor(&sd,SECURITY_DESCRIPTOR_REVISION)) return(mtx);
// set NULL DACL on the SD
	if(!SetSecurityDescriptorDacl(&sd,TRUE,(PACL)NULL,FALSE)) return(mtx);
// now set up the security attributes
	sa.nLength=sizeof(SECURITY_ATTRIBUTES);
	sa.bInheritHandle=TRUE; 
	sa.lpSecurityDescriptor=&sd;
	mtx=CreateMutex(&sa,FALSE,objname);
//FIXME: Owner only
//	mtx=CreateMutex(NULL,FALSE,objname);
	if(GetLastError() == ERROR_ALREADY_EXISTS) {
		if(excl == 1) {
			CloseHandle(mtx);
			mtx=INVALID_MUTEX;
		}
	}
	return(mtx);
}
/*******************************************************************/
HANDLE nca_x_openmutex(char *mtxname, unsigned int id)
{
	HANDLE mtx=INVALID_MUTEX;
	char objname[128];

	if(nca_x_createobjname(mtxname,"mtx",id,objname,sizeof(objname)) != 0) return(mtx);
	mtx=OpenMutex(SYNCHRONIZE,FALSE,objname);
	return(mtx);
}
/*******************************************************************/
HANDLE nca_x_createprivatemutex(void)
{
	HANDLE mtx=INVALID_MUTEX;

	mtx=CreateMutex(NULL,FALSE,NULL);
	return(mtx);
}
/*******************************************************************/
int nca_x_closemutex(HANDLE mtx)
{
	BOOL rval;

	rval=CloseHandle(mtx);
	return(rval == TRUE ? 0 : -1);
}
/*******************************************************************/
DWORD nca_x_waitformutex(HANDLE mtx)
{
	DWORD rval;

//	nca_tmm_enter(NCA_TMM_WAIT);
	rval=WaitForSingleObject(mtx,INFINITE);
//	nca_tmm_leave(NCA_TMM_WAIT);
	return(rval);
}
/*******************************************************************/
DWORD nca_x_waitformutex_tmo(HANDLE mtx, long tmo)
{
	DWORD dwtmo,rval;

	if(tmo >= 0) {
		dwtmo=1000UL*tmo;
	} else {
		dwtmo=INFINITE;
	}
//	nca_tmm_enter(NCA_TMM_WAIT);
	rval=WaitForSingleObject(mtx,dwtmo);
//	nca_tmm_leave(NCA_TMM_WAIT);
	if((rval != WAIT_FAILED) && (rval != WAIT_TIMEOUT)) rval=0;
	return(rval);
}
/*******************************************************************/
int nca_x_releasemutex(HANDLE mtx)
{
	return((ReleaseMutex(mtx) == TRUE) ? 0 : -1);
}
/*******************************************************************/
int nca_x_createmutex_mrsw(char *mtxname, int excl, unsigned int id, MRSWM *mrswm)
{
	int rval=-1;
	char objname[128];

	if(mrswm == NULL) return(-1);
	// Initialize all data members to NULL so that we can
	// accurately check whether an error has occurred.
	mrswm->hMutexNoWriter=NULL;
	mrswm->hEventNoReaders=NULL;
	mrswm->hSemNumReaders=NULL;

	// This mutex guards access to the other objects
	// managed by this data structure and also indicates 
	// whether there are any writer threads writing.
	// Initially no thread owns the mutex.
	if(nca_x_createobjname(mtxname,"mtxnow",id,objname,sizeof(objname)) != 0) goto end;
	if((mrswm->hMutexNoWriter=CreateMutex(NULL,FALSE,objname)) == NULL) goto end;
	if(GetLastError() == ERROR_ALREADY_EXISTS) {
		if(excl == 1) goto end;
	}

	// Create the manual-reset event that is signalled when 
	// no reader threads are reading.
	// Initially no reader threads are reading.
	if(nca_x_createobjname(mtxname,"entnor",id,objname,sizeof(objname)) != 0) goto end;
	if((mrswm->hEventNoReaders=CreateEvent(NULL,TRUE,TRUE,objname)) == NULL) goto end;
	if(GetLastError() == ERROR_ALREADY_EXISTS) {
		if(excl == 1) goto end;
	}

	// Initialize the variable that indicates the number of 
	// reader threads that are reading.
	// Initially no reader threads are reading.
	if(nca_x_createobjname(mtxname,"semnmr",id,objname,sizeof(objname)) != 0) goto end;
	if((mrswm->hSemNumReaders=CreateSemaphore(NULL,0,0x7FFFFFFF,objname)) == NULL) goto end;
	if(GetLastError() == ERROR_ALREADY_EXISTS) {
		if(excl == 1) goto end;
	}

	rval=0;

end:

	if(rval != 0) nca_x_closemutex_mrsw(mrswm);
	return(rval);
}
/*******************************************************************/
int nca_x_openmutex_mrsw(char *mtxname, unsigned int id, MRSWM *mrswm)
{
	int rval=-1;
	char objname[128];

	if(mrswm == NULL) return(-1);
	// Initialize all data members to NULL so that we can
	// accurately check whether an error has occurred.
	mrswm->hMutexNoWriter=NULL;
	mrswm->hEventNoReaders=NULL;
	mrswm->hSemNumReaders=NULL;

	// This mutex guards access to the other objects
	// managed by this data structure and also indicates 
	// whether there are any writer threads writing.
	// Initially no thread owns the mutex.
	if(nca_x_createobjname(mtxname,"mtxnow",id,objname,sizeof(objname)) != 0) goto end;
	if((mrswm->hMutexNoWriter=OpenMutex(SYNCHRONIZE,FALSE,objname)) == NULL) goto end;

	// Create the manual-reset event that is signalled when 
	// no reader threads are reading.
	// Initially no reader threads are reading.
	if(nca_x_createobjname(mtxname,"entnor",id,objname,sizeof(objname)) != 0) goto end;
	if((mrswm->hEventNoReaders=OpenEvent(SYNCHRONIZE | EVENT_MODIFY_STATE,FALSE,objname)) == NULL) goto end;

	// Initialize the variable that indicates the number of 
	// reader threads that are reading.
	// Initially no reader threads are reading.
	if(nca_x_createobjname(mtxname,"semnmr",id,objname,sizeof(objname)) != 0) goto end;
	if((mrswm->hSemNumReaders=OpenSemaphore(SYNCHRONIZE | SEMAPHORE_MODIFY_STATE,FALSE,objname)) == NULL) goto end;

	rval=0;

end:

	if(rval != 0) nca_x_closemutex_mrsw(mrswm);
	return(rval);
}
/*******************************************************************/
int nca_x_closemutex_mrsw(MRSWM *mrswm)
{
	if(mrswm == NULL) return(-1);
	if(NULL != mrswm->hMutexNoWriter) CloseHandle(mrswm->hMutexNoWriter);
	if(NULL != mrswm->hEventNoReaders)	CloseHandle(mrswm->hEventNoReaders);
	if(NULL != mrswm->hSemNumReaders) CloseHandle(mrswm->hSemNumReaders);
	mrswm->hMutexNoWriter=NULL;
	mrswm->hEventNoReaders=NULL;
	mrswm->hSemNumReaders=NULL;
	return(0);
}
/*******************************************************************/
int nca_x_isvalidmutex_mrsw(MRSWM *mrswm)
{
	if(NULL == mrswm) return(-1);
	if(NULL == mrswm->hMutexNoWriter) return(0);
	if(NULL == mrswm->hEventNoReaders)	return(0);
	if(NULL == mrswm->hSemNumReaders) return(0);
	return(1);
}
/*******************************************************************/
DWORD nca_x_waitformutex_rw(MRSWM *mrswm)
{
	DWORD dw; 
	HANDLE aHandles[2];

   // We can write if the following are true:
   // 1. The mutex guard is available and
   //    no other threads are writing.
   // 2. No threads are reading.
	aHandles[0]=mrswm->hMutexNoWriter;
	aHandles[1]=mrswm->hEventNoReaders;
//	nca_tmm_enter(NCA_TMM_WAIT);
  dw=WaitForMultipleObjects(2,aHandles,TRUE,INFINITE);
//	nca_tmm_leave(NCA_TMM_WAIT);

	if(dw != WAIT_TIMEOUT) {
		// This thread can write to the shared data.

		// Because a writer thread is writing, the mutex 
      // should not be released. This stops other 
      // writers and readers.
	}

	return(dw);
}
/*******************************************************************/
DWORD nca_x_waitformutex_rw_tmo(MRSWM *mrswm, long tmo)
{
	DWORD rval,dwtmo;
	HANDLE aHandles[2];

	if(tmo >= 0) {
		dwtmo=1000UL*tmo;
	} else {
		dwtmo=INFINITE;
	}
   // We can write if the following are true:
   // 1. The mutex guard is available and
   //    no other threads are writing.
   // 2. No threads are reading.
	aHandles[0]=mrswm->hMutexNoWriter;
	aHandles[1]=mrswm->hEventNoReaders;
//	nca_tmm_enter(NCA_TMM_WAIT);
  rval=WaitForMultipleObjects(2,aHandles,TRUE,dwtmo);
//	nca_tmm_leave(NCA_TMM_WAIT);
	if((rval != WAIT_FAILED) && (rval != WAIT_TIMEOUT)) rval=0;
	return(rval);
}
/*******************************************************************/
int nca_x_releasemutex_rw(MRSWM *mrswm) {
  // Presumably, a writer thread calling this function has
	// successfully called WaitToWrite. This means that we
  // do not have to wait on any synchronization objects 
  // here because the writer already owns the mutex.

	// Allow other writer/reader threads to use
	// the SWMRG synchronization object.
	return((ReleaseMutex(mrswm->hMutexNoWriter) == TRUE) ? 0 : -1);
}
/*******************************************************************/
DWORD nca_x_waitformutex_ro(MRSWM *mrswm)
{
	DWORD dw; 
	LONG lPreviousCount;

  // We can read if the mutex guard is available
  // and no threads are writing.
//	nca_tmm_enter(NCA_TMM_WAIT);
  dw=WaitForSingleObject(mrswm->hMutexNoWriter,INFINITE);
//	nca_tmm_leave(NCA_TMM_WAIT);

	if(dw != WAIT_TIMEOUT) {
	// This thread can read from the shared data.

	// Increment the number of reader threads.
		ReleaseSemaphore(mrswm->hSemNumReaders,1,&lPreviousCount);
		if(lPreviousCount == 0) {
	// If this is the first reader thread, 
	// set our event to reflect this.
			ResetEvent(mrswm->hEventNoReaders);
		}

	// Allow other writer/reader threads to use
	// the SWMRG synchronization object.
		ReleaseMutex(mrswm->hMutexNoWriter);
	}

	return(dw);
}
/*******************************************************************/
int nca_x_releasemutex_ro(MRSWM *mrswm)
{
	HANDLE aHandles[2];
	LONG lNumReaders;

	// We can stop reading if the mutex guard is available,
	// but when we stop reading we must also decrement the
	// number of reader threads.
	aHandles[0]=mrswm->hMutexNoWriter;
	aHandles[1]=mrswm->hSemNumReaders;
//	nca_tmm_enter(NCA_TMM_WAIT);
	WaitForMultipleObjects(2,aHandles,TRUE,INFINITE);
//	nca_tmm_leave(NCA_TMM_WAIT);

	// Get the remaining number of readers by releasing the
	// semaphore and then restoring the count by immediately
	// performing a wait.
	ReleaseSemaphore(mrswm->hSemNumReaders,1,&lNumReaders);
//	nca_tmm_enter(NCA_TMM_WAIT);
	WaitForSingleObject(mrswm->hSemNumReaders,INFINITE);
//	nca_tmm_leave(NCA_TMM_WAIT);

	// If there are no remaining readers, 
  // set the event to relect this.
	if(lNumReaders == 0) {
  // If there are no reader threads, 
	// set our event to reflect this.
    SetEvent(mrswm->hEventNoReaders);
  }

	// Allow other writer/reader threads to use
	// the SWMRG synchronization object.
	return((ReleaseMutex(mrswm->hMutexNoWriter) == TRUE) ? 0 : -1);
}
/*******************************************************************/
DWORD nca_x_switchmutex_rw2ro(MRSWM *mrswm)
{
	DWORD dw; 
	LONG lPreviousCount;

// Increment the number of reader threads.
	ReleaseSemaphore(mrswm->hSemNumReaders,1,&lPreviousCount);
	if(lPreviousCount == 0) {
// If this is the first reader thread, 
// set our event to reflect this.
		ResetEvent(mrswm->hEventNoReaders);
	}

// Allow other writer/reader threads to use
// the SWMRG synchronization object.
	dw=(ReleaseMutex(mrswm->hMutexNoWriter) == TRUE) ? 0 : -1;

	return(dw);
}
/*******************************************************************/
int nca_x_mkdir(char *dirname, int mode)
{
	return(_mkdir(dirname));
}
/*******************************************************************/
int nca_x_setuserrights(char *uname, char *gname)
{
	return(0);
}
/*******************************************************************/
/*
void nca_x_closehandlersonexit(unsigned long lclnt)
{
	if(lclnt == NCA_INVALID_PTVALUE) return;
	if((SOCKET)lclnt != INVALID_SOCKET) closesocket((SOCKET)lclnt);
}
*/
/*******************************************************************/
void nca_x_waitforexit(int die)
{
	unsigned long thid;
	unsigned int i;
	CHLDEXITSTRUCT *chx=NULL;

	thid=GetCurrentThreadId();
	for(i=0;i < MAX_CHILD_NUM;i++) {
		if(g_chs.chex[i].thid == thid) {
			chx=&g_chs.chex[i];
			break;
		}
	}
	if(chx == NULL) return;
	if(chx->exevent == NULL) return;
	SetEvent(chx->exevent);
}
/*******************************************************************/
void nca_x_waitforchild(int waitforall, childexit_ft childexit_f)
{
	unsigned int i;
	CHLDEXITSTRUCT *chx=NULL;

	while(1) {
		for(i=0;i < MAX_CHILD_NUM;i++) {
			if(g_chs.chex[i].hth != NULL) {
				chx=&g_chs.chex[i];
				if(WaitForSingleObject(chx->hth,(DWORD)(0L)) == WAIT_OBJECT_0) {
					CloseHandle(chx->hth);
					CloseHandle(chx->exevent);

					if(childexit_f != NULL) childexit_f(chx->thid,chx->value,0,NULL,0);

					chx->value=NCA_PTT_UNKNOWN;
					chx->hth=NULL;
					chx->thid=0L;
					chx->exevent=NULL;
					g_chs.ch_num--;
				}
			}
		}
		if(!waitforall) break;
		if(g_chs.ch_num == 0) break;
		nca_x_sleep(1);
	}
}
/*******************************************************************/
void nca_x_printprocessstatus(void *arg, print_ft print_f)
{
	unsigned int i,j;
	CHLDEXITSTRUCT *chx;

	print_f(arg,"Number of active processes: %d\n",g_chs.ch_num);
	for(i=0,j=1;i < MAX_CHILD_NUM;i++) {
		chx=&g_chs.chex[i];
		if(chx->thid > 0) {
			print_f(arg,"%3d:%3d:%6ld:%4lu\n",i,j++,chx->thid,chx->value);
		}
	}
}
/*******************************************************************/
int nca_x_getprocnum(void)
{
	return(g_chs.ch_num);
}
/*******************************************************************/
int nca_x_issubprocess(unsigned long thid, unsigned long ptval)
{
	unsigned int i,j;
	CHLDEXITSTRUCT *chx;

	if(thid <= 0) return(-1);
	for(i=0,j=1;i < MAX_CHILD_NUM;i++) {
		chx=&g_chs.chex[i];
		if((chx->value != ptval) && (ptval != NCA_PTT_ANY)) continue;
		if(chx->thid == thid) return(1);
	}
	return(0);
}
/*******************************************************************/
int nca_x_childcleaner(childexit_ft childexit_f)
{
	return(0);
}
/*******************************************************************/
unsigned long nca_x_beginthread(thread_function startaddr, unsigned long ptval, void *initval)
{
#ifdef _MT
	unsigned int thid;
	HANDLE hth;
	unsigned int i;
	CHLDEXITSTRUCT *chx=NULL;

	if(g_chs.ch_num < MAX_CHILD_NUM) {
		for(i=0;i < MAX_CHILD_NUM;i++) {
			if((g_chs.chex[i].thid == 0) && (g_chs.chex[i].exevent == NULL)) {
				chx=&g_chs.chex[i];
				if((chx->exevent=CreateEvent(NULL,TRUE,FALSE,NULL)) == NULL) chx=NULL;
				break;
			}
		}
		if(chx != NULL) {
//			if((thid=_beginthread(startaddr,0,initval)) != (unsigned long)(-1L)) {
			if((hth=(HANDLE)_beginthreadex(NULL,0,startaddr,initval,CREATE_SUSPENDED,&thid)) != 0) {
				chx->value=ptval;
				chx->hth=hth;
				chx->thid=thid;
				g_chs.ch_num++;
				ResumeThread(hth);
			}
		} else thid=(unsigned long)(-1L);
	} else thid=(unsigned long)(-1L);
	return(thid);
#else
	return((unsigned long)(-1L));
#endif
}
/*******************************************************************/
void *nca_x_allocshm(NLSHMSTRUCT *shm, int excl, unsigned long size, unsigned int id)
{
	char objname[128];
	SECURITY_DESCRIPTOR sd={0};
	SECURITY_ATTRIBUTES sa={0};

	if(size == 0) return(NULL);
	shm->size=size;
	if(shm->shmkey <= NCA_SHM_BASEVALUE) {
		shm->ptr=malloc(size);
	} else {
		if(!InitializeSecurityDescriptor(&sd,SECURITY_DESCRIPTOR_REVISION)) return(NULL);
// set NULL DACL on the SD
		if(!SetSecurityDescriptorDacl(&sd,TRUE,(PACL)NULL,FALSE)) return(NULL);
// now set up the security attributes
		sa.nLength=sizeof(SECURITY_ATTRIBUTES);
		sa.bInheritHandle=TRUE; 
		sa.lpSecurityDescriptor=&sd;
//		_snprintf(name,sizeof(name),"%s_%lu",NCA_SHM_BASENAME,shm->shmkey);
		if(nca_x_createobjname(NCA_SHM_BASENAME,"shm",shm->shmkey+id,objname,sizeof(objname)) != 0) return(NULL);
    if(excl) {
      shm->hMF=CreateFileMapping(INVALID_HANDLE_VALUE,&sa,PAGE_READWRITE,0,size,objname);
//FIXME: Owner only
//      shm->hMF=CreateFileMapping(INVALID_HANDLE_VALUE,NULL,PAGE_READWRITE,0,size,name);
	  } else {
      shm->hMF=OpenFileMapping(FILE_MAP_WRITE,FALSE,objname);
		}   
    if(shm->hMF == NULL) return(NULL);
    shm->ptr=(void*)MapViewOfFile(shm->hMF,FILE_MAP_READ | FILE_MAP_WRITE,0,0,0);
	}
	if(shm->ptr == NULL) shm->size=0;
	return(shm->ptr);
}
/*******************************************************************/
void *nca_x_openshm(NLSHMSTRUCT *shm, unsigned long size, unsigned int id)
{
	char objname[128];

	if(shm->shmkey <= NCA_SHM_BASEVALUE) return(NULL);
//	_snprintf(name,sizeof(name),"%s_%lu",NCA_SHM_BASENAME,shm->shmkey);
	if(nca_x_createobjname(NCA_SHM_BASENAME,"shm",shm->shmkey+id,objname,sizeof(objname)) != 0) return(NULL);
  shm->hMF=OpenFileMapping(FILE_MAP_WRITE,FALSE,objname);
  if(shm->hMF == NULL) return(NULL);
  shm->ptr=(void*)MapViewOfFile(shm->hMF,FILE_MAP_READ | FILE_MAP_WRITE,0,0,0);
	return(shm->ptr);
}
/*******************************************************************/
int nca_x_freeshm(NLSHMSTRUCT *shm)
{
	if(shm->shmkey <= NCA_SHM_BASEVALUE) {
		if(shm->ptr != NULL) free(shm->ptr);
		shm->ptr=NULL;
	} else {
	}
	return(0);
}
/*******************************************************************/
int nca_x_initmp(unsigned int id)
{
	memset((void*)&g_chs,0,sizeof(CHILDSSTRUCT));
	if((g_indexTLS=TlsAlloc()) == (DWORD)(-1)) return(-1);
	return(0);
}
/*******************************************************************/
int nca_x_closemp(void)
{
	if(g_indexTLS != (DWORD)(-1)) TlsFree(g_indexTLS);
  memset((void*)&g_chs,0,sizeof(CHILDSSTRUCT));
	return(0);
}
/*******************************************************************/
int nca_x_purgedir(char *dirname, int delhome)
{
  HANDLE fff;
	WIN32_FIND_DATA fdt;
	DWORD attr;
  BOOL done;
  char fname[MAX_PATH+1];

  sprintf(fname,"%s\\*.*",dirname);
  done=(BOOL)((fff=FindFirstFile(fname,&fdt)) != INVALID_HANDLE_VALUE);
	if(!done) return(-1);
  while(done) {
		if(strlen(dirname)+strlen(fdt.cFileName)+1 >= sizeof(fname)) {
			FindClose(fff);
			return(-2);
		}
    sprintf(fname,"%s\\%s",dirname,fdt.cFileName);
		if((strcmp(fdt.cFileName,".") == 0) || (strcmp(fdt.cFileName,"..") == 0)) {
	    done=FindNextFile(fff,&fdt);
			continue;
		}
		if((attr=GetFileAttributes(fname)) == (DWORD)(-1)) {
			FindClose(fff);
			return(-3);
		}
    if(attr&FILE_ATTRIBUTE_DIRECTORY) {
			if(nca_x_purgedir(fname,1) != 0) {
				FindClose(fff);
				return(-4);
			}
		} else {
			if(DeleteFile(fname) == FALSE) {
				FindClose(fff);
				return(-5);
			}
		}
	  done=FindNextFile(fff,&fdt);
  }
	FindClose(fff);
  if(delhome != 0) done=RemoveDirectory(dirname);
	else done=1;
	return(done ? 0 : -1);
}
/*******************************************************************/
int nca_x_setdirrights(char *dirname, int drights, int frights)
{
  HANDLE fff;
	WIN32_FIND_DATA fdt;
	DWORD attr;
  BOOL done;
  char fname[MAX_PATH+1];

  sprintf(fname,"%s\\*.*",dirname);
  done=(BOOL)((fff=FindFirstFile(fname,&fdt)) != INVALID_HANDLE_VALUE);
	if(!done) return(-1);
  while(done) {
		if(strlen(dirname)+strlen(fdt.cFileName)+1 >= sizeof(fname)) {
			FindClose(fff);
			return(-2);
		}
    sprintf(fname,"%s\\%s",dirname,fdt.cFileName);
		if((strcmp(fdt.cFileName,".") == 0) || (strcmp(fdt.cFileName,"..") == 0)) {
	    done=FindNextFile(fff,&fdt);
			continue;
		}
		if((attr=GetFileAttributes(fname)) == (DWORD)(-1)) {
			FindClose(fff);
			return(-3);
		}
    if(attr&FILE_ATTRIBUTE_DIRECTORY) {
			if(nca_x_setdirrights(fname,drights,frights) != 0) {
				FindClose(fff);
				return(-4);
			}
		} else {
			if(_chmod(fname,frights) != 0) {
				FindClose(fff);
				return(-5);
			}
		}
	  done=FindNextFile(fff,&fdt);
  }
	FindClose(fff);
  done=_chmod(dirname,drights);
	return(done);
}
/*******************************************************************/
int nca_x_copydir(char *dirname1, char *dirname2)
{
  HANDLE fff;
	WIN32_FIND_DATA fdt;
	DWORD attr;
  BOOL done;
  char fname[MAX_PATH+1],fname2[MAX_PATH+1],*dp;

	if((dirname1 == NULL) || (dirname2 == NULL)) return(-1);
  sprintf(fname,"%s\\*.*",dirname1);
  done=(BOOL)((fff=FindFirstFile(fname,&fdt)) != INVALID_HANDLE_VALUE);
	if(!done) return(-2);
  while(done) {
		if(strlen(dirname1)+strlen(fdt.cFileName)+1 >= sizeof(fname)) {
			FindClose(fff);
			return(-3);
		}
    sprintf(fname,"%s\\%s",dirname1,fdt.cFileName);
		if((strcmp(fdt.cFileName,".") == 0) || (strcmp(fdt.cFileName,"..") == 0)) {
	    done=FindNextFile(fff,&fdt);
			continue;
		}
		if((strlen(fname) <= strlen(dirname1)) || (strstr(fname,dirname1) == NULL)) {
			FindClose(fff);
			return(-4);
		}
		dp=fname+strlen(dirname1);
		if(strlen(dirname2)+strlen(dp) >= sizeof(fname2)) {
			FindClose(fff);
			return(-5);
		}
		sprintf(fname2,"%s%s",dirname2,dp);
		if((attr=GetFileAttributes(fname)) == (DWORD)(-1)) {
			FindClose(fff);
			return(-6);
		}
    if(attr&FILE_ATTRIBUTE_DIRECTORY) {
			if(_mkdir(fname2) != 0) {
				FindClose(fff);
				return(-7);
			}
			if(nca_x_copydir(fname,fname2) != 0) {
				FindClose(fff);
				return(-8);
			}
		} else {
			if(CopyFile(fname,fname2,TRUE) == FALSE) {
				FindClose(fff);
				return(-9);
			}
		}
	  done=FindNextFile(fff,&fdt);
  }
	FindClose(fff);
	return(done);
}
/*******************************************************************/
int nca_x_getnextfile(char *dirname, char fname[MAX_PATH+1], unsigned long *idx)
{
	int rval=-1;
  HANDLE fff;
	WIN32_FIND_DATA fdt;
	DWORD attr;
  BOOL done;
	unsigned long i=0;

	if(dirname == NULL) return(-1);
	if(idx == NULL) return(-2);
	if(*idx == 0) return(-3);
  sprintf(fname,"%s\\*.*",dirname);
  done=(BOOL)((fff=FindFirstFile(fname,&fdt)) != INVALID_HANDLE_VALUE);
	if(!done) return(-4);
  while(done) {
		if(strlen(dirname)+strlen(fdt.cFileName)+1 >= MAX_PATH) {
			FindClose(fff);
			return(-5);
		}
    sprintf(fname,"%s\\%s",dirname,fdt.cFileName);
		if((strcmp(fdt.cFileName,".") == 0) || (strcmp(fdt.cFileName,"..") == 0)) {
	    done=FindNextFile(fff,&fdt);
			continue;
		}
		if((attr=GetFileAttributes(fname)) == (DWORD)(-1)) {
			FindClose(fff);
			return(-6);
		}
    if(attr&FILE_ATTRIBUTE_DIRECTORY) {
/*
        sprintf(dnamer,"%s",fname);
				if(nca_x_getnextfile(dnamer,fname,recursive,idx) < 0) {
					FindClose(fff);
					return(-1);
				}
				break;
*/
		} else {
/*
			if(DeleteFile(fname) == FALSE) {
				FindClose(fff);
				return(-1);
			}
*/
			i++;
		  if(*idx == i) break;
		}
	  done=FindNextFile(fff,&fdt);
  }

	if(done) {
		(*idx)++;
		rval=1;
	}	else {
		if(GetLastError() == ERROR_NO_MORE_FILES) rval=0;
		else rval=-1;
	}

	FindClose(fff);
	return(rval);
}
/*******************************************************************/
int nca_x_processfiles(char *dirname, long deep, processfile_ft processfile_f, void *userdata)
{
	int rval=-1,pd;
  HANDLE fff;
	WIN32_FIND_DATA fdt;
	DWORD attr;
  BOOL done;
	char fname[MAX_PATH+1];

	if((dirname == NULL) || (processfile_f == NULL)) return(-1);
  sprintf(fname,"%s\\*.*",dirname);
  done=(BOOL)((fff=FindFirstFile(fname,&fdt)) != INVALID_HANDLE_VALUE);
	if(!done) return(-2);
	if(deep > 0) deep--;
  while(done) {
		if(strlen(dirname)+strlen(fdt.cFileName)+1 >= sizeof(fname)) {
			FindClose(fff);
			return(-3);
		}
    sprintf(fname,"%s\\%s",dirname,fdt.cFileName);
		if((strcmp(fdt.cFileName,".") == 0) || (strcmp(fdt.cFileName,"..") == 0)) {
	    done=FindNextFile(fff,&fdt);
			continue;
		}
		if((attr=GetFileAttributes(fname)) == (DWORD)(-1)) {
			FindClose(fff);
			return(-5);
		}
    if(attr&FILE_ATTRIBUTE_DIRECTORY) {
			if(processfile_f(NCA_FTYPE_DIR,fname,userdata) != 0) {
				FindClose(fff);
				return(-4);
			}
			if(deep < 0) {
				pd=1;
			} else {
				if(deep > 0) {
					pd=1;
				} else {
					pd=0;
				}
			}
			if(pd == 1) {
				if(nca_x_processfiles(fname,deep,processfile_f,userdata) != 0) {
					FindClose(fff);
					return(-6);
				}
			}
		} else {
			if(processfile_f(NCA_FTYPE_FILE,fname,userdata) != 0) {
				FindClose(fff);
				return(-7);
			}
		}
	  done=FindNextFile(fff,&fdt);
  }
	if(GetLastError() == ERROR_NO_MORE_FILES) rval=0;
	FindClose(fff);
	return(rval);
}
/*******************************************************************/
static int nca_x_filetimetounxtime(FILETIME *ft, time_t *ut)
{
	SYSTEMTIME st;
	struct tm tms1,*tms2;
	time_t mirrortime;

	if((ft == NULL) || (ut == NULL)) return(-1);
	if(!FileTimeToSystemTime(ft,&st)) return(-1);
	tms1.tm_year=st.wYear-1900;
	tms1.tm_mon=st.wMonth-1;
	tms1.tm_mday=st.wDay;
	tms1.tm_sec=st.wSecond;
	tms1.tm_min=st.wMinute; 
	tms1.tm_hour=st.wHour;
	tms1.tm_isdst=-1;
	if((*ut=mktime(&tms1)) == (time_t)(-1)) return(-1);
  if((tms2=gmtime(ut)) == NULL) return(-1);
  if((mirrortime=mktime(tms2)) == (time_t)(-1)) return(-1);
	(*ut)-=(mirrortime-*ut);
	return(0);
}
/*******************************************************************/
int nca_x_processfiles_ex(char *dirname, char *pattern, int enumtype,
													processfileex_ft processfileex_f, void *userdata)
{
	int rval=-1,etidx=0,ret;
	unsigned long etval;
  HANDLE fff;
	WIN32_FIND_DATA fdt;
	DWORD attr;
  BOOL done;
	FSTATSTRUCT fss;

	if((dirname == NULL) || (processfileex_f == NULL)) return(-1);
	do {
		switch(enumtype) {
			case NCA_FFET_NOORDER:
				etval=NCA_FFET_DIRSNOW|NCA_FFET_FILESNOW;
			  break;
			case NCA_FFET_DIRSFIRST:
				if(etidx == 0) etval=NCA_FFET_DIRSNOW;
				else etval=NCA_FFET_FILESNOW;
				break;
			case NCA_FFET_FILESFIRST:
				if(etidx == 0) etval=NCA_FFET_FILESNOW;
				else etval=NCA_FFET_DIRSNOW;
				break;
			default:
				return(-2);
		}
//		sprintf(fss.fname,"%s\\*.*",dirname);
	  sprintf(fss.fname,"%s\\%s",dirname,(pattern == NULL ? "*.*" : pattern));
		done=(BOOL)((fff=FindFirstFile(fss.fname,&fdt)) != INVALID_HANDLE_VALUE);
		if(!done) return(-3);
		while(done) {
			if(strlen(dirname)+strlen(fdt.cFileName)+1 >= sizeof(fss.fname)) {
				FindClose(fff);
				return(-4);
			}
			sprintf(fss.fname,"%s\\%s",dirname,fdt.cFileName);
			if(nca_x_filetimetounxtime(&fdt.ftCreationTime,&fss.fsst_ctime) != 0) {
				FindClose(fff);
				return(-5);
			}
			if(strcmp(fdt.cFileName,".") == 0) {
				if((etval&NCA_FFET_DIRSNOW) == 0) goto cont;
				if(processfileex_f(NCA_FTYPE_DIR,&fss,0,userdata) != 0) {
					FindClose(fff);
					return(-6);
				}
				done=FindNextFile(fff,&fdt);
				continue;
			}
			if(strcmp(fdt.cFileName,"..") == 0) {
				done=FindNextFile(fff,&fdt);
				continue;
			}
			if((attr=GetFileAttributes(fss.fname)) == (DWORD)(-1)) {
				FindClose(fff);
				return(-7);
			}
			if(attr&FILE_ATTRIBUTE_DIRECTORY) {
				if((etval&NCA_FFET_DIRSNOW) == 0) goto cont;
				if((ret=nca_x_processfiles_ex(fss.fname,pattern,enumtype,processfileex_f,userdata)) != 0) {
					FindClose(fff);
					return(ret);
				}
			} else {
				if((etval&NCA_FFET_FILESNOW) == 0) goto cont;
				if(processfileex_f(NCA_FTYPE_FILE,&fss,0,userdata) != 0) {
					FindClose(fff);
					return(-8);
				}
			}

cont:

			done=FindNextFile(fff,&fdt);
		}
		if(GetLastError() == ERROR_NO_MORE_FILES) {
			if(processfileex_f(NCA_FTYPE_DIR,&fss,etidx+1,userdata) != 0) {
				FindClose(fff);
				return(-9);
			}
			rval=0;
		}
		FindClose(fff);
		if(enumtype == NCA_FFET_NOORDER) {
			etidx=2;
		}	else {
			etidx++;
		}
	}	while(etidx < 2);

	return(rval);
}
/*******************************************************************/
int nca_x_diskfreespace(char *dir, unsigned long *mbavail, unsigned long *mbtotal,
												long *inavail, long *intotal)
{
	int rval=-1;
  unsigned __int64 i64FreeBytesToCaller,i64TotalBytes,i64FreeBytes;

  if(GetDiskFreeSpaceEx(dir,(PULARGE_INTEGER)&i64FreeBytesToCaller,
                            (PULARGE_INTEGER)&i64TotalBytes,
                            (PULARGE_INTEGER)&i64FreeBytes)) {
		if(mbavail != NULL) *mbavail=(unsigned long)(i64FreeBytesToCaller/(1024*1024));
		if(mbtotal != NULL) *mbtotal=(unsigned long)(i64TotalBytes/(1024*1024));
		if(inavail != NULL) *inavail=0;
		if(intotal != NULL) *intotal=0;
  	rval=0;
	}
 
	return(rval);
}
/*******************************************************************/
int nca_x_isabsolutepath(char *path)
{
	if(path == NULL) return(-1);
	if(strlen(path) < 2) return(0);
	if(*(path+1) == ':') return(1);
	return(0);
}
/*******************************************************************/
int nca_x_getdevname(char *dir, char dev[MAX_PATH+1])
{
	int rval=-1;
	char cwd[MAX_PATH+1];

	if(_getcwd(cwd,MAX_PATH) == NULL) goto end;
	if(dir == NULL) {
		strcpy(dev,cwd);
	} else {
		if(strlen(dir) > MAX_PATH) goto end;
		if(_strnicmp(dir,cwd,2) == 0) {
			strcpy(dev,cwd);
		} else {
			strncpy(dev,dir,2);
			dev[2]='\\';
			dev[3]='\0';
		}
	}
	if(strlen(dev) != 3) dev[3]='\0';
	_strlwr(dev);

	rval=0;

end:

	return(rval);
}
/*******************************************************************/
int nca_x_getfstat(char *fname, FSTATSTRUCT *fss)
{
	int rval=-1;
	struct _stati64 buf;
	char disk[4];

	if((fname == NULL) || (fss == NULL)) goto end;
	if(strlen(fname) >= sizeof(fss->fname)) goto end;
	if((strlen(fname) == 2) && (*(fname+1) == ':')) {
		strcpy(disk,fname);
		disk[2]='\\';
		disk[3]='\0';
		if(_stati64(disk,&buf) != 0) goto end;
	} else {
		if(_stati64(fname,&buf) != 0) goto end;
	}
	fss->tscan=time(NULL);
	strcpy(fss->fname,fname);
	fss->fsst_ctime=buf.st_ctime;
	fss->fsst_mtime=buf.st_mtime;
	fss->fsst_size=buf.st_size;
	fss->fsst_gid=buf.st_gid;
	fss->fsst_uid=buf.st_uid;
	fss->fsst_mode=buf.st_mode;

	rval=0;

end:

	return(rval);
}
/*******************************************************************/
int nca_x_diffstat(FSTATSTRUCT *fss)
{
	int rval=-1;
	FSTATSTRUCT fss2={0};

	if(fss == NULL) goto end;
	if(nca_x_getfstat(fss->fname,&fss2) != 0) goto end;
	if(fss->fsst_ctime != fss2.fsst_ctime) return(NCA_FSTATCH_CTIME);
	if(fss->fsst_mtime != fss2.fsst_mtime) {
		if(fss2.fsst_mtime >= fss->tscan) return(NCA_FSTATCH_MTIME);
	}
	if(fss->fsst_size != fss2.fsst_size) return(NCA_FSTATCH_SIZE);
	if(fss->fsst_uid != fss2.fsst_uid) return(NCA_FSTATCH_UID);
	if(fss->fsst_gid != fss2.fsst_gid) return(NCA_FSTATCH_GID);
	if(fss->fsst_mode != fss2.fsst_mode) return(NCA_FSTATCH_MODE);

	rval=0;

end:

	return(rval);
}
/*******************************************************************/
int nca_x_samefile(char *fname1, char *fname2)
{
	struct _stati64 buf1,buf2;

	if((fname1 == NULL) || (fname2 == NULL)) return(-1);
	if(*fname1 == '\0' || (*fname2 == '\0')) return(-1);
	if(strcmp(fname1,fname2) == 0) return(1);
	errno=0;
	if(_stati64(fname1,&buf1) != 0) {
		if(errno == ENOENT) return(0);
		return(-1);
	}
	if(_stati64(fname2,&buf2) != 0) {
		if(errno == ENOENT) return(0);
		return(-1);
	}
	if((buf1.st_dev == buf2.st_dev) && 
	   (buf1.st_ino == buf2.st_ino) &&
		 (buf1.st_mode == buf2.st_mode) &&
		 (buf1.st_uid == buf2.st_uid) &&
		 (buf1.st_gid == buf2.st_gid)	&&
		 (buf1.st_size == buf2.st_size)	&&
		 (buf1.st_atime == buf2.st_atime)	&&
		 (buf1.st_mtime == buf2.st_mtime)	&&
		 (buf1.st_ctime == buf2.st_ctime)) return(1);
	return(0);
}
/*******************************************************************/
int nca_x_getprectime(int millis, time_t *tv_sec, long *tv_msec)
{
	struct _timeb tptr;

	_ftime(&tptr);
	if(tv_sec != NULL) *tv_sec=tptr.time;
	if(tv_msec != NULL) {
		if(millis == 0) *tv_msec=0L;
		else if(millis == 10) *tv_msec=(long)(tptr.millitm-tptr.millitm%100);
		else if(millis == 100) *tv_msec=(long)(tptr.millitm-tptr.millitm%10);
		else if(millis == 1000) *tv_msec=(long)tptr.millitm;
		else return(-1);
	}
	return(0);
}
/*******************************************************************/
/* Match STRING against the filename pattern PATTERN, returning zero if
   it matches, nonzero if not.  */
int nca_x_fnmatch(const char *pattern, const char *string, int flags)
{
  register const char *p = pattern, *n = string;
  register char c;

/* Note that this evaluates C many times.  */
#define FOLD(c) ((flags & FNM_CASEFOLD) && ISUPPER ((unsigned char) (c)) \
                 ? tolower ((unsigned char) (c)) \
                 : (c))

	if((p == NULL) || (n == NULL)) return(-1);

  while((c = *p++) != '\0') {
      c = FOLD (c);

      switch (c) {
	case '?':
	  if (*n == '\0')
	    return FNM_NOMATCH;
	  else if ((flags & FNM_FILE_NAME) && *n == '/')
	    return FNM_NOMATCH;
	  else if ((flags & FNM_PERIOD) && *n == '.' &&
		   (n == string || ((flags & FNM_FILE_NAME) && n[-1] == '/')))
	    return FNM_NOMATCH;
	  break;

	case '\\':
	  if (!(flags & FNM_NOESCAPE))
	    {
	      c = *p++;
	      if (c == '\0')
		/* Trailing \ loses.  */
		return FNM_NOMATCH;
	      c = FOLD (c);
	    }
	  if (FOLD (*n) != c)
	    return FNM_NOMATCH;
	  break;

	case '*':
	  if ((flags & FNM_PERIOD) && *n == '.' &&
	      (n == string || ((flags & FNM_FILE_NAME) && n[-1] == '/')))
	    return FNM_NOMATCH;

	  for (c = *p++; c == '?' || c == '*'; c = *p++)
	    {
	      if (c == '?')
		{
		  /* A ? needs to match one character.  */
		  if (*n == '\0' || (*n == '/' && (flags & FNM_FILE_NAME)))
		    /* There isn't another character; no match.  */
		    return FNM_NOMATCH;
		  else
		    /* One character of the string is consumed in matching
		       this ? wildcard, so *??? won't match if there are
		       less than three characters.  */
		    ++n;
		}
	    }

	  if (c == '\0')
	    {
	      if ((flags & (FNM_FILE_NAME | FNM_LEADING_DIR)) == FNM_FILE_NAME)
		for (; *n != '\0'; n++)
		  if (*n == '/')
		    return FNM_NOMATCH;
	      return 0;
	    }

	  {
	    char c1 = (!(flags & FNM_NOESCAPE) && c == '\\') ? *p : c;
	    c1 = FOLD (c1);
	    for (--p; *n != '\0'; ++n)
	      if ((c == '[' || FOLD (*n) == c1) &&
		  nca_x_fnmatch (p, n, flags & ~FNM_PERIOD) == 0)
		return 0;
	      else if (*n == '/' && (flags & FNM_FILE_NAME))
		break;
	    return FNM_NOMATCH;
	  }

	case '[':
	  {
	    /* Nonzero if the sense of the character class is inverted.  */
	    register int not;

	    if (*n == '\0')
	      return FNM_NOMATCH;

	    if ((flags & FNM_PERIOD) && *n == '.' &&
		(n == string || ((flags & FNM_FILE_NAME) && n[-1] == '/')))
	      return FNM_NOMATCH;

	    not = (*p == '!' || *p == '^');
	    if (not)
	      ++p;

	    c = *p++;
	    for (;;)
	      {
		register char cstart = c, cend = c;

		if (!(flags & FNM_NOESCAPE) && c == '\\')
		  {
		    if (*p == '\0')
		      return FNM_NOMATCH;
		    cstart = cend = *p++;
		  }

		cstart = cend = FOLD (cstart);

		if(c == '\0')
		  /* [ (unterminated) loses.  */
		  return FNM_NOMATCH;

		c=*p++;
		c=FOLD (c);

		if ((flags & FNM_FILE_NAME) && c == '/')
		  /* [/] can never match.  */
		  return FNM_NOMATCH;

		if (c == '-' && *p != ']')
		  {
		    cend = *p++;
		    if (!(flags & FNM_NOESCAPE) && cend == '\\')
		      cend = *p++;
		    if (cend == '\0')
		      return FNM_NOMATCH;
		    cend = FOLD (cend);

		    c = *p++;
		  }

		if (FOLD (*n) >= cstart && FOLD (*n) <= cend)
		  goto matched;

		if (c == ']')
		  break;
	      }
	    if (!not)
	      return FNM_NOMATCH;
	    break;

	  matched:;
	    /* Skip the rest of the [...] that already matched.  */
	    while (c != ']')
	      {
		if (c == '\0')
		  /* [... (unterminated) loses.  */
		  return FNM_NOMATCH;

		c = *p++;
		if (!(flags & FNM_NOESCAPE) && c == '\\')
		  {
		    if (*p == '\0')
		      return FNM_NOMATCH;
		    /* XXX 1003.2d11 is unclear if this is right.  */
		    ++p;
		  }
	      }
	    if (not)
	      return FNM_NOMATCH;
	  }
	  break;

	default:
	  if (c != FOLD (*n))
	    return FNM_NOMATCH;
	}

      ++n;
    }

  if(*n == '\0')
    return 0;

  if ((flags & FNM_LEADING_DIR) && *n == '/')
    /* The FNM_LEADING_DIR flag says that "foo*" matches "foobar/frobozz".  */
    return 0;

  return FNM_NOMATCH;

#undef FOLD
}
/*******************************************************************/
int nca_x_npipecreatebypid(char *path, char *np, NCANPHS *nph, unsigned int rrcount, print_ft print_f)
{
	int rval=-1;
	unsigned long  thid=nca_x_getthid();
	FILE *fp=NULL;
	OVERLAPPED ovlp={0};
	HANDLE hevent=NULL;
	DWORD ret;
	SECURITY_DESCRIPTOR sd={0};
	SECURITY_ATTRIBUTES sa={0};

	if((nph == NULL) || (path == NULL)) goto end;

	nph->created=FALSE;
	nph->pn[MAX_PATH]='\0';
	nph->wp[MAX_PATH]='\0';
	nph->ph=INVALID_HANDLE_VALUE;
	nph->rrcount=rrcount;
	_snprintf(nph->pn,MAX_PATH,"%s\\%s%ld",path,np,thid);
	if((fp=fopen(nph->pn,FOM_WO_BIN)) == NULL) {
		nph->pn[MAX_PATH]='\0';
		goto end;
	}
	fclose(fp);
	fp=NULL;

	if(!InitializeSecurityDescriptor(&sd,SECURITY_DESCRIPTOR_REVISION)) goto end;
// set NULL DACL on the SD
	if(!SetSecurityDescriptorDacl(&sd,TRUE,(PACL)NULL,FALSE)) goto end;
// now set up the security attributes
	sa.nLength=sizeof(SECURITY_ATTRIBUTES);
	sa.bInheritHandle=TRUE; 
	sa.lpSecurityDescriptor=&sd;

	sprintf(nph->wp,"\\\\.\\pipe\\%s%ld",np,thid);
	if((nph->ph=CreateNamedPipe(nph->wp,PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,PIPE_TYPE_BYTE|PIPE_READMODE_BYTE|PIPE_WAIT,
		                          2,1024,1024,1000,&sa)) == INVALID_HANDLE_VALUE) goto end;
	nph->created=TRUE;
	if((hevent=CreateEvent(NULL,TRUE,FALSE,NULL)) == NULL) goto end;
	ovlp.hEvent=hevent;
	if(!ConnectNamedPipe(nph->ph,&ovlp)) {
		if((ret=GetLastError()) != ERROR_IO_PENDING) goto end;
	}

	do {

		print_f(NULL,"-");

		if((ret=WaitForSingleObject(hevent,1000)) == WAIT_TIMEOUT) {
			if(nph->rrcount > 0) nph->rrcount--;
		} else {
			break;
		}
	} while(nph->rrcount > 0);

	if(ret != WAIT_OBJECT_0) goto end;

	rval=0;

end:

	if(fp != NULL) fclose(fp);
	if(hevent != NULL) CloseHandle(hevent);
	return(rval);
}
/*******************************************************************/
int nca_x_npipeopen(char *pname, NCANPHS *nph, unsigned int rrcount)
{
	char *p;
	SECURITY_DESCRIPTOR sd={0};
	SECURITY_ATTRIBUTES sa={0};

	if((nph == NULL) || (pname == NULL)) return(-1);

	nph->created=FALSE;
	nph->pn[MAX_PATH]='\0';
	nph->wp[MAX_PATH]='\0';
	nph->ph=INVALID_HANDLE_VALUE;
	nph->rrcount=rrcount;
	if(strlen(pname) >= sizeof(nph->wp)) return(-1);
	if((p=strrchr(pname,WIN_PATH_SEPARATOR)) == NULL) p=pname;
	else p++;

	if(!InitializeSecurityDescriptor(&sd,SECURITY_DESCRIPTOR_REVISION)) return(-1);
// set NULL DACL on the SD
	if(!SetSecurityDescriptorDacl(&sd,TRUE,(PACL)NULL,FALSE)) return(-1);
// now set up the security attributes
	sa.nLength=sizeof(SECURITY_ATTRIBUTES);
	sa.bInheritHandle=TRUE; 
	sa.lpSecurityDescriptor=&sd;

	sprintf(nph->wp,"\\\\.\\pipe\\%s",p);
  nph->ph=CreateFile(nph->wp,   // pipe name 
										 GENERIC_READ | GENERIC_WRITE,// read and write access 
									   0,              // no sharing 
						         &sa,           // no security attributes
						         OPEN_EXISTING,  // opens existing pipe 
						         0,              // default attributes 
										 NULL);          // no template file 
  if(nph->ph == INVALID_HANDLE_VALUE) return(-1); 

	return(0);
}
/*******************************************************************/
int nca_x_npipeclose(NCANPHS *nph)
{
	if(nph == NULL) return(-1);
	if(nph->created && (nph->ph != INVALID_HANDLE_VALUE)) DisconnectNamedPipe(nph->ph);
	if(nph->ph != INVALID_HANDLE_VALUE) CloseHandle(nph->ph);
	if(strlen(nph->pn) != 0) DeleteFile(nph->pn);
	memset((void*)nph,0,sizeof(NCANPHS));
	return(0);
}
/*******************************************************************/
int nca_x_npipewrite(NCANPHS *nph, char *data, unsigned long dlen)
{
	DWORD wb;

	if((nph == NULL) || (data == NULL) || (dlen == 0)) return(-1);
	if(!WriteFile(nph->ph,(void*)data,(DWORD)dlen,&wb,NULL)) return(-1);
	if((DWORD)dlen != wb) return(-1);
	return(0);
}
/*******************************************************************/
int nca_x_npipewritewait(NCANPHS *nph, char *data, unsigned long dlen, long sec)
{
	DWORD wb;

	if((nph == NULL) || (data == NULL) || (dlen == 0)) return(-1);
	if(!WriteFile(nph->ph,(void*)data,(DWORD)dlen,&wb,NULL)) return(-1);
	if((DWORD)dlen != wb) return(-1);
	return(1);
}
/*******************************************************************/
int nca_x_npiperead(NCANPHS *nph, char *data, unsigned long dlen)
{
	DWORD rb;

	if((nph == NULL) || (data == NULL) || (dlen == 0)) return(-1);
	if(!ReadFile(nph->ph,(void*)data,(DWORD)dlen,&rb,NULL)) return(-1);
	if((DWORD)dlen != rb) return(-1);
	return(0);
}
/*******************************************************************/
int nca_x_npipereadwait(NCANPHS *nph, char *data, unsigned long dlen, long sec)
{
	DWORD rb;

	if((nph == NULL) || (data == NULL) || (dlen == 0)) return(-1);
	if(!ReadFile(nph->ph,(void*)data,(DWORD)dlen,&rb,NULL)) return(-1);
	if((DWORD)dlen != rb) return(-1);
	return(1);
}
/*******************************************************************/
int nca_x_npipegeth(NCANPHS **nph)
{
	TLSDATA *tls;

  if(nph == NULL) return(-1);
	*nph=NULL;

	if((tls=(TLSDATA *)TlsGetValue(g_indexTLS)) == NULL) return(0);
	if((*nph=tls->nph) == NULL) return(0);

	return(1);
}
/*******************************************************************/
int nca_x_apipecreate(NCAAPHS *aph, unsigned int pwt)
{
	int rval=-1;

	return(rval);
}
/*******************************************************************/
int nca_x_apipepinit(NCAAPHS *aph)
{
	int rval=-1;

	return(rval);
}
/*******************************************************************/
int nca_x_apipecinit(NCAAPHS *aph)
{
	int rval=-1;

	return(rval);
}
/*******************************************************************/
int nca_x_apipeclose(NCAAPHS *aph)
{
	int rval=-1;

	return(rval);
}
/*******************************************************************/
int nca_x_apipewrite(NCAAPHS *aph, char *data, unsigned long dlen)
{
	int rval=-1;

	return(rval);
}
/*******************************************************************/
int nca_x_apiperead(NCAAPHS *aph, char *data, unsigned long dlen)
{
	int rval=-1;

	return(rval);
}
/*******************************************************************/
int nca_x_apipereadwait(NCAAPHS *aph, char *data, unsigned long dlen, long sec)
{
	int rval=-1;

	return(rval);
}
/*******************************************************************/
int	nca_x_setTLS(TLSDATA *tls)
{
	if(g_indexTLS == (DWORD)(-1)) return(-1);
	if(!TlsSetValue(g_indexTLS,(LPVOID)tls)) return(-1);
	return(0);
}
/*******************************************************************/
int nca_x_getch(int *ch)
{
	int ret;
	DWORD mode,get;
	unsigned char input[8];
	NCANPHS *nph;
	char d1;

	if(ch == NULL) return(-1);
	if((ret=nca_x_npipegeth(&nph)) == 1) {
//		if(nca_x_npipewrite(nph,"\0ncacon$",9) != 0) return(-1);
		if(nca_x_npipewrite(nph,"\0",1) != 0) return(-1);
		if(nca_x_npipewrite(nph,NCA_NCACON_NAME,(unsigned long)strlen(NCA_NCACON_NAME)+1) != 0) return(-1);
		if(nca_x_npiperead(nph,&d1,1UL) != 0) return(-1);
		*ch=(int)(unsigned char)d1;
		return(0);
	}

	if(!GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE),&mode)) return(-1);
  if(!SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE),0)) return(-1);
  if(!ReadConsole(GetStdHandle(STD_INPUT_HANDLE),(void*)input,1,&get,NULL)) {
		SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE),mode);
		return(-1);
	}
	SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE),mode);
	*ch=(int)input[0];
  return(0);
}
/*******************************************************************/
int nca_x_blocksignals(void)
{
	return(0);
}
/*******************************************************************/
int nca_x_resetsignals(void)
{
	return(0);
}
/*******************************************************************/
int nca_x_processsignals(void)
{
	return(0);
}
/*******************************************************************/
int nca_x_gettimeofday(struct timeval *tp, void *tzp)
{
  struct _timeb timebuf;

  if(tp == NULL) return(-1);
  _ftime(&timebuf);
  tp->tv_sec=(long)timebuf.time;
  tp->tv_usec=(long)(timebuf.millitm)*(1000000/1000);
  return(0);
}
/*******************************************************************/
int nca_x_ismyhomedir(void)
{
	int rval=-1;
  char *p,p1[1024],p2[1024];
	DWORD ret;

	if(!GetModuleFileName(NULL,p1,sizeof(p1))) goto end;
	if((p=strrchr(p1,PATH_SEPARATOR)) == NULL) goto end;
	*p='\0';
	if((ret=GetCurrentDirectory(sizeof(p2),p2)) == 0) goto end;
	if((ret > sizeof(p2)) || (ret < 1)) goto end;
	p=p2+strlen(p2)-1;
	if(*p == PATH_SEPARATOR) *p='\0';
	if(strcmp(p1,p2) == 0) rval=1;
	else rval=0;

end:

	return(rval);
}
/*******************************************************************/
int nca_x_setcwd2modulepath(void)
{
	int rval=-1;
  char *p,exe[1024];

	if(!GetModuleFileName(NULL,exe,sizeof(exe))) goto end;
	if((p=strrchr(exe,PATH_SEPARATOR)) == NULL) goto end;
	*(p+1)='\0';
	if(!SetCurrentDirectory(exe)) goto end;

	rval=0;

end:

	return(rval);
}
/*******************************************************************/
int nca_x_osversion(char *vd, unsigned long vl)
{
	unsigned int _osver=1,_winmajor=1,_winminor=1,_winver=1;

	snprintf(vd,vl,"%u.%u.%u.%u",_osver,_winmajor,_winminor,_winver);
	return(0);
}
/*******************************************************************/
int tcflush(HANDLE fdHandle, int queue)
{
  if(queue & (TCOFLUSH | TCIOFLUSH)) {
	  if(!PurgeComm(fdHandle,PURGE_TXABORT | PURGE_TXCLEAR)) return(-1);
	}
  if (queue & (TCIFLUSH | TCIOFLUSH))	{
	  /* Input flushing by polling until nothing turns up
	     (we stop after 1000 chars anyway) */
	  COMMTIMEOUTS old;
	  COMMTIMEOUTS tmp;
	  char b;
	  DWORD more=1;
	  int max=1000;

	  if(!PurgeComm(fdHandle,PURGE_RXABORT | PURGE_RXCLEAR)) return(-1);
	  if(!GetCommTimeouts(fdHandle,&old)) return(-1);;
	  memset(&tmp,0,sizeof(tmp));
	  tmp.ReadTotalTimeoutConstant=100;
	  if(!SetCommTimeouts(fdHandle,&tmp)) return(-1);
	  while(max > 0 && more) {
			if(!ReadFile(fdHandle,&b,1,&more,0)) {
			  SetCommTimeouts(fdHandle,&old);
				return(-1);
			}
	    max--;
	  }
	  SetCommTimeouts(fdHandle,&old);
	}

  return(0);
}
/*******************************************************************/
int tcsetattr (HANDLE fdHandle, int actions, const struct termios *t)
{
  int newrate;
  int newsize;

  COMMTIMEOUTS to;
  DCB state;

  switch(t->c_ospeed) {
    case B110:
      newrate=CBR_110;
      break;
    case B300:
      newrate=CBR_300;
      break;
    case B600:
      newrate=CBR_600;
      break;
    case B1200:
      newrate=CBR_1200;
      break;
    case B2400:
      newrate=CBR_2400;
      break;
    case B4800:
      newrate=CBR_4800;
      break;
    case B9600:
      newrate=CBR_9600;
      break;
    case B19200:
      newrate=CBR_19200;
      break;
    case B38400:
      newrate=CBR_38400;
      break;
    case B57600:
      newrate=CBR_57600;
      break;
    case B115200:
      newrate=CBR_115200;
      break;
    default:
      errno=EINVAL;
      return(-1);
    }

  switch(t->c_cflag & CSIZE) {
    case CS5:
      newsize=5;
      break;
    case CS6:
      newsize=6;
      break;
    case CS7:
      newsize=7;
      break;
    case CS8:
      newsize=8;
      break;
    }

//  fdHandle = get_std_handle(fd);
  
  if(!GetCommState(fdHandle,&state)) return(-1);
  state.BaudRate=newrate;
  state.ByteSize=newsize;
  state.fBinary=1;
  state.fParity=0;
  state.fOutxCtsFlow=0; /*!!*/
  state.fOutxDsrFlow=0; /*!!*/
  state.fDsrSensitivity=0; /*!!*/

  if (t->c_cflag & PARENB) {
    state.Parity=(t->c_cflag & PARODD) ? ODDPARITY:EVENPARITY;
  } else {
    state.Parity=NOPARITY;
  }

  if(!SetCommState(fdHandle,&state)) return(-1);

#if 0
  h->r_binary = (t->c_iflag & IGNCR) ? 0 : 1;
  h->w_binary = (t->c_oflag & ONLCR) ? 0 : 1;

  h->vtime = t->c_cc[VTIME];
  h->vmin = t->c_cc[VMIN];
#endif

  memset(&to,0,sizeof(to));

#if 0
  to.ReadTotalTimeoutConstant = h->vtime * 100;
#endif

  if(!SetCommTimeouts(fdHandle,&to)) return(-1);
  return(0);
}
/*******************************************************************/
int tcgetattr(HANDLE fdHandle, struct termios *t)
{
	DCB state;
  int thisspeed;
  int thissize;

  if(!GetCommState(fdHandle,&state)) return(-1);
  switch(state.BaudRate) {
		case CBR_110:
			thisspeed=B110;
			break;
		case CBR_300:
			thisspeed=B300;
			break;
		case CBR_600:
			thisspeed=B600;
			break;
		case CBR_1200:
			thisspeed=B1200;
			break;
		case CBR_2400:
			thisspeed=B2400;
			break;
		case CBR_4800:
			thisspeed=B4800;
			break;
		case CBR_9600:
			thisspeed=B9600;
			break;
		case CBR_19200:
			thisspeed=B19200;
			break;
		case CBR_38400:
			thisspeed=B38400;
			break;
		case CBR_57600:
			thisspeed=B57600;
			break;
		case CBR_115200:
			thisspeed=B115200;
			break;
		default:
			errno=EINVAL;
			return(-1);
	}

  switch(state.ByteSize) {
		case 5:
			thissize = CS5;
			break;
		case 6:
			thissize = CS6;
			break;
		case 7:
			thissize = CS7;
			break;
		case 8:
		default:
			thissize = CS8;
			break;
	}

  memset(t,0,sizeof(*t));
  t->c_ospeed=t->c_ispeed=thisspeed;
  t->c_cflag|=thissize;

#if 0
      if (!h->r_binary)
	t->c_iflag |= IGNCR;
      if (!h->w_binary)
	t->c_oflag |= ONLCR;

      t->c_cc[VTIME] = h->vtime ;
      t->c_cc[VMIN] = h->vmin;
#endif

  return(0);
}
/*******************************************************************/
int nca_x_spopen(NCASPCS *spc)
{
	SECURITY_DESCRIPTOR sd={0};
	SECURITY_ATTRIBUTES sa={0};

	if(spc == NULL) return(-1);
//  sprintf(str, "\\\\.\\%s", port);

	if(!InitializeSecurityDescriptor(&sd,SECURITY_DESCRIPTOR_REVISION)) return(-1);
// set NULL DACL on the SD
	if(!SetSecurityDescriptorDacl(&sd,TRUE,(PACL)NULL,FALSE)) return(-1);
// now set up the security attributes
	sa.nLength=sizeof(SECURITY_ATTRIBUTES);
	sa.bInheritHandle=TRUE; 
	sa.lpSecurityDescriptor=&sd;

	spc->syserr=0;
  if((spc->fdx=CreateFile(spc->device, 
									GENERIC_READ | GENERIC_WRITE,
                  0,                    // exclusive access
//                  NULL,                 // no security attrs
									&sa,
                  OPEN_EXISTING,
                  FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
                  NULL)) == INVALID_HANDLE_VALUE) {
//        log_event(ups,LOG_ERR,"Cannot open serial port [%s]: 0x%08x\n",ups->device,GetLastError());
				spc->syserr=nca_x_getsyserr();
				return(-1);
    }

		spc->overlapped.hEvent=CreateEvent(NULL,  TRUE /* manual reset */,
										FALSE, /* initially non-signalled */
										NULL);  
		if(spc->overlapped.hEvent == NULL)	{
			spc->syserr=nca_x_getsyserr();
			CloseHandle(spc->fdx);
//      log_event(ups,LOG_ERR,"Cannot create event on %s: 0x%08x\n",ups->device,GetLastError());
			return(-2);
		}

    tcgetattr(spc->fdx,&spc->spci.oldtio); /* Save old settings */

    spc->spci.newtio.c_cflag=spc->default_speed | CS8 | CLOCAL | CREAD;
    spc->spci.newtio.c_iflag=IGNPAR;	      /* Ignore errors, raw input */
    spc->spci.newtio.c_oflag=0;	  /* Raw output */
    spc->spci.newtio.c_lflag=0;	  /* No local echo */

 /* w.p. This makes a non.blocking read() with TIMER_READ (10) sec. timeout */ 
    spc->spci.newtio.c_cc[VMIN]=0;
    spc->spci.newtio.c_cc[VTIME]=spc->timer_read*10;

#if defined(HAVE_CYGWIN) || defined(HAVE_OSF1_OS) || defined(_MSC_VER) || \
    defined(HAVE_LINUX_OS) || defined(HAVE_DARWIN_OS)
    cfsetospeed(&spc->spci.newtio,spc->default_speed);
    cfsetispeed(&spc->spci.newtio,spc->default_speed);
#endif /* do it the POSIX way */

    tcflush(spc->fdx,TCIFLUSH);
    tcsetattr(spc->fdx,TCSANOW,&spc->spci.newtio);
    tcflush(spc->fdx,TCIFLUSH);

		return(0);
}
/*******************************************************************/
int nca_x_spclose(NCASPCS *spc)
{
	if(spc == NULL) return(-1);
	if(spc->overlapped.hEvent != NULL) {
		CloseHandle(spc->overlapped.hEvent);
		spc->overlapped.hEvent=NULL;
	}
	if(spc->fdx != INVALID_HANDLE_VALUE) {
		CloseHandle(spc->fdx);
		spc->fdx=INVALID_HANDLE_VALUE;
	}
	return(0);
}
/*******************************************************************/
int nca_x_spwrite(NCASPCS *spc, unsigned char *data, unsigned long dlen)
{
	DWORD dwWritten;
	DWORD dwError,ret;
	COMSTAT cs={0};

	if(spc == NULL) return(-1);
	spc->syserr=0;
	if(spc->fdx == INVALID_HANDLE_VALUE) return(-2);

/*
	if(WriteFile(fd,buffer,count,&dwWritten,NULL) == 0) return(-1);
	return((int)dwWritten);
*/

//	ClearCommError(spc->fdx,&dwError,NULL);
	ClearCommError(spc->fdx,&dwError,&cs);
	if(dwError) {
//		log_event(ups,LOG_ERR,"ClearCommError error in serport_write: %lu.\n",dwError);
	}
	
	if(!WriteFile(spc->fdx,data,dlen,&dwWritten,&spc->overlapped)) {
		if((dwError=GetLastError()) == ERROR_IO_PENDING) {
			// We've been deferred, wait for it.
			if((ret=WaitForSingleObject(spc->overlapped.hEvent,10000)) != WAIT_OBJECT_0) {
				// We failed. Cancel it.
				PurgeComm(spc->fdx,PURGE_TXABORT);
				//CancelIo(hPort);
//				if((ret == WAIT_TIMEOUT) && is_ups_set(UPS_COMMLOST)) {
				if(ret == WAIT_TIMEOUT) {
				} else {
//					log_event(ups,LOG_ERR,"WaitForSingleObject on write failed: %lu/%lu.\n",GetLastError(),ret);
				}
				return(-3);
			}
			if(!GetOverlappedResult(spc->fdx,&spc->overlapped,&dwWritten,FALSE))	{
//				log_event(ups,LOG_ERR,"GetOverlappedResult on write failed: %lu.\n",GetLastError());
				return(-4);
			}
			if(!dwWritten) {
//				log_event(ups,LOG_ERR,"Wrote 0 bytes.\n");
				return(-5);
			}	else {
				//fprintf(stderr, "Write succeeded after time %c (%d).\n", ch, ch);
				FlushFileBuffers(spc->fdx);
				return((int)dwWritten);
			}
		}	else {
//			log_event(ups,LOG_ERR,"Write failed: %lu.\n",dwError);
			return(-6);
		}
	} else {
		if(dwWritten == 0) {
//			log_event(ups,LOG_WARNING,"Strange, we got a zero byte write.\n");
			return(-7);
		}
	}

	FlushFileBuffers(spc->fdx);

	return((int)dwWritten);
}
/*******************************************************************/
int nca_x_spread(NCASPCS *spc, char *data, unsigned long dlen, long sec)
{
	DWORD dwRead;
	DWORD dwError,ret;
	COMSTAT cs={0};
//	int wait=0;

	if(spc == NULL) return(-1);
	spc->syserr=0;
	if(spc->fdx == INVALID_HANDLE_VALUE) return(-2);

/*
	if(ReadFile(fd,buffer,count,&dwRead,NULL) == 0) return(-1);
	return((int)dwRead);
*/
	
	ClearCommError(spc->fdx,&dwError,&cs);
	if(dwError) {
//		log_event(ups,LOG_ERR,"ClearCommError error in serport_read: %lu.\n",dwError);
	}

	if(!ReadFile(spc->fdx,data,dlen,&dwRead,&spc->overlapped)) {
		if((dwError=GetLastError()) == ERROR_IO_PENDING) {
			// We've been deferred, wait for it.
			switch((ret=WaitForSingleObject(spc->overlapped.hEvent,sec*1000)))	{
				case WAIT_OBJECT_0:
					// We succeeded 
					break;
				case WAIT_TIMEOUT:
					// We timed out
					//fprintf(stderr, "Read timed out\n");
					PurgeComm(spc->fdx,PURGE_RXABORT);
					//CancelIo(hPort);
					errno=EAGAIN;
					return(-3);
				case WAIT_ABANDONED:
					// We were abandoned, did the hEvent go away?
//					log_event(ups,LOG_ERR,"WaitForSingleObject abandoned.\n");
					PurgeComm(spc->fdx,PURGE_RXABORT);
					//CancelIo(hPort);
					return(-4);
				case WAIT_FAILED:
				default:
//					log_event(ups,LOG_ERR,"WaitForSingleObject on read failed: %lu/%lu.\n",GetLastError(),ret);
					PurgeComm(spc->fdx,PURGE_RXABORT);
					//CancelIo(hPort);
					return(-5);
			}
			if(!GetOverlappedResult(spc->fdx,&spc->overlapped,&dwRead,TRUE)) {
//				log_event(ups,LOG_ERR,"GetOverlappedResult failed: %lu.\n",GetLastError());
				return(-6);
			}
			if(!dwRead){
//				log_event(ups,LOG_ERR,"Read 0 bytes.\n");
				return(-7);
			}
		}	else {
//			log_event(ups,LOG_ERR,"Read failed: %lu.\n",dwError);
			return(-8);
		}
	}	else {
		if(dwRead == 0)	{
//			log_event(ups,LOG_ERR,"Strange, we got a zero byte read.\n");
			return(-9);
		}
	}
	return((int)dwRead);
}
/*******************************************************************/
int nca_x_spwaitonread(NCASPCS *spc, long sec)
{
	DWORD mask,dwError,r,ret;

	if(!SetCommMask(spc->fdx,EV_RXCHAR)) return(-1);
	if(!WaitCommEvent(spc->fdx,&mask,&spc->overlapped)) {
		if((dwError=GetLastError()) == ERROR_IO_PENDING) {
			switch((ret=WaitForSingleObject(spc->overlapped.hEvent,sec*1000)))	{
				case WAIT_OBJECT_0:
					// We succeeded 
					break;
				case WAIT_TIMEOUT:
					return(0);
				default:
					return(-1);
			}
			if(!GetOverlappedResult(spc->fdx,&spc->overlapped,&r,FALSE)) {
				if((dwError=GetLastError()) == ERROR_IO_INCOMPLETE) {
					return(0);
				} else {
					return(-1);
				}
			}
		} else {
			return(-1);
		}
	}
	if(mask&EV_RXCHAR) return(1);
	return(0);
}
/*******************************************************************/
int nca_x_spsetRTS(NCASPCS *spc)
{
	spc->syserr=0;
	if(!EscapeCommFunction(spc->fdx,SETRTS)) {
		spc->syserr=nca_x_getsyserr();
		return(-1);
	}
	return(0);
}
/*******************************************************************/
int nca_x_spclrRTS(NCASPCS *spc)
{
	spc->syserr=0;
	if(!EscapeCommFunction(spc->fdx,CLRRTS)) {
		spc->syserr=nca_x_getsyserr();
		return(-1);
	}
	return(0);
}
/*******************************************************************/
int nca_x_spgetCTS(NCASPCS *spc, int *state)
{
	DWORD ms;

//  if(!GetCommState(spc->fdx,&dcb)) return(-1);
	spc->syserr=0;
  if(!GetCommModemStatus(spc->fdx,&ms)) {
		spc->syserr=nca_x_getsyserr();
		return(-1);
	}
	if(ms&MS_CTS_ON) *state=1;
	else *state=0;
	return(0);
}
/*******************************************************************/
int nca_x_spisreopenerror(NCASPCS *spc)
{
	if(spc == NULL) return(-1);
	if(spc->syserr == ERROR_FILE_NOT_FOUND) return(1);
	if(spc->syserr == ERROR_ACCESS_DENIED) return(1);
	return(0);
}
/*******************************************************************/
int nca_x_rename(char *oldpath, char *newpath)
{
	if(MoveFileEx(oldpath,newpath,MOVEFILE_REPLACE_EXISTING) != 0) return(0);
	return(-1);
}
/*******************************************************************/
int nca_x_lockfile(FILE *fp)
{
	int fno;
	const DWORD len=0xffffffff;
	OVERLAPPED offset;
	DWORD flags; 
	HANDLE fh;

	fno=_fileno(fp);
	if((fh=(HANDLE)_get_osfhandle(fno)) < 0) return(-1);
	flags=LOCKFILE_EXCLUSIVE_LOCK;
	memset(&offset,0,sizeof(offset));
	if(!LockFileEx(fh,flags,0,len,len,&offset)) return(-1);
	return(0);
}
/*******************************************************************/
int nca_x_unlockfile(FILE *fp)
{
	int fno;
	const DWORD len=0xffffffff;
	OVERLAPPED offset;
	HANDLE fh;

	fno=_fileno(fp);
	if((fh=(HANDLE)_get_osfhandle(fno)) < 0) return(-1);
	memset(&offset,0,sizeof(offset));
	if(!UnlockFileEx(fh,0,len,len,&offset)) return(-1);
	return(0);
}
/*******************************************************************/
int nca_x_logsyserr(long sec, FILE *fp)
{
	int rval=-1;
	LPVOID lpMsgBuf=NULL;
	char *p;

	if(fp == NULL) goto end;
	if(sec == 0) {
		rval=0;
		goto end;
	}
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
								NULL,
								(DWORD)sec,
								MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
								(LPTSTR)&lpMsgBuf,
								0,
								NULL);

	p=(char*)lpMsgBuf+strlen((char*)lpMsgBuf)-1;
	while(isspace(*p) && (p >= (char*)lpMsgBuf)) {
		*p='\0';
		p--;
	}

	fprintf(fp,"Os report: [%s].\n",(char*)lpMsgBuf);

	rval=1;

end:

	if(lpMsgBuf != NULL) LocalFree(lpMsgBuf);

	return(rval);
}
/*******************************************************************/
int nca_x_allocthreaddbgmh(void)
{
	return(0);
}
/*******************************************************************/
int nca_x_releasethreaddbgmh(void)
{
	unsigned long i;
	char *ptr;
	NCADMS *dms=NULL;

	for(i=0;i < g_dbgmh.num;i++) {
		ptr=*(g_dbgmh.ptr+i);
		dms=(NCADMS*)(ptr-sizeof(NCADMS));
		printf("Unreleased: [%s], %lu\n",ptr,dms->flen);
	}
	return(0);
}
/*******************************************************************/
int nca_x_addptrdbgmh(char *ptr)
{
	char **p;
	NCADMS *dms=NULL;

	if(ptr == NULL) return(0);
	dms=(NCADMS*)(ptr-sizeof(NCADMS));
	if((p=(char**)realloc((void*)g_dbgmh.ptr,(g_dbgmh.num+1)*sizeof(*g_dbgmh.ptr))) == NULL) return(-1);
	g_dbgmh.ptr=p;
	*(g_dbgmh.ptr+g_dbgmh.num)=ptr;
	g_dbgmh.num++;
	return(0);
}
/*******************************************************************/
int nca_x_delptrdbgmh(char *ptr)
{
	char **p;
	unsigned long i;

	if(ptr == NULL) return(0);
	for(i=0;i < g_dbgmh.num;i++) {
		if(*(g_dbgmh.ptr+i) == ptr) {
			if(i < g_dbgmh.num-1) {
				*(g_dbgmh.ptr+i)=*(g_dbgmh.ptr+g_dbgmh.num-1);
			}
			if(g_dbgmh.num > 1) {
				if((p=(char**)realloc((void*)g_dbgmh.ptr,(g_dbgmh.num-1)*sizeof(*g_dbgmh.ptr))) == NULL) return(-1);
				g_dbgmh.ptr=p;
				g_dbgmh.num--;
			} else {
				free((void*)g_dbgmh.ptr);
				g_dbgmh.ptr=NULL;
				g_dbgmh.num=0;
			}
			return(0);
		}
	}
	return(-1);
}
/*******************************************************************/
