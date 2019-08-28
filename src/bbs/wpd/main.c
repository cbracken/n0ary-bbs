#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <termios.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>

#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"
#include "wp.h"

struct active_processes *procs = NULL;

char output[4096];
int shutdown_daemon = FALSE;

time_t
	time_now = 0,
	Wpd_Refresh,
	Wpd_Age,
	Wpd_Update,
	Wpd_Flush;

int
	Wpd_Hour,
	Wpd_Update_Size,
	Wpd_Port,
	Wpd_Global_Type,
	Wpd_Local_Type;

char
	*Bbs_Call,
	*Bin_Dir,
	*Wpd_Bind_Addr = NULL,
	*Wpd_Update_Subject,
	*Wpd_Global_Server,
	*Wpd_Global_Type_Str,
	*Wpd_Local_Distrib,
	*Wpd_Local_Type_Str,
	*Wpd_User_File,
	*Wpd_Bbs_File,
	*Wpd_Dump_File;

struct ConfigurationList ConfigList[] = {
	{ "",					tCOMMENT,	NULL },
	{ "BBS_CALL",			tSTRING,	(int*)&Bbs_Call },
	{ "BBS_HOST",			tSTRING,	(int*)&Bbs_Host },
	{ "BBSD_PORT",			tINT,		(int*)&Bbsd_Port },
	{ "BIN_DIR", 			tDIRECTORY,	(int*)&Bin_Dir },
	{ "",					tCOMMENT,	NULL },
	{ "WPD_PORT",			tINT,		(int*)&Wpd_Port },
	{ "WPD_GLOBAL_SERVER",	tSTRING,	(int*)&Wpd_Global_Server },
	{ "WPD_GLOBAL_TYPE",	tSTRING,	(int*)&Wpd_Global_Type_Str },
	{ "WPD_BIND_ADDR",	tSTRING,	(int*)&Wpd_Bind_Addr },
	{ "WPD_LOCAL_DISTRIB",	tSTRING,	(int*)&Wpd_Local_Distrib },
	{ "WPD_LOCAL_TYPE",	tSTRING,	(int*)&Wpd_Local_Type_Str },
	{ "WPD_UPDATE_SUBJECT",	tSTRING,	(int*)&Wpd_Update_Subject },
	{ "WPD_USER_FILE",		tFILE,		(int*)&Wpd_User_File },
	{ "WPD_BBS_FILE",		tFILE,		(int*)&Wpd_Bbs_File },
	{ "WPD_DUMP_FILE",		tFILE,		(int*)&Wpd_Dump_File },
	{ "",					tCOMMENT,	NULL },
	{ "WPD_REFRESH",		tTIME,		(int*)&Wpd_Refresh},
	{ "WPD_AGE",			tTIME,		(int*)&Wpd_Age },
	{ "WPD_FLUSH",			tTIME,		(int*)&Wpd_Flush },
	{ "WPD_UPDATE",			tTIME,		(int*)&Wpd_Update },
	{ "WPD_UPDATE_SIZE",	tINT,		(int*)&Wpd_Update_Size },
	{ "WPD_HOUR",			tINT,		(int*)&Wpd_Hour },
	{ NULL, 0, NULL}};

static int get_message_type(const char *varname, const char *str, int def);
static void log_wpd(char *str);

int
service_port(struct active_processes *ap)
{
    char *c, *s, buf[256];

	if(socket_read_line(ap->fd, buf, 256, 10) == ERROR)
		return ERROR;
	log_f("wpd", "R:", buf);

	s = buf;
	NextChar(s);
	if(*s) {
		if((c = parse(ap, s)) == NULL)
			return ERROR;
	} else
		c = Error("call");

	log_f("wpd", "S:", c);
	if(socket_raw_write(ap->fd, c) == ERROR)
		return ERROR;

	return OK;
}

struct active_processes *
add_proc()
{
    struct active_processes
        *tap = procs,
        *ap = malloc_struct(active_processes);

    if(tap == NULL)
        procs = ap;
    else {
        while(tap->next)
            NEXT(tap);
        tap->next = ap;
    }

    ap->fd = ERROR;
    return ap;
}


struct active_processes *
remove_proc(struct active_processes *ap)
{
    struct active_processes *tap = procs;

    if((long)procs == (long)ap) {
        NEXT(procs);
		tap = procs;
    } else {
        while(tap) {
            if((long)tap->next == (long)ap) {
                tap->next = tap->next->next;
				NEXT(tap);
                break;
            }
            NEXT(tap);
        }
    }    


    socket_close(ap->fd);
    free(ap);
    return tap;
}

time_t
calc_wp_time(time_t current)
{
	struct tm *tm;

	if(current == 0) {
		time_t now = Time(NULL);
		tm = localtime(&now);

		if(tm->tm_hour > Wpd_Hour) {
			now += tDay;
			tm = localtime(&now);
		}

		tm->tm_hour = Wpd_Hour;
		tm->tm_min = 0;
		tm->tm_sec = 0;
#ifdef SUNOS
		return timelocal(tm);
#else
		return mktime(tm);
#endif
	}

	return current + Wpd_Update;
}

