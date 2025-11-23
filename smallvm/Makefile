include $(APPDIR)/Make.defs

CFLAGS += -DGNUBLOCKS -DNUTTX -Wno-strict-prototypes -Wno-shadow

CSRCS = dataPrims.c \
    interp.c \
    mem.c \
    miscPrims.c \
    persist.c \
    runtime.c \
    tinyJSON.c \
    varPrims.c

MAINSRC = nuttx.c

PROGNAME = smallvm
PRIORITY = 100
STACKSIZE = $(CONFIG_DEFAULT_TASK_STACKSIZE)
MODULE = $(CONFIG_SMALLVM)

include $(APPDIR)/Application.mk
