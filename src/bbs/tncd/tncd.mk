# Get the directory in which this Makefile fragment resides.
TNCD_APP_SRCDIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
TNCD_APP_OBJDIR := $(BBS_OBJDIR)/tncd

TNCD_APP_PROD := b_tncd

TNCD_APP_SRCS := ax25.c ax25dump.c ax25subr.c ax25user.c ax_mbx.c\
		bsd_io.c\
		kiss.c\
		lapb.c lapbtime.c\
		main.c mbuf.c monitor.c\
		slip.c socket.c\
		timer.c

$(eval $(call create_bbs_app_rules,TNCD_APP))

BBS_PRODUCTS += $(TNCD_APP)
BBS_CLEAN_TARGETS += TNCD_APP_CLEAN