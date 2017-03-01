/* requires a call to axchk() to be inserted in the main commutator loop of
	the net main.c code */

#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/wait.h>

#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"
#include "global.h"
#include "timer.h"
#include "ax25.h"
#include "mbuf.h"
#include "socket.h"

#if 0
#define DEBUG1 1
#endif

extern char
    versionc[80],
	*Bbs_My_Call,
	*Bin_Dir,
	*Bbs_Dir;

#include "ax_mbx.h"

static void
	convrt2cr(char *data, int cnt),
	convrt2nl(char *data, int cnt),
	mbx_tx(struct ax25_cb *axp, int cnt),
	mbx_rx(struct ax25_cb *axp, int cnt),
	shutdown_process(struct mboxsess *mbp, int issue_disc),
	set_ax25_params(char *p, struct ax25_cb *axp),
	freembox(struct mboxsess *mbp),
	initmbox(struct mboxsess *mbp);

static int
	calleq(struct ax25_cb *axp, struct ax25_addr *addr),
	command(struct mboxsess *mbp);

static struct mboxsess
	*newmbox(void);

struct mboxsess *base = NULLMBS;  /*pointer to base of mailbox chain*/
struct mboxsess fwdstruct;	/*forwarding session*/
static struct mboxsess *newmbox();

int control = 0;
char *Tncd_Host;
char *Tncd_Name;
char *Tncd_Device;

extern struct ax25_addr bbscall, fwdcall;

static int cntrl_socket;

/* We need to setup a socket that we will check from time to time to
 * see if a program would like access to the tnc's or to status. This
 * is the mechanism for polling for weather or forwarding.
 */

int
init_ax_control(int c_port, int m_port)
{
	int port = c_port;
	if((cntrl_socket = socket_listen(&port)) < 0) {
		fprintf(stderr,
		"init_ax_control: Problem opening control socket ... aborting\n");
		return ERROR;
	}
	port = m_port;
	if((monitor_socket = socket_listen(&port)) < 0) {
		fprintf(stderr,
		"init_ax_control: Problem opening monitor socket ... aborting\n");
		return ERROR;
	}
	return OK;
}


/* setup a control port where bbs processes can ask questions regarding
 * current connections, etc.
 */
void
ax_control(void)
{
	int fd;

	if((fd = accept_socket(cntrl_socket)) != ERROR) {
		struct mboxsess *mbp = newmbox();
		initmbox(mbp);

		if(dbug_level & dbgVERBOSE)
			printf("ax_control(), accepted socket %d\n", fd);
		mbp->fd = fd;
		socket_write(fd, versionc);
	}
}


static struct mboxsess *
newmbox(void)
{
	struct mboxsess *mbp ;
#ifdef DEBUG1
	int cnt = 2;
	char buf[80];
#endif

	if(base == NULLMBS){
		base = (struct mboxsess *)malloc(sizeof(struct mboxsess));
		base->next = NULLMBS;
		initmbox(base);
#ifdef DEBUG1
		bbsd_msg("usrcnt=1");
#endif
		return base;
	} else{
		mbp = base;
		while(mbp->next != NULLMBS) {	   /*go up the chain to the top*/
			mbp = mbp->next;
#ifdef DEBUG1
			cnt++;
#endif
		}
		mbp->next = (struct mboxsess *)malloc(sizeof(struct mboxsess));
		mbp->next->next = NULLMBS;
		initmbox(mbp->next);
#ifdef DEBUG1
		sprintf(buf, "usrcnt=%d", cnt);
		bbsd_msg(buf);
#endif
		return mbp->next;
	}
}

static void
initmbox(struct mboxsess *mbp)
{
	mbp->pid = 0;	
	mbp->spawned = FALSE;
	mbp->bytes = 0;
	mbp->byte_cnt = 0;
	mbp->p = mbp->buf;
	mbp->cmd_cnt = 0;
	mbp->cmd_state = 1;
	mbp->orig = NULL;
	mbp->fd = ERROR;
	mbp->socket = ERROR;
}

