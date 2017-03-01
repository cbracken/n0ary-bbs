#include <time.h>

extern void
    read_routing(char *buf, char *homebbs, time_t *orig_date, int *num),
	init_forwarding(char *port, char *call);

/* bbs.c */

extern int
	chat(void);

extern void
	putchar_socket(char *str),
	get_socket(char *str, int cnt);

extern void
#ifndef SABER
	print_socket(char *fmt, ...);
#else
	print_socket();
#endif

void write_socket(const char *, size_t);

extern time_t
	Time(time_t *t);

/*addr.c*/

extern int
	isState(char *p);

extern char
	*get_country_by_state(char *s),
	*get_continent_by_state(char *s);

extern void
	exit_bbs(void);

extern void
	trim_comment(char *s),
	convert_dash2underscore(char *s),
	GETnSTR(char *str, int cnt, int caps),
	GETnSTRdef(char *str, int cnt, int caps, char *def),
	case_string(char *p, int caps),
	case_strcpy(char *to, char *from, int caps);

extern int
	get_yes_no(int def),
	get_yes_no_quit(int def),
	get_quit_scroll_cmd(int def),
	check_long_xfer_abort(int msg, int cnt),
	isCall(char *s);

extern short
	sum_string(char *s);

/*event.c*/
extern int
	event(void);


/*file.c*/
extern int
	Open(char *filename, int mode),
	last_file_altered();

extern void
	Close(int filedes);

/* distrib.c */
extern int
	distrib_t(void);

/*filesys.c*/
extern int
	signature(void),
	vacation(void),
	filesys(void);


/*help.c*/
extern int
	help(void),
	information(void),
	initialize_help_message(void),
	system_msg(int num),
	bad_cmd(int errmsg, int location),
	bad_cmd_double(int errmsg, int loc1, int loc2),
	error(int errmsg);

extern void
	system_msg_number(int num, int value),
	system_msg_string(int num, char *str),
	system_msg_numstr(int num, int value, char *str),
	show_command_menus(int which),
	close_down_help_messages(void);
	

/* load.c */
extern int
	load_t(void);

/*login.c*/
extern int
	login_user(void);


/*maintenance.c*/
extern void
	check_all_files(void);

extern int
	maint(void);

/*parse.c*/
extern int
	parse_command_line(char *str);

extern struct TOKEN
	*resync_token_via_location(struct TOKEN *t, char *new_loc),
	*grab_token_struct(struct TOKEN *prev);

extern void
	pipe_msg_num(int num);

/*server.c*/
extern int
	initiate(void);

/*lock.c*/
extern int
	ports(void);

/*process.c*/
extern int
	uustat(void),
	process(void);

/*wx.c*/
extern int
	wx(void);

/*unixlogin.c*/
int unixlogin(void);
