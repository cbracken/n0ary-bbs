# root of source tree
SRC_ROOT = #SRC_ROOT#

# Complier choice
#CC = acc
#ANSI = -Xt
DBUG = -O
CC = gcc
ANSI = -ansi
AR = ar
LD = ld

ENVIRONMENT = -D_BSD_SOURCE -DSUNOS -DDECTALK -DDTMF
INCLUDE = $(SRC_ROOT)/include
LIB = $(SRC_ROOT)/lib

INC = $(LOCAL_INC) -I$(INCLUDE)
LIBS = -L$(SRC_ROOT)/lib $(LOCAL_LIBS) -ltools
CFLAGS = $(DBUG) $(XTYPE) $(INC) $(ANSI) $(ENVIRONMENT)
