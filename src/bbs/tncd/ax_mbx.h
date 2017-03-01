#include "alib.h"

/* see comment at the head of ax_mbx.c - Bdale */
/* Defines for the ax.25 mailbox facility */

/*defines that control forwarder states */

struct mboxsess{			/*mailbox session structure*/
	pid_t pid;			/*id of bbs process*/
	int spawned;			/*did we create the process?*/
	alEventHandle al_proc_handle;

	int fd;
	alEventHandle al_fd_handle;

	int socket;
	alEventHandle al_socket_handle;

	int port;

	int sendable_count; /* number of bytes that can be enqueued now */
	int byte_cnt; /* number of bytes in buf, waiting to be enqueued */
	struct ax25_cb *axbbscb; /*axp associated with this structure*/
	struct ax25_addr *orig;
	char buf[1024];
	char call[7];
	char command[10][80];
	char cmd_cnt;
	char cmd_state;
	struct mboxsess *next;	/*pointer to next session*/
};

#define NULLMBS  (struct mboxsess *)0
#define NULLFWD  (struct ax25_cb *)0

extern struct mboxsess
	*base;

int ax_control_init(char *c_bindaddr, int c_port);

void mbx_state(struct ax25_cb *axp, int old, int new);
void mbx_incom(struct ax25_cb *axp, int cnt);
