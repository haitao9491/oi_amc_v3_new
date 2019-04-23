/*
 *
 * apfrm.c - Common framework of applications
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include "os.h"
#ifndef WIN32
#include <unistd.h>
#else
#include <process.h>
#include <Winbase.h>
#endif
#include <string.h>
#include <signal.h>
#include <sys/types.h>

#include "apfrm.h"
#include "aplog.h"
#include "misc.h"
#include "apbldinfo.h"

DECLARE_LOGHANDLE;

static int aprunning = 1;
static int received_sig = -1;

struct ap_framework *mainapp = NULL;
extern struct ap_framework *register_ap(void);

static char *progname = NULL;
static char  progbasename[32];

static const unsigned int logfilenamelen = 512;
static char  logfile[512];
static int   loglevel = LGWRLEVELERROR;
static int   logsize = 1024;

static int  foreground = 0;
static int  nopidfile = 0;
static long instance = 0;

static void make_daemon(void)
{
#ifndef OS_WINDOWS
	int rc;

	rc = daemon(1, 0);
	if (rc == -1) {
		printf("Failed to make this process a daemon.\n");
	}
#endif
}

static void version(void)
{
	printf("Powered by Application Framework\n");
#if defined(AP_VERSION)
	printf("Powered by AP Platform %s\n", AP_VERSION);
#else
	printf("Powered by AP Platform V0\n");
#endif
}

static void usage(char *prgname)
{
	version();
	printf("\n");
	printf("Usage: %s [standard options] [application specific options]\n",
			prgname);
	printf("    standard options:\n");
	printf("        --version|-V    : to show the version\n");
	printf("        --help|-H       : to show this screen\n");
	printf("        --bldinfo|-B    : to show build information\n");
	printf("        --foreground|-F : don't run in daemon mode\n");
	printf("        --nopidfile|-N  : don't create pid file\n");
	printf("        --instance|-I <instance number> : \n");
	printf("        --logfile <filename> : to dump log to this file\n");
	printf("        --loglevel <emergency|alert|critical|error|warn|notice|info|debug>\n");
	printf("        --logsize <size in mega-byte>\n");
	printf("\n");
	printf("    application specific options:\n");

	if (mainapp->usage)
		mainapp->usage(prgname);
	else
		printf("        Not available\n");

	printf("\n");
}

static int parse_standard_args(int argc, char **argv)
{
	int i;

	if ((strlen(progname) + 25) >= logfilenamelen) {
		fprintf(stderr, "Program name [%s] too long to generate logfile name\n",
				progname);
		return -1;
	}
	memset(logfile, 0, logfilenamelen);

	for (i = 1; i < argc; i++) {
		if ((strcmp(argv[i], "--version") == 0) ||
		    (strcmp(argv[i], "-V") == 0)) {
			version();
			if (mainapp->version)
				mainapp->version(progbasename);
			return 0;
		}
		else if ((strcmp(argv[i], "--help") == 0) ||
		         (strcmp(argv[i], "-H") == 0)) {
			usage(progbasename);
			return 0;
		}
		else if ((strcmp(argv[i], "--bldinfo") == 0) ||
				(strcmp(argv[i], "-B") == 0)) {
			printf("%s\n", ApGetBldInfo());
			return 0;
		}
		else if ((strcmp(argv[i], "--foreground") == 0) ||
				(strcmp(argv[i], "-F") == 0)) {
			foreground = 1;
		}
		else if ((strcmp(argv[i], "--nopidfile") == 0) ||
				(strcmp(argv[i], "-F") == 0)) {
			nopidfile = 1;
		}
		else if ((strcmp(argv[i], "--instance") == 0) ||
		         (strcmp(argv[i], "-I") == 0)) {
			char *ptr;
			i++;
			if (i >= argc)
				return -1;
			instance = strtol(argv[i], &ptr, 0);
			if (*ptr || (instance < 0))
				return -1;
		}
		else if (strcmp(argv[i], "--logfile") == 0) {
			if (((i + 1) >= argc) || (argv[i + 1][0] == '-'))
				return -1;

			i++;
			if (strlen(argv[i]) >= logfilenamelen) {
				fprintf(stderr, "Logfile name [%s]: too long\n",
						argv[i]);
				return -1;
			}
			memset(logfile, 0, logfilenamelen);
			strcpy(logfile, argv[i]);
		}
		else if (strcmp(argv[i], "--loglevel") == 0) {
			i++;
			if (i >= argc)
				return -1;

			if (strcmp(argv[i], "emergency") == 0)
				loglevel = LGWRLEVELEMERGENCY;
			else if (strcmp(argv[i], "alert") == 0)
				loglevel = LGWRLEVELALERT;
			else if (strcmp(argv[i], "critical") == 0)
				loglevel = LGWRLEVELCRITICAL;
			else if (strcmp(argv[i], "error") == 0)
				loglevel = LGWRLEVELERROR;
			else if (strcmp(argv[i], "warn") == 0)
				loglevel = LGWRLEVELWARN;
			else if (strcmp(argv[i], "notice") == 0)
				loglevel = LGWRLEVELNOTICE;
			else if (strcmp(argv[i], "info") == 0)
				loglevel = LGWRLEVELINFO;
			else if (strcmp(argv[i], "debug") == 0)
				loglevel = LGWRLEVELDEBUG;
			else
				return -1;
		}
		else if (strcmp(argv[i], "--logsize") == 0) {
			char *ptr;
			i++;
			if (i >= argc)
				return -1;

			logsize = strtol(argv[i], &ptr, 0);
			if (!*ptr) {
				logsize *= 1024;
			}
			else {
				if (!strcmp(ptr, "k") || !strcmp(ptr, "K")) {
				}
				else if (!strcmp(ptr, "m") || !strcmp(ptr, "M")) {
					logsize *= 1024;
				}
				else if (!strcmp(ptr, "g") || !strcmp(ptr, "G")) {
					logsize *= 1048576;
				}
				else {
					return -1;
				}
			}
			if ((logsize < 1) || (logsize >= 2097152))
				return -1;
		}
		else
			break;
	}
 
	if (logfile[0] == 0) {
		if (instance == 0) {
			sprintf(logfile, "%s.log", progname);
		}
		else {
			sprintf(logfile, "%s.%ld.log", progname, instance);
		}
	}

	return i;
}

static int parse_args(int argc, char **argv)
{
	int ret;
	int apargstart;

	apargstart = parse_standard_args(argc, argv);
	if (apargstart <= 0)
		return apargstart;

	if ((apargstart < argc) && mainapp->parse_args) {
		ret = mainapp->parse_args(argc - apargstart, argv + apargstart);
		if (ret)
			return -1;

		return 1;
	}

	return (apargstart < argc) ? -1 : 1;
}

static int check_pidfile(FILE *fp)
{
	int   len;
	long  pid;
	char *ptr, pidstr[16];

	memset(pidstr, 0, sizeof(pidstr));
	if (fgets(pidstr, 16, fp) != pidstr) {
		LOGERROR("APFRM: Process %s[%ld]: unable to read from pidfile.",
				progbasename, instance);
		return 0;
	}

	len = (int)strlen(pidstr);
	while (len > 0) {
		if ((pidstr[len - 1] != 0x0d) && (pidstr[len - 1] != 0x0a))
			break;
		len--;
		pidstr[len] = 0;
	}
	if (len <= 0) {
		LOGERROR("APFRM: Process %s[%ld]: no pid string in pidfile?",
				progbasename, instance);
		return 0;
	}

	pid = strtol(pidstr, &ptr, 0);
	if (*ptr || (pid <= 0)) {
		LOGERROR("APFRM: Process %s[%ld]: invalid pid(%s) in pidfile.",
				progbasename, instance, pidstr);
		return 0;
	}
#ifndef WIN32
	if (kill(pid, 0) < 0) {
#else
	if (killprocessbypid(pid) < 0) {
#endif
		LOGERROR("APFRM: Process %s[%ld]: No process with pid %ld?",
				progbasename, instance, pid);
		return 0;
	}

	return (int)pid;

}

static int create_pidfile(char *pidfile, long instance)
{
	FILE *fp;
	char *fname;
	char *basename;
	int   len;

	basename = pidfile ? pidfile : progname;
	len = (int)strlen(basename);

	fname = (char *)malloc(len + 16);
	if (!fname) {
		LOGEMERGENCY("APFRM: Process %s[%ld]: Insufficient memory.",
				progbasename, instance);
		return -1;
	}

	memset(fname, 0, len + 16);
	sprintf(fname, "%s.pid.%ld", basename, instance);

	fp = fopen(fname, "r");
	if (fp) {
		int pid;

		if ((pid = check_pidfile(fp)) > 0) {
			LOGWARN("APFRM: Process %s[%ld] is running "
					"(pid %d : %s).",
					progbasename, instance, pid, fname);
			fclose(fp);
			free(fname);
			return -1;
		}
		fclose(fp);
	}

	if((fp = fopen(fname, "w")) == NULL) {
		LOGEMERGENCY("APFRM: Process %s[%ld]: Unable to open pidfile:"
				" %s.", progbasename, instance, fname);
		free(fname);
		return -1;
	}
#ifndef WIN32
	fprintf(fp, "%d\n", getpid());
#else
	fprintf(fp, "%d\n", _getpid());
#endif
	fclose(fp);

	LOG("APFRM: Process %s[%ld]: pid %d written to %s.",
			progbasename, instance, getpid(), fname);

	free(fname);

	return 0;

}

static void remove_pidfile(char *pidfile, long instance)
{

	char *fname;
	char *basename;
	int   len;

	basename = pidfile ? pidfile : progname;
	len = (int)strlen(basename);

	fname = (char *)malloc(len + 16);
	if (!fname)
		return;

	memset(fname, 0, len + 16);
	sprintf(fname, "%s.pid.%ld", basename, instance);
	unlink(fname);
	LOG("APFRM: Process %s[%ld]: pidfile %s removed.",
			progbasename, instance, fname);
	free(fname);
}

#ifdef WIN32
void ap_loop_logLevel(void)
{
	if (loglevel < (LGWRLEVELS - 1)) {
		loglevel++;
	}
	else{
		loglevel = 0;
	}
	SETLGWRLEVEL(loglevel);
	LOG("Adjust log level to [%d][%s].", loglevel, LOGLEVELTITLE(loglevel));
	printf("Adjust log level to [%d][%s].\n", loglevel, LOGLEVELTITLE(loglevel));
}

BOOL CtrlHandler( DWORD fdwCtrlType )
{
	received_sig = fdwCtrlType;
	switch( fdwCtrlType )
	{
	case CTRL_C_EVENT:
	case CTRL_LOGOFF_EVENT:
	case CTRL_SHUTDOWN_EVENT:
		LOG("APFRM: Terminated by user.");
		printf("APFRM: Terminated by user.");
		aprunning = 0;
		return TRUE;

	case CTRL_CLOSE_EVENT:
		return FALSE;

	case CTRL_BREAK_EVENT:
		Beep( 900, 200 );
		ap_loop_logLevel();
		return TRUE;

	default:
	  return FALSE;
	}
}

#endif

void signal_handler(int sig, siginfo_t *sip, void *uap)
{
#ifndef WIN32
	received_sig = sig;

	switch (sig) {
	case SIGALRM:
	case SIGUSR1:
	case SIGUSR2:
		break;

	case SIGINT:
		if (foreground) {
			aprunning = 0;
		}
		break;

	case SIGPIPE:
	case SIGCHLD:
		break;

	default:
		aprunning = 0;
		break;
	}
#endif
}

static void install_signal_handler(void)
{
#ifndef OS_WINDOWS
	int              sig;
	sigset_t         sigmask;
	struct sigaction sigact;

	sigemptyset(&sigmask);
	pthread_sigmask(SIG_SETMASK, &sigmask, NULL);

	sigact.sa_sigaction = signal_handler;
	sigact.sa_mask = sigmask;

	for (sig = 1; sig < NSIG; sig++) {
		if ((sig == SIGKILL) || (sig == SIGSTOP))
			continue;

		sigact.sa_flags = SA_RESTART;
		if ((sig == SIGSEGV) || (sig == SIGBUS) ||
		    (sig == SIGFPE) || (sig == SIGILL))
			sigact.sa_flags |= SA_RESETHAND;

		sigaction(sig, &sigact, NULL);
	}
#else
	SetConsoleCtrlHandler( (PHANDLER_ROUTINE) CtrlHandler, TRUE );
#endif
}

void main_cleanup(void)
{
	LGWRCLOSE();
}

void ap_set_foreground(void)
{
	foreground = 1;
}

void ap_inc_loglevel(void)
{
	if (loglevel < (LGWRLEVELS - 1)) {
		loglevel++;
		SETLGWRLEVEL(loglevel);
		LOG("Increase log level to %d, being more verbose.", loglevel);
	}
}

void ap_dec_loglevel(void)
{
	if (loglevel > 0) {
		loglevel--;
		SETLGWRLEVEL(loglevel);
		LOG("Decrease log level to %d, being less verbose.", loglevel);
	}
}


int ap_is_running(void)
{
	if (received_sig == -1)
		return aprunning;
#ifndef WIN32
	LOG("APFRM: Catched signal %d.", received_sig);
	switch (received_sig) {
	case SIGALRM:
		if (mainapp->sigalrm_handle)
			mainapp->sigalrm_handle();
		break;

	case SIGUSR1:
		if (mainapp->sigusr1_handle)
			mainapp->sigusr1_handle();
		else
			ap_inc_loglevel();
		break;

	case SIGUSR2:
		if (mainapp->sigusr2_handle)
			mainapp->sigusr2_handle();
		else
			ap_dec_loglevel();
		break;

	case SIGINT:
		if (foreground) {
			LOG("APFRM: Terminated by user.");
		}
		break;

	case SIGPIPE:
	case SIGCHLD:
		LOG("APFRM: Catched signal ignored.");
		break;

	default:
		LOG("APFRM: A fatal signal catched, terminating program.");
		break;
	}
#else
	LOG("APFRM: Catched signal %d.", received_sig);
#endif
	received_sig = -1;

	return aprunning;
}

void ap_stop_running(void)
{
	aprunning = 0;
}

static int parse_progname(char *prgname, char *basename, int size)
{
	char *p;
#ifndef WIN32
	if ((p = strrchr(prgname, '/')) == NULL)
#else
	if ((p = strrchr(prgname, '\\')) == NULL)
#endif
		p = prgname;
	else
		p++;

	if (strlen(p) >= (unsigned int)size)
		return -1;

	memset(basename, 0, size);
	strcpy(basename, p);

	return 0;
}

int main(int argc, char **argv)
{
	int ret;

	progname = argv[0];
	if (parse_progname(progname, progbasename, sizeof(progbasename)) < 0) {
		fprintf(stderr, "Failed to get basename.\n");
		return 1;
	}

	setlocale(LC_ALL, "");
#if !defined(WIN32) && !defined(__tile__)
	bindtextdomain(progbasename, "./locales");
	textdomain(progbasename);
#endif

	if ((mainapp = register_ap()) == NULL) {
		fprintf(stderr, "Failed to register application.\n");
		return 1;
	}

	ret = parse_args(argc, argv);
	if (ret <= 0) {
		if (ret)
			usage(progbasename);
		return ret;
	}

	if (!foreground)
		make_daemon();

	LGWROPEN(logfile, loglevel, logsize);

	if (!nopidfile) {
		if (create_pidfile(mainapp->pidfile, instance)) {
			main_cleanup();
			return 2;
		}
	}

	install_signal_handler();

	if (mainapp->run) {
		LOG("APFRM: Process %s[%ld]: ready to go ...",
				progbasename, instance);
		mainapp->run(instance, mainapp->data);
		LOG("APFRM: Process %s[%ld]: terminated.",
				progbasename, instance);
	}

	if (!nopidfile) {
		remove_pidfile(mainapp->pidfile, instance);
	}
	main_cleanup();

	return 0;
}