static void
freembox(struct mboxsess *mbp)
{
	struct mboxsess *p;
#ifdef DEBUG1
	int cnt = 0;
	char buf[80];
#endif
	
	if(mbp == base){					/*special case for base session*/
		base = base->next;				/*make base point to next session*/
		free(mbp);						/*free up the storage*/
	} else {

		if(mbp->orig)
			free(mbp->orig);

		p = base;
		for(;;){
			if(p->next == mbp) {		/*if next upward session is THE one*/
				p->next = mbp->next;	/*eliminate the next upward session*/
				free(mbp);
				break;
			}
			if(p->next == NULLMBS) {	/*something is wrong here!*/
				free(mbp);				/*try to fix it*/
				break;
			}
			p = p->next;
		}
	}

#ifdef DEBUG1
	p = base;
	while(p != NULLMBS) {	   /*go up the chain to the top*/
		NEXT(p);
		cnt++;
	}
	sprintf(buf, "usrcnt=%d", cnt);
	bbsd_msg(buf);
#endif
}

static void
set_ax25_params(char *p, struct ax25_cb *axp)
{
	char param[20];
	int value;

	NextChar(p);

	if(*p == 0)
		return;
	strcpy(param, get_string(&p));
	if(*p == 0)
		return;
	value = get_number(&p);

	uppercase(param);
	if(!strcmp(param, "T1")) {
		axp->t1.start = value * (axp->addr.ndigis + 1);
		return;
	}
	if(!strcmp(param, "T2")) {
		axp->t2.start = value * (axp->addr.ndigis + 1);
		return;
	}
	if(!strcmp(param, "T3")) {
		axp->t3.start = value * (axp->addr.ndigis + 1);
		return;
	}
	if(!strcmp(param, "MAXFRAME")) {
		axp->maxframe = value;
		return;
	}
	if(!strcmp(param, "PACLEN")) {
		axp->paclen = value;
		axp->pthresh = value;
		return;
	}
	if(!strcmp(param, "N2")) {
		axp->n2 = value;
		return;
	}
}

static int
command(struct mboxsess *mbp)
{
	struct ax25 addr;
	extern int axwindow;
	int i;

	for(i=0; i<mbp->cmd_cnt; i++) {
		char call[10], *q, *p = mbp->command[i];

		if(dbug_level & dbgVERBOSE)
			printf("Command: %s\n", p);

		switch(*p++) {
		case 'B':
			return ERROR;

		case 'S':
			set_ax25_params(p, mbp->axbbscb);
			break;

		case 'C':
				/* N0ARY-1>N6ZFJ,N6UNE,N6ZZZ
				 *
				 *   MYCALL N0ARY-1
				 *   C N6ZFJ via N6UNE,N6ZZZ
				 */

			q = call;
			while(*p != '>') *q++ = *p++;
			*q = 0;
			mbp->orig = (struct ax25_addr *)malloc(sizeof(struct ax25_addr));
#if 1
			setcall(&addr.source, call);
			setcall(mbp->orig, call);
#else
			setcall(&addr.source, Bbs_My_Call);
			setcall(mbp->orig, Bbs_My_Call);
#endif
			p++;

			q = call;
			while(*p != ',' && *p) *q++ = *p++;
			*q = 0;
			setcall(&addr.dest, call);

			addr.ndigis = 0;
			while(*p++ == ',' && addr.ndigis < 8) {
				q = call;
				while(*p != ',' && *p) *q++ = *p++;
				*q = 0;
				setcall(&addr.digis[addr.ndigis], call);
				addr.ndigis++;
			}

			if(find_ax25(&addr.source, &addr.dest) != NULLAX25)
				return ERROR;

			mbp->axbbscb =
				open_ax25(&addr,axwindow,mbx_rx,mbx_tx,mbx_state,0,(char *)0);
		}
	}
	mbp->cmd_cnt = 0;
	return OK;
}

