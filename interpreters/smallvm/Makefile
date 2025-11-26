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
PRIORITY = $(CONFIG_INTERPRETERS_SMALLVM_PRIORITY)
STACKSIZE = $(CONFIG_INTERPRETERS_SMALLVM_STACKSIZE)
MODULE = $(CONFIG_INTERPRETERS_SMALLVM)

include $(APPDIR)/Application.mk
