#include <stdio.h>
#include <stdlib.h>

#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"
#include "user.h"

struct UserList *UL = NULL;

static int
usrfile_put(struct UserInformation *u, FILE *fp)
{
	int i = 0;
	int lastseen = 0, cnt = 0, where = 0;
	struct IncludeList *list;
	struct Ports *port = u->port;

	while(port) {
		if(port->firstseen) {
			if(lastseen < port->lastseen) {
				lastseen = port->lastseen;
				where = i;
				cnt += port->count;
			}
		}
		NEXT(port);
		i++;
	}

	fprintf(fp, "+%ld %d %d %d %d\n",
		u->number, (u->immune) ? 1:0, lastseen, where, cnt);

	fprintf(fp, "NUMBER %ld\n", u->number);
	if(u->lname[0]) fprintf(fp, "LNAME %s\n", u->lname);
	if(u->phone[0]) fprintf(fp, "PHONE %s\n", u->phone);
	if(u->email_addr[0]) fprintf(fp, "ADDRESS %s\n", u->email_addr);
	if(u->freq[0]) fprintf(fp, "FREQ %s\n", u->freq);
	if(u->tnc[0]) fprintf(fp, "TNC %s\n", u->tnc);
	if(u->software[0]) fprintf(fp, "SOFTWARE %s\n", u->software);
	if(u->computer[0]) fprintf(fp, "COMPUTER %s\n", u->computer);
	if(u->rig[0]) fprintf(fp, "RIG %s\n", u->rig);
	if(u->lines) fprintf(fp, "LINES %ld\n", u->lines);
	if(u->base) fprintf(fp, "BASE %ld\n", u->base);

	list = u->Include;
	while(list) {
		fprintf(fp, "INCLUDE %s\n", list->str);
		NEXT(list);
	}

	list = u->Exclude;
	while(list) {
		fprintf(fp, "EXCLUDE %s\n", list->str);
		NEXT(list);
	}
	fprintf(fp, "MESSAGE %ld\n", u->message);
	if(u->bbs) fprintf(fp, "BBS\n");
	if(u->sysop) fprintf(fp, "SYSOP\n");
	if(u->approved) fprintf(fp, "APPROVED\n");
	if(u->nonham) fprintf(fp, "NONHAM\n");
	if(u->email) fprintf(fp, "EMAIL\n");
	if(u->emailall) fprintf(fp, "EMAILALL\n");
	if(u->vacation) fprintf(fp, "VACATION\n");
	if(u->signature) fprintf(fp, "SIGNATURE\n");
	if(u->logging) fprintf(fp, "LOGGING\n");
	if(u->immune) fprintf(fp, "IMMUNE\n");
	if(u->regexp) fprintf(fp, "REGEXP\n");
	if(u->halfduplex) fprintf(fp, "DUPLEX\n");
	if(u->newline) fprintf(fp, "NEWLINE\n");
	if(u->uppercase) fprintf(fp, "UPPERCASE\n");
	if(u->ascending) fprintf(fp, "ASCEND\n");
	fprintf(fp, "HELP %ld\n", u->help);

	for(i=0; i<10; i++)
		if(u->macro[i][0])
			fprintf(fp, "MACRO %d %s\n", i, u->macro[i]);

	port = u->port;
	while(port) {
		fprintf(fp, "PORT %s %s %ld %ld %ld\n", port->name,
			port->allow ? "OK":"NO", port->count,
			port->firstseen, port->lastseen);
		NEXT(port);
	}

	return OK;
}

struct IncludeList *
free_list(struct IncludeList *list)
{
	struct IncludeList *t;

	while(list) {
		t = list;
		NEXT(list);
		free(t);
	}

	return NULL;
}

struct IncludeList *
add_2_list(struct IncludeList *List, char *txt)
{
	struct IncludeList *t = malloc_struct(IncludeList);
	t->next = List;
	strcpy(t->str, txt);
	return t;
}

