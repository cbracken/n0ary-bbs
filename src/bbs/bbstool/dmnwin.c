#include <stdio.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xm/Xm.h>
#include <Xm/List.h>

#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"
#include "bbstool.h"

Widget dmn_parent, dmn_list;

void
dmn_callback(Widget widget, caddr_t client_data, caddr_t call_data)
{
	static Position x = 10;
	static Position y = 690;
	Arg args[10];
	Cardinal n = 0;

	if(XtIsRealized(dmn_parent)) {
		XtSetArg(args[n], XtNx, &x); n++;
		XtSetArg(args[n], XtNy, &y); n++;
		XtGetValues(dmn_parent, args, n);
		XtUnrealizeWidget(dmn_parent);
		x-=5;
		y-=26;
	} else {
		XtSetArg(args[n], XtNx, x); n++;
		XtSetArg(args[n], XtNy, y); n++;
		XtSetValues(dmn_parent, args, n);
		XtRealizeWidget(dmn_parent);
	}
}

dmn_display()
{
	struct my_active_procs *ap = ActiveProcs;
	XmStringTable str_list;
	Arg args[4];
	Cardinal n = 0;
	int i,j, cnt = 0;

	XmListDeleteAllItems( dmn_list); 

	while(ap) {
		if(ap->window == DMNWIN)
			cnt++;
		NEXT(ap);
	}
	ap = ActiveProcs;

	str_list = (XmStringTable)XtMalloc(cnt * sizeof(XmString *));
	for(i=0,j=0; j<cnt; i++) {
		if(ap->window == DMNWIN)
			str_list[j++] = XmStringCreateSimple(ap->display);
		NEXT(ap);
	}

	XtSetArg(args[n], XmNitems, str_list); n++;
	XtSetArg(args[n], XmNitemCount, cnt); n++;
	XtSetValues(dmn_list, args, n);

	for(i=0; i<cnt; i++)
		XmStringFree(str_list[i]);
	XtFree(str_list);
}

void
dmn_create_window(void)
{
	XmStringTable str_list;
	int i;
	Arg args[10];
	Cardinal n = 0;
	char title[80];

	sprintf(title, "Daemon List @ %s", Bbs_Host);

    n = 0;
	XtSetArg(args[n], XmNmwmDecorations, Decoration); n++;
	XtSetArg(args[n], XmNtitle, title); n++;
	dmn_parent = XtCreateApplicationShell("Daemons",
		topLevelShellWidgetClass, args, n);

	XtSetArg(args[n], XmNwidth, 360); n++;
	XtSetArg(args[n], XmNheight, 160); n++;
	dmn_list = XmCreateScrolledList(dmn_parent, "Daemons", args, n);

	XtAddCallback(dmn_list, XmNdefaultActionCallback, proc_connect, NULL);
	XtManageChild(dmn_list);
}
