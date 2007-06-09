/*
 * Error handling and logging functions
 *
 * $Id$
 */

/* Implementation details:
 *
 * All argument-list based functions call their va_list (v..) counterparts
 * to avoid doubled code. Furthermore, all functions use 'vlogmsg' to do the
 * actual writing of the logmessage and all error/warning functions use
 * 'vlogerror' to generate the error-logmessage from the input. vlogerror in
 * turn calls errorstring to generate the actual error message;
 */

#include "lib.error.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <time.h>

/* Variables defining logmessages verbosity to stderr/logfile */
int  verbose      = LOG_NOTICE;
int  loglevel     = LOG_DEBUG;
FILE *stdlog      = NULL;

/* Main function that contains logging code */
void vlogmsg(int msglevel, char *mesg, va_list ap)
{
    time_t currtime;
    char timestring[128];
	char *buffer;
	int mesglen = (mesg==NULL ? 0 : strlen(mesg));
	int bufferlen;

	/* Try to open logfile if it is defined */
#ifdef LOGFILE
	if ( stdlog==NULL ) stdlog = fopen(LOGFILE,"a");
#endif
	
	currtime  = time(NULL);
	strftime(timestring, sizeof(timestring), "%b %d %H:%M:%S", localtime(&currtime));

	bufferlen = strlen(timestring)+strlen(progname)+mesglen+20;
	buffer = (char *)malloc(bufferlen);
	if ( buffer==NULL ) abort();

	snprintf(buffer, bufferlen, "[%s] %s[%d]: %s\n",
	         timestring, progname, getpid(), mesg);
	
	if ( msglevel<=verbose  ) { vfprintf(stderr, buffer, ap); fflush(stderr); }
	if ( msglevel<=loglevel &&
	     stdlog!=NULL       ) { vfprintf(stdlog, buffer, ap); fflush(stdlog); }

	free(buffer);
}

/* Argument-list wrapper function around vlogmsg */
void logmsg(int msglevel, char *mesg, ...)
{
	va_list ap;
	va_start(ap, mesg);

	vlogmsg(msglevel, mesg, ap);

	va_end(ap);
}

/* Function to generate error/warning string */
char *errorstring(char *type, int errnum, char *mesg)
{
	int buffersize;
	char *buffer;
	char *endptr; /* pointer to current end of buffer */
#ifdef __GLIBC__
	char *tmpstr;
	int tmplen;
#endif

	if ( type==NULL ) {
		type = strdup(ERRSTR);
		if ( type==NULL ) abort();
	}

	/* 256 > maxlength strerror() */
	buffersize = strlen(type) + (mesg==NULL ? 0 : strlen(mesg)) + 256;

	endptr = buffer = (char *) malloc(buffersize);
	if ( buffer==NULL ) abort();

	sprintf(buffer,type);
	endptr = strchr(endptr,0);
	
	if ( mesg!=NULL ) {
		snprintf(endptr, buffersize-strlen(buffer), ": %s", mesg);
		endptr = strchr(endptr,0);
	}		
	if ( errnum!=0 ) {
		snprintf(endptr, buffersize-strlen(buffer), ": ");
		endptr = strchr(endptr,0);
#ifdef __GLIBC__
/* glibc strerror_r doesn't comply with POSIX standards. From man-page:
 *   
 *   char *strerror_r(int errnum, char *buf, size_t n);
 * 
 * is a GNU extension used by glibc (since 2.0), and must be regarded
 * as obsolete in view of SUSv3.  The GNU version may, but need not,
 * use the user-supplied buffer. If it does, the result may be
 * truncated in case the supplied buffer is too small. The result is
 * always NUL-terminated.
 */
		tmplen = buffersize-strlen(buffer);
		tmpstr = strerror_r(errnum, endptr, tmplen);
		strncat(endptr, tmpstr, tmplen);
#else
		strerror_r(errnum, endptr, buffersize-strlen(buffer));
#endif
		endptr = strchr(endptr,0);
	}
	if ( mesg==NULL && errnum==0 ) {
		sprintf(endptr,": unknown error");
		endptr = strchr(endptr,0);
	}

	return buffer;
}

/* Function to generate and write error logmessage (using vlogmsg) */
void vlogerror(int errnum, char *mesg, va_list ap)
{
	char *buffer;

	buffer = errorstring(ERRSTR, errnum, mesg);

	vlogmsg(LOG_ERR, buffer, ap);

	free(buffer);
}

/* Argument-list wrapper function around vlogerror */
void logerror(int errnum, char *mesg, ...)
{
	va_list ap;
	va_start(ap, mesg);

	vlogerror(errnum, mesg, ap);

	va_end(ap);
}

/* Logs an error message and exit with non-zero exitcode */
void verror(int errnum, char *mesg, va_list ap)
{
	vlogerror(errnum, mesg, ap);

	exit(exit_failure);
}

/* Argument-list wrapper function around verror */
void error(int errnum, char *mesg, ...)
{
	va_list ap;
	va_start(ap, mesg);

	verror(errnum, mesg, ap);
}

/* Logs a warning message */
void vwarning(int errnum, char *mesg, va_list ap)
{
	char *buffer;

	buffer = errorstring(WARNSTR, errnum, mesg);

	vlogmsg(LOG_WARNING, buffer, ap);

	free(buffer);
}

/* Argument-list wrapper function around vwarning */
void warning(int errnum, char *mesg, ...)
{
	va_list ap;
	va_start(ap, mesg);

	vwarning(errnum, mesg, ap);

	va_end(ap);
}