static int
usrfile_parse(struct UserInformation *u, FILE *fp)
{
	char buf[256];
	struct Ports *port = u->port;

	bzero(u, SizeOfUserInfo);
	u->port = port;

	while(port) {
		port->allow = FALSE;
		port->count = 0;
		port->firstseen = 0;
		port->lastseen = 0;
		NEXT(port);
	}

	while(fgets(buf, 256, fp)) {
		char *p, *q = buf;

		if(*q == '+')
			continue;

		buf[strlen(buf)-1] = 0;
		p = get_string(&q);

		if(!strcmp(p, "LNAME")) {
			strncpy(u->lname, q, LenLNAME);
			continue;
		}
		if(!strcmp(p, "PHONE")) {
			strncpy(u->phone, q, LenPHONE);
			continue;
		}
		if(!strcmp(p, "ADDRESS")) {
			strncpy(u->email_addr, q, LenEMAIL);
			continue;
		}
		if(!strcmp(p, "FREQ")) {
			strncpy(u->freq, q, LenFREQ);
			continue;
		}
		if(!strcmp(p, "TNC")) {
			strncpy(u->tnc, q, LenEQUIP);
			continue;
		}
		if(!strcmp(p, "SOFTWARE")) {
			strncpy(u->software, q, LenEQUIP);
			continue;
		}
		if(!strcmp(p, "COMPUTER")) {
			strncpy(u->computer, q, LenEQUIP);
			continue;
		}
		if(!strcmp(p, "RIG")) {
			strncpy(u->rig, q, LenEQUIP);
			continue;
		}
		if(!strcmp(p, "LINES")) {
			u->lines = get_number(&q);
			continue;
		}
		if(!strcmp(p, "BASE")) {
			u->base = get_number(&q);
			continue;
		}
		if(!strcmp(p, "NUMBER")) {
			u->number = get_number(&q);
			continue;
		}
		if(!strcmp(p, "INCLUDE")) {
			u->Include = add_2_list(u->Include, q);
			continue;
		}
		if(!strcmp(p, "EXCLUDE")) {
			u->Exclude = add_2_list(u->Exclude, q);
			continue;
		}
		if(!strcmp(p, "MESSAGE")) {
			u->message = get_number(&q);
			continue;
		}
		if(!strcmp(p, "BBS")) {
			u->bbs = TRUE;
			continue;
		}
		if(!strcmp(p, "SYSOP")) {
			u->sysop = TRUE;
			continue;
		}
		if(!strcmp(p, "APPROVED")) {
			u->approved = TRUE;
			continue;
		}
		if(!strcmp(p, "NONHAM")) {
			u->nonham = TRUE;
			continue;
		}
		if(!strcmp(p, "EMAIL")) {
			u->email = TRUE;
			continue;
		}
		if(!strcmp(p, "EMAILALL")) {
			u->emailall = TRUE;
			continue;
		}
		if(!strcmp(p, "VACATION")) {
			u->vacation = TRUE;
			continue;
		}
		if(!strcmp(p, "SIGNATURE")) {
			u->signature = TRUE;
			continue;
		}
		if(!strcmp(p, "LOGGING")) {
			u->logging = TRUE;
			continue;
		}
		if(!strcmp(p, "IMMUNE")) {
			u->immune = TRUE;
			continue;
		}
		if(!strcmp(p, "REGEXP")) {
			u->regexp = TRUE;
			continue;
		}
		if(!strcmp(p, "DUPLEX")) {
			u->halfduplex = TRUE;
			continue;
		}
		if(!strcmp(p, "NEWLINE")) {
			u->newline = TRUE;
			continue;
		}
		if(!strcmp(p, "UPPERCASE")) {
			u->uppercase = TRUE;
			continue;
		}
		if(!strcmp(p, "ASCEND")) {
			u->ascending = TRUE;
			continue;
		}
		if(!strcmp(p, "HELP")) {
			u->help = get_number(&q);
			continue;
		}
		if(!strcmp(p, "MACRO")) {
			int num = get_number(&q);
			strncpy(u->macro[num], q, LenMACRO);
			continue;
		}
		if(!strcmp(p, "PORT")) {
			p = get_string(&q);
			port = u->port;

			while(port) {
				if(!stricmp(port->name, p)) {
					port->allow = FALSE;
					if(!strcmp("OK", get_string(&q)))
						port->allow = TRUE;
					port->count = get_number(&q);
					port->firstseen = get_number(&q);
					port->lastseen = get_number(&q);
					break;
				}
				NEXT(port);
			}
		}
	}
	return OK;
}

struct UserList *
usrfile_find(char *call)
{
	struct UserList *ul = UL;
	while(ul) {
		if(!strcmp(ul->call, call))
			return ul;
		NEXT(ul);
	}
	return NULL;
}

static struct UserList *
usrfile_allocate(void)
{
	struct UserList **ul = &UL;
	struct UserList *t = malloc_struct(UserList);
	struct PortDefinition *pd = port_table();
	struct Ports **port = &(t->info.port);

	while(*ul)
		ul = &((*ul)->next);

	*ul = t;

	while(pd) {
		*port = malloc_struct(Ports);
		strcpy((*port)->name, pd->name);
		port = &((*port)->next);
		NEXT(pd);
	}
	return t;
}

