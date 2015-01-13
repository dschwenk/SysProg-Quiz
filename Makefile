# Makefile für das Multiplayer-Quiz
# Stefan Gast, 2013, 2014
###############################################################################

#################
# Konfiguration #
#################

OUTPUT_TARGETS = bin/server bin/client bin/loader
DIALECT_OPTS = -Wall -std=gnu99

ARCH = $(shell uname -m)
CCARCH_32 = -march=i686
CCARCH_64 =

ifndef CFLAGS
CFLAGS = -pipe -ggdb
ifeq ($(ARCH),i686)
CFLAGS += $(CCARCH_32)
else
ifeq ($(ARCH),x86_64)
CFLAGS += $(CCARCH_64)
endif
endif
endif

LIBQUIZGUI = client/gui/libquizgui-$(ARCH).a
GTK_LIBS = `pkg-config --libs gtk+-2.0` `pkg-config --libs gthread-2.0`

CLEANFILES = server/*.o server/*.dep \
	     client/*.o client/*.dep \
	     loader/*.o loader/*.dep \
	     common/*.o common/*.dep
MRPROPERFILES = $(OUTPUT_TARGETS)

###############################################################################

################################################
# Module der Programme Server, Client und Loader
################################################

SERVER_MODULES=server/catalog.o \
	       server/clientthread.o \
	       server/login.o \
	       server/main.o \
	       server/score.o \
	       server/user.o \
	       common/sockets.o \
	       common/networking.o \
	       common/rfc.o \
	       common/util.o

CLIENT_MODULES=client/fragewechsel.o \
	       client/listener.o \
	       client/main.o \
	       common/sockets.o \
	       common/networking.o \
	       common/rfc.o \
	       common/util.o

LOADER_MODULES=loader/browse.o \
	       loader/load.o \
	       loader/main.o \
	       loader/parser.o \
	       loader/util.o \
	       common/util.o

###############################################################################

########################################
# Target zum Kompilieren aller Programme
########################################

.PHONY: all
all: $(OUTPUT_TARGETS)

###############################################################################

########################################################
# Includes für eventuell generierte Abhängigkeiten + GUI
########################################################

-include client/gui/gui.mk
-include $(SERVER_MODULES:.o=.dep)
-include $(CLIENT_MODULES:.o=.dep)
-include $(LOADER_MODULES:.o=.dep)

###############################################################################

#######################
# Targets zum Aufräumen
#######################

.PHONY: clean
clean:
	@for i in $(CLEANFILES) ; do \
		[ -f "$$i" ] && rm -v "$$i" ;\
	done ;\
	exit 0

.PHONY: mrproper
mrproper: clean
	@for i in $(MRPROPERFILES) ; do \
		[ -f "$$i" ] && rm -v "$$i" ;\
	done ;\
	exit 0

###############################################################################

##################################################
# Targets zum Linken von Server, Client und Loader
##################################################

bin/server: $(SERVER_MODULES)
	$(CC) -pthread -o $@ $^ -lrt

bin/client: $(CLIENT_MODULES) $(LIBQUIZGUI)
	$(CC) -pthread -o $@ $^ -lrt $(GTK_LIBS)

bin/loader: $(LOADER_MODULES)
	$(CC) -pthread -o $@ $^ -lrt

###############################################################################

###########################################################
# Kompilieren der Module und Erzeugen der Abhängigkeiten
# (siehe http://scottmcpeak.com/autodepend/autodepend.html)
###########################################################

%.o: %.c
	$(CC) -c -I. $(CFLAGS) $(DIALECT_OPTS) -pthread -o $@ $<
	@$(CC) -MM -I. $(CFLAGS) -pthread $< > $*.dep
	@mv -f $*.dep $*.dep.tmp
	@sed -e 's|.*:|$*.o:|' < $*.dep.tmp > $*.dep
	@sed -e 's/.*://' -e 's/\\$$//' < $*.dep.tmp | fmt -1 | \
	  sed -e 's/^ *//' -e 's/$$/:/' >> $*.dep
	@rm -f $*.dep.tmp