void
axchk(void)
{
	struct mbuf *bp;
	struct mboxsess * mbp;
	char *cp;
	int testsize,size;
	int connect_cnt;
	
	if(base == NULLMBS)
		return;

	mbp = base;
	connect_cnt = 0;

	while(mbp != NULLMBS) {
		char buf[1025];
		int cnt;
		struct mboxsess *tmbp;

				/* check to see if the bbs has connected to us yet? If not
				 * go see if it's there. If it's not skip to next process.
				 */
		if(mbp->fd == ERROR) {
			if((mbp->fd = accept_socket(mbp->socket)) == ERROR) {
				mbp = mbp->next;
				continue;
			}
			if(dbug_level & dbgVERBOSE)
				printf("axchk(), accepted socket %d\n", mbp->fd);
		}

		connect_cnt++;

				/* we do have an active bbs or outgoing process. If we don't
				 * have any data pending on this process then check to see
				 * if the process has sent us something new.
				 */

		if(mbp->byte_cnt == 0) {
			cnt = read(mbp->fd, buf, 1024);
			switch(cnt) {
			case 0:
				if(dbug_level & dbgVERBOSE)
					printf("axchk: %s broken pipe\n", mbp->call);
				tmbp = mbp;
				mbp = mbp->next;
				shutdown_process(tmbp, TRUE);
				continue;

			case ERROR:
				switch(errno) {
				case EAGAIN:
#if EWOULDBLOCK != EAGAIN
				case EWOULDBLOCK:
#endif
					mbp->byte_cnt = 0;
					break;
				default:
					if(dbug_level & dbgVERBOSE)
						printf("axchk: %s broken pipe\n", mbp->call);
						/* anything other than the items above indicate a
						 * fatal problem or broken pipe. Let's shut down
						 * the process.
						 */
					tmbp = mbp;
					mbp = mbp->next;
					shutdown_process(tmbp, TRUE);
					continue;
				}
				break;

			default:
				{
					int cmd_cnt = 0;
					char *p = buf;
					char *q = mbp->buf;

					buf[cnt] = 0;
					if(dbug_level & dbgVERBOSE)
						printf("%s<{%s}\n", mbp->call, buf);

					while(cnt--) {
						if(*p == '\r') {
							p++;
							continue;
						}

						switch(mbp->cmd_state) {
						case 0:	/* IDLE */
							if(*p == '\n')
								mbp->cmd_state++;
							break;
						case 1: /* CR seen looking for '~' */
							if(*p == '~') {
								mbp->cmd_state++;
								p++;
								continue;
							}
							mbp->cmd_state = 0;
							break;
						case 2: /* '~' seen */
							if(*p == '~') {
								mbp->cmd_state = 0;
								break;
							}
							mbp->cmd_state++;
							cmd_cnt = 0;
							mbp->command[mbp->cmd_cnt][cmd_cnt++] = *p++;
							continue;
						case 3: /* in command */
							if(*p == '\n') {
								mbp->command[mbp->cmd_cnt++][cmd_cnt] = 0;
								mbp->cmd_state = 1;
							} else
								mbp->command[mbp->cmd_cnt][cmd_cnt++] = *p;
							p++;
							continue;
						}
						if(*p) {
							*q++ = *p++;
							mbp->byte_cnt++;
						} else
							p++;
					}
					mbp->p = mbp->buf;

				}
				break;
			}
		}

		if(mbp->cmd_cnt)
			if(command(mbp) == ERROR) {
				tmbp = mbp;
				mbp = mbp->next;
				shutdown_process(tmbp, TRUE);
				connect_cnt--;
				continue;
			}

		if(mbp->byte_cnt) {
			if(dbug_level & dbgVERBOSE)
				printf("axchk: %s byte_cnt = %d\n", mbp->call, mbp->byte_cnt);
			testsize = min(mbp->bytes, mbp->axbbscb->paclen+1);
			size = min(testsize, mbp->byte_cnt) + 1;
			bp = alloc_mbuf(size);			 
			cp = bp->data;

			*cp++ = PID_FIRST | PID_LAST | PID_NO_L3;
			bp->cnt =1;
		
			while(bp->cnt < size && mbp->byte_cnt){
				*cp++ = *mbp->p++;
				bp->cnt++;
				mbp->byte_cnt--;
			}

			if(dbug_level & dbgVERBOSE)
				printf("[%s]\n", &(bp->data[1]));

			convrt2cr(bp->data, bp->cnt);
			send_ax25(mbp->axbbscb, bp);
		}

		mbp = mbp->next;
	}
}

static void
shutdown_process(struct mboxsess *mbp, int issue_disc)
{
	if(dbug_level & dbgVERBOSE)
		printf("Shutdown process %d..\n", mbp->pid);
	if(mbp->spawned) {
		if(dbug_level & dbgVERBOSE)
			printf("Killing process %d..\n", mbp->pid);
		kill(mbp->pid,9);
	}

	if(mbp->fd != ERROR) {
		if(dbug_level & dbgVERBOSE)
			printf("closing fd %x..\n", mbp->fd);
		close(mbp->fd);
	}
	if(mbp->socket != ERROR) {
		if(dbug_level & dbgVERBOSE)
			printf("closing socket %x..\n", mbp->socket);
		close(mbp->socket);
	}

	if(issue_disc) {
		if(dbug_level & dbgVERBOSE)
			printf("Issue disconnect\n");
		if(mbp->axbbscb != NULL)
			disc_ax25(mbp->axbbscb);
	}

		/* it should have died by now. Let's get it's completion
		 * status.
		 */

	if(mbp->spawned)
		waitpid(mbp->pid, NULL, WNOHANG);
		
#if 0
	if(mbp->orig)
		delete_call(mbp->orig);
#endif

	freembox(mbp);
	if(dbug_level & dbgVERBOSE)
		printf("Shutdown complete\n");
}