static void
usrfile_attach_proc(struct UserList *ul, struct active_processes *ap)
{
	struct ProcList *t, *pl = ul->pl;

	while(pl) {
		if(pl->ap == ap)
			return;
		NEXT(pl);
	}

	t = malloc_struct(ProcList);
	t->next = ul->pl;
	t->ap = ap;
	ul->pl = t;
}

static struct ProcList *
usrfile_detach_proc(struct UserList *ul, struct active_processes *ap)
{
	struct ProcList *t, *pl = ul->pl;

	if(ul->pl->ap == ap) {
		t = ul->pl;
		ul->pl = t->next;
		free(t);
		return ul->pl;
	}

	while(pl->next) {
		if(pl->next->ap == ap) {
			t = pl->next;
			pl->next = t->next;
			free(t);
			return ul->pl;
		}
		NEXT(pl);
	}

	return ul->pl;
}

struct UserInformation *
usrfile_read(char *call, struct active_processes *ap)
{
	struct UserList *ul = usrfile_find(call);
	char fn[80];
	FILE *fp;

	if(ul != NULL) {
		usrfile_attach_proc(ul, ap);
		ap->ul = ul;
		return &ul->info;
	}

	sprintf(fn, "%s/%s", Userd_Acc_Path, call);
	if((fp = fopen(fn, "r")) == NULL)
		return NULL;

	ul = usrfile_allocate();
	if(usrfile_parse(&(ul->info), fp) == ERROR) {
		fclose(fp);
		return NULL;
	}

	fclose(fp);
	ul->condition = CLEAN;
	strcpy(ul->call, call);
	usrfile_attach_proc(ul, ap);
	ap->ul = ul;
	return &ul->info;
}

static int
default_true_false(char *item)
{
	char *c, cmd[80];
	sprintf(cmd, "USERD_DEF_%s", item);
	c = bbsd_get_variable(cmd);
	uppercase(c);

	if(!strncmp(c, "TRUE", 4))
		return TRUE;
	return FALSE;
}

static int
default_number(char *item)
{
	char *c, cmd[80];
	sprintf(cmd, "USERD_DEF_%s", item);
	c = bbsd_get_variable(cmd);

	return atoi(c);
}

static char *
default_string(char *item)
{
	char *c, cmd[80];
	sprintf(cmd, "USERD_DEF_%s", item);
	c = bbsd_get_variable(cmd);

	if(!strncmp(c, "NO,", 3))
		return NULL;

	return c;
}

int
usrfile_create(char *call)
{
	struct UserInformation u;
	struct PortDefinition *pd = port_table();
	struct Ports **port, *p;
	char fn[80];
	char *c, *s, buf[256];
	FILE *fp;
	time_t now = Time(NULL);

	sprintf(fn, "%s/%s", Userd_Acc_Path, call);
	if(fopen(fn, "r") != NULL)
		return TRUE;

	bzero(&u, SizeOfUserInfo);

	while(!usrdir_unique_number(u.number))
		u.number++;

	port = &u.port;
	while(pd) {
		*port = malloc_struct(Ports);
		strcpy((*port)->name, pd->name);
		if(!strcmp(pd->name, "STATUS")) {
			(*port)->firstseen = now;
			(*port)->lastseen = now;
			(*port)->count = 1;
		}
		port = &(*port)->next;
		NEXT(pd);
	}

	s = bbsd_get_variable("USERD_DEF_ALLOWED");
	if(strncmp(s, "NO,", 3))
		while((c = get_string(&s)) != 0) {
			uppercase(c);
			p = u.port;
			while(p) {
				if(!strcmp(c, p->name))
					p->allow = TRUE;
				NEXT(p);
			}
		}

	u.approved = default_true_false("APPROVED");
	u.ascending = default_true_false("ASCENDING");
	u.lines = default_number("LINES");
	u.base = default_number("BASE");
	u.help = default_number("HELP");
	u.logging = default_true_false("LOGGING");
	u.sysop = default_true_false("SYSOP");
	u.nonham = default_true_false("NONHAM");
	u.newline = default_true_false("NEWLINE");
	u.uppercase = default_true_false("UPPERCASE");
	u.immune = default_true_false("IMMUNE");
	u.halfduplex = default_true_false("HALFDUPLEX");
	u.regexp = default_true_false("REGEXP");
	u.bbs = default_true_false("BBS");

	strcpy(u.macro[0], default_string("MACRO0"));
	strcpy(u.macro[1], default_string("MACRO1"));
	strcpy(u.macro[2], default_string("MACRO2"));
	strcpy(u.macro[3], default_string("MACRO3"));
	strcpy(u.macro[4], default_string("MACRO4"));
	strcpy(u.macro[5], default_string("MACRO5"));
	strcpy(u.macro[6], default_string("MACRO6"));
	strcpy(u.macro[7], default_string("MACRO7"));
	strcpy(u.macro[8], default_string("MACRO8"));
	strcpy(u.macro[9], default_string("MACRO9"));

	
	if((fp = spool_fopen(fn)) == NULL)
		return ERROR;

	if(usrfile_put(&u, fp) != OK) {
		spool_fclose(fp);
		return ERROR;
	}

	spool_fclose(fp);

	if((fp = fopen(fn, "r")) == NULL)
		return ERROR;

	fgets(buf, 256, fp);
	fclose(fp);

	usrdir_allocate(call, &buf[1]);
	return OK;
}