void
usage(char *pgm)
{
	printf("usage: %s [-a] [-l] [-d#] [-?|h] [-w] [-f config.file]\n", pgm);
	printf("          -?/H\tdisplay help\n");
	printf("          -a\trun as aging process\n");
	printf("          -l\tdaemon logging\n");
	printf("          -L\tdaemon logging, with clear\n");
	printf("          -w\twrite configuration to stdout\n");
	printf("          -f\tread configuration file\n");
	printf("          -d#\tdebugging options (hex)\n");
	printf("              1     verbose\n");
	printf("              100   leave daemon in foreground\n");
	printf("              200   ignore host (now default)\n");
	printf("              400   don't connect to other daemons\n");
	printf("              800   inhibit mail\n");
	printf("	     1000   test host (check hostname)\n");
	printf("\n");
}

int
main(int argc, char *argv[])
{
	int listen_port;
	int listen_sock;
	char *listen_addr;
	int bbsd_sock;
	struct timeval t;
	struct active_processes *ap;
	time_t flush_time;
	time_t wp_update_time;
	parse_options(argc, argv, ConfigList, "WPD - White Pages Daemon");

	if(dbug_level & dbgTESTHOST)
		test_host(Bbs_Host);
	if(!(dbug_level & dbgFOREGROUND))
		daemon(0, 0);

	if(bbsd_open(Bbs_Host, Bbsd_Port, "wpd", "DAEMON") == ERROR)
		error_print_exit(0);
	error_clear();
	bbsd_get_configuration(ConfigList);
	bbsd_sock = bbsd_socket();

	Wpd_Global_Type = get_message_type("WPD_GLOBAL_TYPE",
		Wpd_Global_Type_Str, sendPRIVATE);
	Wpd_Local_Type = get_message_type("WPD_LOCAL_TYPE",
		Wpd_Local_Type_Str, sendPRIVATE);
	error_report(log_wpd, TRUE);

	flush_time = Time(NULL) + Wpd_Flush;
	wp_update_time = calc_wp_time(0);

	startup();

	listen_port = Wpd_Port;
	listen_addr = Wpd_Bind_Addr;
	if((listen_sock = socket_listen(listen_addr, &listen_port)) == ERROR)
		error_print_exit(1);

	bbsd_port(Wpd_Port);
	signal(SIGPIPE, SIG_IGN);

	while(TRUE) {
		int cnt;
		fd_set ready;
		int fdlimit;

		error_print();

		if(shutdown_daemon) {
			log_f("wpd", "S", "Shutdown requested");
			if(user_image == DIRTY)
				write_user_file();
			if(bbs_image == DIRTY)
				write_bbs_file();
			return 0;
		}

		ap = procs;
		FD_ZERO(&ready);
		FD_SET(listen_sock, &ready);
		fdlimit = listen_sock;

		FD_SET(bbsd_sock, &ready);
		if(bbsd_sock > fdlimit)
			fdlimit = bbsd_sock;

		while(ap) {
			if(ap->fd != ERROR) {
				FD_SET(ap->fd, &ready);
				if(ap->fd > fdlimit)
					fdlimit = ap->fd;
			}
			NEXT(ap);
		}
		fdlimit += 1;

		t.tv_sec = 60;
		t.tv_usec = 0;

		if((cnt = select(fdlimit, (void*)&ready, NULL, NULL, &t)) < 0)
			continue;
		
		if(cnt == 0) {
			time_t now = Time(NULL);
			if(now > flush_time) {
				flush_time = now + Wpd_Flush;
				log_clear("wpd");
				if(user_image == DIRTY)
					write_user_file();
				if(bbs_image == DIRTY)
					write_bbs_file();
			}

			if(now > wp_update_time) {
				if(!(dbug_level & dbgNODAEMONS))
					bbsd_msg("WP update generation");

				{
					time_t t0;
					if(dbug_level & dbgVERBOSE)
						t0 = time(NULL);
					if(!(dbug_level & dbgNODAEMONS))
						bbsd_msg("Aging in progress");

					if(time_now == 0)
						generate_wp_update(NULL);

					if(dbug_level & dbgVERBOSE) {
						char out[80];
						sprintf(out, "Aging took %"PRTMd" seconds\n", time(NULL) - t0);
						puts(out);
						log_f("wpd", "S:", out);
					}
				}

				wp_update_time = calc_wp_time(wp_update_time);
				if(!(dbug_level & dbgNODAEMONS))
					bbsd_msg("");
			}

			continue;
		}

		if(FD_ISSET(bbsd_sock, &ready))
			shutdown_daemon = TRUE;

        if(FD_ISSET(listen_sock, &ready)) {
            ap = add_proc();
            if((ap->fd = socket_accept(listen_sock)) < 0)
                ap = remove_proc(ap);
			else
				if(socket_write(ap->fd, daemon_version("wpd", Bbs_Call)) == ERROR)
					ap = remove_proc(ap);
        }

        ap = procs;
        while(ap) {
            if(FD_ISSET(ap->fd, &ready)) {
                if(service_port(ap) == ERROR) {
                    ap = remove_proc(ap);
					continue;
				}
            }
            NEXT(ap);
        }

		wait3(NULL, WNOHANG, NULL);
	}

	return 0;
}

time_t
Time(time_t *t)
{
	time_t now = time_now;

	if(now == 0)
		now = time(NULL);

	if(t != NULL)
		*t = now;
	return now;
}

static void
log_wpd(char *str)
{
	log_f("wpd", "%s", str);
}

static int
get_message_type(const char *varname, const char *str, int def)
{
	if (str == NULL)
		return def;

	switch (str[0]) {
	case 'p':
	case 'P':
		return sendPRIVATE;
	case 'b':
	case 'B':
		return sendBULLETIN;
	default:
		error_log("%s: invalid message type '%c'.\n", varname, str[0]);
		break;
	}

	return def;
}