void
mbx_state(struct ax25_cb *axp, int old, int new)
{
	char call[10], port[10];
	int pid, j;
	struct mboxsess *mbp;

	if(dbug_level & dbgVERBOSE)
		printf("mbx_state(%d, %d)\n", old, new);
	switch(new){
	case DISCONNECTED:
		if(dbug_level & dbgVERBOSE)
			printf("mbx_state(DISCONNECTED)\n");
		if((old == DISCONNECTED) || (old == DISCPENDING))
		   return;
		if(base == NULLMBS)
			break;
		if(dbug_level & dbgVERBOSE)
			printf("mbx_state(isDISCONNECTED)\n");
		mbp = base;
		while(mbp != NULLMBS){
			if(axp == mbp->axbbscb){
				if(dbug_level & dbgVERBOSE)
					printf("mbx_state:Killing BBS, disconnect recv\n");
				shutdown_process(mbp, FALSE);
				break; /* from while loop */
			}
			mbp = mbp->next;
		}
		break;   /*end of DISCONNECTED case*/
					
	case CONNECTED:
		switch(old) {
		case SETUP:
			mbp = base;
			while(mbp != NULLMBS && axp != mbp->axbbscb)
				mbp = mbp->next;
			if(mbp == NULLMBS) {
			if(dbug_level & dbgVERBOSE)
				printf("couldn't find the process\n");
				exit(1);
			}
			write(mbp->fd, "~C\n", 3);

			mbp->axbbscb=axp;
			mbp->bytes = 128;	/*jump start the upcall*/
			return;

		case CONNECTED:
				/* we are already CONNECTED but received another SABM
				 * this means they probably didn't get our UA
				 */
		if(dbug_level & dbgVERBOSE)
			printf("Another SABM received, they must be deaf.\n");
			if(base == NULLMBS)
				break;

			mbp = base;
			while(mbp != NULLMBS){
				if(axp == mbp->axbbscb){
					if(dbug_level & dbgVERBOSE)
						printf("mbx_state:Killing BBS, disconnect recv\n");
					shutdown_process(mbp, FALSE);
					break; /* from while loop */
				}
				mbp = mbp->next;
			}
			break;

		default:
			return;

		case DISCONNECTED:
			break;
		}

			if(!calleq(axp,&bbscall)) { /*not for the mailbox*/
				axp->s_upcall = NULL;
				return;
			}
	
		if(dbug_level & dbgVERBOSE)
			printf("mbx_state(isCONNECTED)\n");
		mbp =newmbox();		/*after this, this is a mailbox connection*/
								/* so, make a new mailbox session*/
		axp->r_upcall = mbx_rx ;
		axp->t_upcall = mbx_tx ;

		mbp->axbbscb=axp;
		mbp->bytes = 128;	/*jump start the upcall*/

		for(j=0;j<6;j++){			   /*now, get incoming call letters*/
			call[j]=mbp->axbbscb->addr.dest.call[j];
			call[j]=call[j] >> 1;
			call[j]=call[j] & (char)0x7f;
			if(call[j]==' ') call[j]='\0';
		}
		call[6]='\0';			/*terminate call letters*/
		strcpy(mbp->call,call);   /*Copy call to session*/

		if(mbp->pid == 0) {
			mbp->fd = ERROR;
			mbp->port = 0;
			mbp->socket = socket_listen(&(mbp->port));
			sprintf(port, "%d", mbp->port);

		if(dbug_level & dbgVERBOSE) {
			printf("Starting bbs process [1]:\n");
			printf("\t%s/b_bbs -v %s -s %s %s\n",
				Bin_Dir, Tncd_Name, port, call);
		}
										/*now, fork and exec the bbs*/
			if((pid=fork()) == 0){			/*if we are the child*/
				char cmd[256];

				sprintf(cmd, "%s/b_bbs", Bin_Dir);
				execl(cmd, "b_bbs", "-v", Tncd_Name, "-s", port, call, 0);
				exit(1);
			}

			mbp->pid=pid;				/* save pid of new baby*/
			mbp->spawned = TRUE;
		} else {
			char *buf = "CONNECTED\n";
			write(mbp->fd, buf, strlen(buf));
		}
		break;
	}
}