int
usrfile_write(char *call)
{
	struct UserList *ul = usrfile_find(call);
	char fn[80];
	FILE *fp;

	if(ul == NULL)
		return OK;

	sprintf(fn, "%s/%s", Userd_Acc_Path, call);
	if((fp = spool_fopen(fn)) == NULL)
		return ERROR;

	if(usrfile_put(&(ul->info), fp) != OK) {
		spool_fclose(fp);
		return ERROR;
	}

	spool_fclose(fp);
	return OK;
}


int
usrfile_close(struct active_processes *ap)
{
	struct UserList *t = NULL, *ul = UL;

	while(ul) {
		if(ul == ap->ul)
			break;
		t = ul;
		NEXT(ul);
	}

	if(ul == NULL)
		return OK;

	if(usrfile_detach_proc(ul, ap) == NULL) {
		if(ul->condition == DIRTY)
			if(usrfile_write(ul->call) != OK)
				return ERROR;

		while(ul->info.port) {
			struct Ports *port = ul->info.port;
			ul->info.port = ul->info.port->next;
			free(port);
		}

		if(t == NULL) {
			UL = ul->next;
			free(ul);
			ul = UL;
		} else {
			t->next = ul->next;
			free(ul);
			ul = t->next;
		}
	}
	ap->ul = NULL;
	return OK;
}

int
usrfile_close_all(struct active_processes *ap)
{
	struct UserList *t = NULL, *ul = UL;

	if(ul == NULL)
		return OK;

	while(ul) {
		if(usrfile_detach_proc(ul, ap) == NULL) {
			if(ul->condition == DIRTY)
				if(usrfile_write(ul->call) != OK)
					return ERROR;
	
			while(ul->info.port) {
				struct Ports *port = ul->info.port;
				ul->info.port = ul->info.port->next;
				free(port);
			}

			if(t == NULL) {
				UL = ul->next;
				free(ul);
				ul = UL;
			} else {
				t->next = ul->next;
				free(ul);
				ul = t->next;
			}
			continue;
		}

		t = ul;
		NEXT(ul);
	}
	return OK;
}

int
usrfile_kill(char *call)
{
	struct UserList *ul = UL;

	if(UL == NULL)
		return OK;

	if(!strcmp(call, UL->call)) {
		UL = ul->next;
		while(ul->info.port) {
			struct Ports *port = ul->info.port;
			ul->info.port = ul->info.port->next;
			free(port);
		}

		free(ul);
		return OK;
	}

	while(ul->next) {
		if(!strcmp(call, ul->next->call)) {
			struct UserList *t = ul->next;
			ul->next = t->next;
			while(t->info.port) {
				struct Ports *port = t->info.port;
				t->info.port = t->info.port->next;
				free(port);
			}
			free(t);
			return OK;
		}
		NEXT(ul);
	}

	return OK;
}

char *
usrfile_show()
{
	struct UserList *ul = UL;
	int i = 0;

	sprintf(output, "usrfile_show()\n");
	while(ul) {
		struct ProcList *pl = ul->pl;
		sprintf(output, "%s\t%d %s [%s]\t", output, i++, ul->call,
			(ul->condition == CLEAN) ? "CLEAN":"DIRTY");
		while(pl) {
			sprintf(output, "%s%p ", output, pl->ap);
			NEXT(pl);
		}
		strcat(output, "\n");
		NEXT(ul);
	}
	strcat(output, "\n");
	return output;
}
