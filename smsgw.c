#include "winx.h"

#include <curl/curl.h>

typedef struct tagGDT_MEM {
	char *buf;
	size_t len;
} GDT_MEM;

typedef struct PAYLOADDATA {
	char *msg;
	char *fname;
	FILE *fp;
	size_t rb;
} PAYLOADDATA;

NCASPCS g_spc={0};

/*******************************************************************/
int nca_spc_serportread(char *data, size_t len, long sec)
{
	int rval=-1;
	size_t plen;

	if(data == NULL) return(-1);
	if(g_spc.ized != 1) return(-1);

	plen=0;
	while(plen < len) {
		if(nca_x_spread(&g_spc,data+plen,1,sec) != 1) goto end;
		plen++;
	}

	rval=0;

end:

	return(rval);
}
/*******************************************************************/
int nca_spc_init(void)
{
	int rval=-1;

//	g_spc.default_speed=B9600;
//	g_spc.default_speed=B2400;
	g_spc.default_speed=B115200;
	g_spc.device="\\\\.\\COM13";
//	g_spc.device="COM1";
	g_spc.timer_read=10;
	g_spc.fdx=INVALID_HANDLE_VALUE;
	if(nca_x_spopen(&g_spc) != 0) {
		printf("err: %d, [%s] %d\n",g_spc.syserr,"spopen",g_spc.syserr);
		goto end;
	}

	g_spc.ized=1;
//	nca_spc_serportread_abort();

	rval=0;

end:

	return(rval);
}
/*******************************************************************/
int nca_spc_destroy(void)
{
	int rval=-1;

	nca_x_spclose(&g_spc);
	g_spc.ized=0;

	return(0);
}
/*******************************************************************/
int nca_spc_serportwrite(char *data, size_t len)
{
	int rval=-1,nxx;
	size_t plen,le;
	size_t len1;

	if(data == NULL) return(-1);
	if(g_spc.ized != 1) return(-1);

	len1=16000;
//	len1=2;
	plen=0;
	while(len > 0) {
		if(len < len1) {
			len1=len;
		}
		if((nxx=nca_x_spwrite(&g_spc,(unsigned char*)data+plen,(unsigned long)len1)) != (int)len1) {
			le=GetLastError();
			goto end;
		}
		len-=len1;
		plen+=len1;
	}

	rval=0;

end:

	return(rval);
}
/*******************************************************************/
int fona_read(char *msg, size_t msz, size_t *len)
{
	int rval=-1,ret;
	char c;

	for(;;) {
		ret=nca_spc_serportread(&c,1,1);
		if(ret == 0) {
			if((c != '\r') && (c != '\n')) {
//			printf("%c %02x\n",c,(unsigned char)c);
//				printf("%c",c);
				*(msg+*len)=c;
				(*len)++;
				if((*len+1) >= msz) break;
			} else if(c == '\n') {
				break;
			}
		} else {
			rval=0;
			goto end;
		}
	}

	if(*len < msz) {
		*(msg+*len)='\0';
	}

	rval=1;

end:

	return(rval);
}
/*******************************************************************/
static size_t receiver_f(void *ptr, size_t size, size_t nmemb, void *ud)
{
	GDT_MEM *rd=(GDT_MEM*)ud;
	size_t new_len;
	void *buf;

	if(rd == NULL) return(0);

	new_len=rd->len+size*nmemb;
//	if(nym_dm_reinit(&rd->buf,NULL,new_len+1) == NULL) return(0);
//	if(nym_dm_update(&rd->buf,NULL,new_len+1) == NULL) return(0);
	if((buf=realloc((void*)rd->buf,new_len+1)) == NULL) return(0);
	rd->buf=(char*)buf;
	memcpy((void*)(rd->buf+rd->len),ptr,size*nmemb);
  rd->len=new_len;
	*(rd->buf+new_len)='\0';

  return size*nmemb;
}
/*******************************************************************/
static size_t payload_func(void *ptr, size_t size, size_t nmemb, void *userp)
{
  PAYLOADDATA *pd=(PAYLOADDATA*)userp;
	size_t retcode;

  if((size == 0) || (nmemb == 0) || ((size*nmemb) < 1)) {
    return 0;
  }

	retcode=fread(ptr,size,nmemb,pd->fp);

	if(retcode == 0) {
		if(pd->msg != NULL) {
			memcpy((void*)((char*)ptr+retcode),(void*)pd->msg,strlen(pd->msg));
			retcode+=strlen(pd->msg);
			pd->msg=NULL;
		}
	}

  pd->rb+=retcode;

  return(retcode);
}
/*******************************************************************/
int fona_sendmail(char *msg)
{
	int rval;
  CURL *curl=NULL;
	GDT_MEM rd={0};
	char errorbuffer[CURL_ERROR_SIZE]={0};
  char *mail_from,*mail_to,*userpwd,*url;
  struct curl_slist *recipients=NULL;
	CURLcode res;
	PAYLOADDATA pd={0};
  struct stat file_info;

	url="smtps://smtp.gmail.com:465";
	userpwd="mx28.me:PASS4me642";
	mail_from="<mx28.me@gmail.com>";
	mail_to="<cicipo@gmail.com>";
	pd.fname="mail.txt";

  stat(pd.fname,&file_info);
	if((pd.fp=fopen(pd.fname,FOM_RO_BIN)) == NULL) goto end;
	pd.msg=msg;

  if((curl=curl_easy_init()) == NULL) goto end;

  curl_easy_setopt(curl,CURLOPT_WRITEFUNCTION,receiver_f);
	curl_easy_setopt(curl,CURLOPT_WRITEDATA,(void*)&rd);
  curl_easy_setopt(curl,CURLOPT_ERRORBUFFER,errorbuffer);
	curl_easy_setopt(curl,CURLOPT_SSL_VERIFYPEER,0L);
	curl_easy_setopt(curl,CURLOPT_SSL_VERIFYHOST,0L);

  curl_easy_setopt(curl,CURLOPT_USE_SSL,(long)CURLUSESSL_ALL);

  curl_easy_setopt(curl,CURLOPT_MAIL_FROM,mail_from);
  recipients=curl_slist_append(recipients,mail_to);
//  mail_rcpt=curl_slist_append(mail_rcpt, CC_ADDR);
  curl_easy_setopt(curl,CURLOPT_MAIL_RCPT,recipients);

    /* tell it to "upload" to the URL */
    curl_easy_setopt(curl,CURLOPT_UPLOAD, 1L);
    /* set where to read from (on Windows you need to use READFUNCTION too) */
    curl_easy_setopt(curl,CURLOPT_READFUNCTION,payload_func);
		curl_easy_setopt(curl,CURLOPT_READDATA,&pd);

    /* and give the size of the upload (optional) */
//    curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE,(curl_off_t)file_info.st_size);

    curl_easy_setopt(curl, CURLOPT_USERPWD, userpwd);

//  curl_easy_setopt(curl,CURLOPT_POSTFIELDS,*buf);
//  curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)len);

	rd.len=0;
	curl_easy_setopt(curl,CURLOPT_URL,url);
	res = curl_easy_perform(curl);
	if(res != CURLE_OK) {
		fprintf(stderr, "curl_easy_perform() failed: %s\n",curl_easy_strerror(res));
		goto end;
	}

	rval=0;

end:

  if(recipients != NULL) curl_slist_free_all(recipients);
	if(curl != NULL) curl_easy_cleanup(curl);

	return(rval);
}
/*******************************************************************/
int p4(void)
{
	unsigned long n,i;

	n=16000000;
	for(i=400;i >= 200;i--) {
		if(n%i == 0) {
			printf("%lu\n",i);
		}
	}

	return(0);
}
/*******************************************************************/
int main(int argc, char *argv[])
{
	int rval=-1,ret,setpin=0,cpinok=0,pinready=0,callready=0,smsready=0;
	char *at;
	char msg[1024],sms[1024],mm[1024];
	size_t len=0;
	int smsi;

/*
	p4();
	exit(0);
*/

  curl_global_init(CURL_GLOBAL_ALL);

//	fona_sendmail("massage");


//#if 0
	if(nca_spc_init() != 0) goto end;

	at="AT\n";
	ret=nca_spc_serportwrite(at,strlen(at));
	
	for(;;) {
		len=0;
		if((ret=fona_read(msg,sizeof(msg),&len)) == 1) {
			printf("%s\n",msg);
			if(strcmp(msg,"OK") == 0) break;
		}
	}

	at="ATE0\n";
	ret=nca_spc_serportwrite(at,strlen(at));
	for(;;) {
		len=0;
		if((ret=fona_read(msg,sizeof(msg),&len)) == 1) {
			printf("%s\n",msg);
			if(strcmp(msg,"OK") == 0) break;
		}
	}

	at="AT+COPS?\n";
	ret=nca_spc_serportwrite(at,strlen(at));
	for(;;) {
		len=0;
		if((ret=fona_read(msg,sizeof(msg),&len)) == 1) {
			printf("%s\n",msg);
			if(strcmp(msg,"OK") == 0) break;
		}
	}

	at="AT+CBC\n";
	ret=nca_spc_serportwrite(at,strlen(at));
	for(;;) {
		len=0;
		if((ret=fona_read(msg,sizeof(msg),&len)) == 1) {
			printf("%s\n",msg);
			if(strcmp(msg,"OK") == 0) break;
		}
	}

	at="AT+CPIN?\n";
	ret=nca_spc_serportwrite(at,strlen(at));
	for(;;) {
		len=0;
		ret=fona_read(msg,sizeof(msg),&len);
		printf("%s\n",msg);
		if(strcmp(msg,"OK") == 0) break;
		if(strcmp(msg,"+CPIN: SIM PIN") == 0) setpin=1;
	}

	if(setpin == 1) {
		at="AT+CPIN=3001\n";
		ret=nca_spc_serportwrite(at,strlen(at));
		for(;;) {
			len=0;
			if((ret=fona_read(msg,sizeof(msg),&len)) == 1) {
				printf("%s\n",msg);
				if(strcmp(msg,"OK") == 0) cpinok=1;
				else if(strcmp(msg,"+CPIN: READY") == 0) pinready=1;
				else if(strcmp(msg,"Call Ready") == 0) callready=1;
				else if(strcmp(msg,"SMS Ready") == 0) smsready=1;
				if((cpinok == 1) && (pinready == 1) && (callready == 1) && (smsready == 1)) break;
			}
		}
	}

	at="AT+COPS?\n";
	ret=nca_spc_serportwrite(at,strlen(at));
	for(;;) {
		len=0;
		ret=fona_read(msg,sizeof(msg),&len);
		printf("%s\n",msg);
		if(strcmp(msg,"OK") == 0) break;
	}

	at="AT+CMGF=1\n";
	ret=nca_spc_serportwrite(at,strlen(at));
	for(;;) {
		len=0;
		ret=fona_read(msg,sizeof(msg),&len);
		printf("%s\n",msg);
		if(strcmp(msg,"OK") == 0) break;
	}

	at="AT+CMGL=\"ALL\"\n";
	ret=nca_spc_serportwrite(at,strlen(at));
	for(;;) {
		len=0;
		ret=fona_read(msg,sizeof(msg),&len);
		printf("%s\n",msg);
		if(strcmp(msg,"OK") == 0) break;
	}
//#endif

	at="AT+CMGDA=\"DEL ALL\"\n";
	ret=nca_spc_serportwrite(at,strlen(at));
	for(;;) {
		len=0;
		ret=fona_read(msg,sizeof(msg),&len);
		printf("%s\n",msg);
		if(strcmp(msg,"OK") == 0) break;
	}

	for(;;) {
		smsi=0;
		len=0;
		ret=fona_read(msg,sizeof(msg),&len);
		if(ret == 1) {
			at="+CMTI: \"SM\",";
			if(strncmp(msg,at,strlen(at)) == 0) {
				smsi=atoi(msg+strlen(at));
				sprintf(sms,"AT+CMGR=%d\n",smsi);

				mm[0]='\0';
				at=sms;
				ret=nca_spc_serportwrite(at,strlen(at));
				for(;;) {
					len=0;
					ret=fona_read(msg,sizeof(msg),&len);
					if(strcmp(msg,"OK") == 0) break;
					strcat(mm,msg);
					printf("%s\n",msg);
				}
				fona_sendmail(mm);
			}
		}

	}

	rval=0;

end:

	nca_spc_destroy();

  curl_global_cleanup();

	return(rval);
}
/*******************************************************************/
//1777 *102# USSD
//AT+CUSD=1,"*100#"
//AT+CUSD=1,"*102#"