/* Incoming mailbox session via ax.25 */

/* * This is the ax25 receive upcall function
 *it gathers incoming data and stuff it down the IPC queue to the proper BBS
 */

/*ARGSUSED*/
void
mbx_incom(struct ax25_cb *axp, int cnt)
{
	char call[10], port[10];
	int pid, j;
	struct mboxsess *mbp;
	struct mbuf *bp;
	
	if(dbug_level & dbgVERBOSE)
		printf("mbx_incom()\n");

	mbp = newmbox();	/*after this, this is a mailbox connection*/
						/* so, make a new mailbox session*/
	axp->r_upcall = mbx_rx ;
	axp->t_upcall = mbx_tx ;
	axp->s_upcall = mbx_state;

	mbp->axbbscb=axp;
	mbp->bytes = 128;	/*jump start the upcall*/

	bp = recv_ax25(axp) ;	/* get the initial input */
	free_p(bp) ;			/* and throw it away to avoid confusion */


	for(j=0;j<6;j++){			   /*now, get incoming call letters*/
		call[j] = mbp->axbbscb->addr.dest.call[j];
		call[j] = call[j] >> 1;
		call[j] = call[j] & (char)0x7f;
		if(call[j]==' ')
			call[j]='\0';
	}
	call[6]='\0';					/*terminate call letters*/
	strcpy(mbp->call, call);   		/*Copy call to session*/

	mbp->fd = ERROR;
	mbp->port = 0;
	mbp->socket = socket_listen(&(mbp->port));
	sprintf(port, "%d", mbp->port);
 
	if(dbug_level & dbgVERBOSE) {
		printf("Starting bbs process [2]:\n");
		printf("\t%s/b_bbs -v %s -s %s %s\n",
			Bin_Dir, Tncd_Name, port, call);
	}
									/*now, fork and exec the bbs*/
	if((pid=fork()) == 0){			/*if we are the child*/
		char cmd[256];
		sprintf(cmd, "%s/b_bbs", Bin_Dir);
		execl(cmd, "b_bbs", "-v", Tncd_Name, "-s", port, call, 0);
		exit(1);
	}
	mbp->pid=pid;				/* save pid of new baby*/
	mbp->spawned = TRUE;
}	

/*ARGSUSED*/
static void
mbx_rx(struct ax25_cb *axp, int cnt)
{
	struct mbuf *bp;
	struct mboxsess * mbp;

	
	if(base == NULLMBS)
		return;

	if(dbug_level & dbgVERBOSE)
		printf("mbx_rx()\n");
	mbp = base;
	while(mbp != NULLMBS){
		if(mbp->axbbscb == axp){  /* match requested block? */
			if((bp = recv_ax25(axp)) == NULLBUF)  /*nothing there*/
				continue;
			while(bp != NULLBUF){
				convrt2nl(bp->data, bp->cnt);
				if(dbug_level & dbgVERBOSE)
					write(2, bp->data, bp->cnt);
				write(mbp->fd, bp->data, bp->cnt);
				bp = free_mbuf(bp);   /*free the mbuf and get the next */
			}
		}
	mbp = mbp->next;
	}
}

static void
mbx_tx(struct ax25_cb *axp, int cnt)
{
	struct mboxsess *mbp;
	if(base == NULLMBS)
		return;					 /*no sessions*/

	if(dbug_level & dbgVERBOSE)
		printf("mbx_tx()\n");
	mbp = base;
	while(mbp != NULLMBS){
		if(mbp->axbbscb == axp)
			mbp->bytes = cnt;
		mbp = mbp->next;
	}
}

static int
calleq(struct ax25_cb *axp, struct ax25_addr *addr)
{
	if(memcmp((char*)axp->addr.source.call, (char*)addr->call, ALEN) != 0)
		return 0;
	if((axp->addr.source.ssid & SSID) != (addr->ssid & SSID))
		return 0;
	return 1;
}

static void
convrt2nl(char *data, int cnt)
{
	int i;
	for(i=0; i<cnt; i++) {
		if(*data == '\r')
			*data = '\n';
		data++;
	}
}

static void
convrt2cr(char *data, int cnt)
{
	int i;
	for(i=0; i<cnt; i++) {
		if(*data == '\n')
			*data = '\r';
		data++;
	}
}
