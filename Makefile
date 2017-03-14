#/***************************************************************
#*  Makefile                                        #######     *
#*  chaos-eng(c) int13h 2005-2008                   #.....#     *
#*                                                  #..@..#     *
#*  Build source tree                               #.....#     *
#*  REQUIRES gnu make!                              #######     *
#*                                                              *
#***************************************************************/

ARCH = $(shell uname)
LIBS := -lm
OPTIMISE = -g -Wall
CFLAGS := $(OPTIMISE)
CC = cc

#
# Options
#
WITH_NCURSES = YES
WITH_X11 = YES
WITH_HILDON = NO
WITH_NET = YES
WITH_POSIXTIMER = NO
WITH_NEWLOS = NO

# Irix/Irix64
ifneq (,$(findstring IRIX,$(ARCH)))
   CC = c99
   CFLAGS += -I/usr/local/include
   LDFLAGS += -L/usr/local/lib
   WITH_POSIXTIMER = YES
endif

# Solaris
ifneq (,$(findstring SunOS,$(ARCH)))
   WITH_X11 = NO
   WITH_POSIXTIMER = NO
   LIBS += -lsocket -lnsl
endif

objects = main.o \
	log.o \
	history.o \
	chat.o \
	timer.o \
	chaos.o \
	generate.o \
	tree.o \
	wall.o \
	blob.o \
	magic_special.o \
	magic_upgrade.o \
	magic_balance.o \
	magic_spell_attrib.o \
	magic_ranged.o \
	input_common.o \
	display_common.o \
	sound_common.o
	
#
# X11 Front End
#
ifeq ($(WITH_X11),YES)
   LIBS += -lX11
   CFLAGS += -I/usr/X11R6/include -DWITH_X11
   LDFLAGS += -L/usr/X11R6/lib
   objects += input_x11.o \
	display_x11.o \
	sound_x11.o
endif

#
# NCurses Front End
#
ifeq ($(WITH_NCURSES),YES)
   LIBS += -lncurses
   CFLAGS += -DWITH_NCURSES
   objects += input_ncurses.o \
	display_ncurses.o \
	sound_ncurses.o
endif

#
# Hildon Front End
#
ifeq ($(WITH_HILDON),YES)
   LDFLAGS += $(shell pkg-config --libs hildon-1)
   CFLAGS += -DWITH_HILDON $(shell pkg-config --cflags hildon-1)
   objects += input_hildon.o \
	display_hildon.o \
	sound_hildon.o
endif

#
# Enable Networking
#
ifeq ($(WITH_NET),YES)
   CFLAGS += -DWITH_NET
   objects += net.o \
   	net_client.o \
	net_server.o \
	input_net.o
endif

#
# Timer Options
#
ifeq ($(WITH_POSIXTIMER),YES)
   CFLAGS += -DPOSIXTIMER
endif

#
# Experimental Options
#
ifeq ($(WITH_NEWLOS),YES)
   CFLAGS += -DNEWLOS
endif

all:	$(objects)
	$(CC) $(objects) -o chaos $(CFLAGS) $(LDFLAGS) $(LIBS)
	@echo "Built Chaos-eng for..." $(ARCH)

main.o: 		main.c
	$(CC) $(CFLAGS) -c main.c

log.o:			log.c
	$(CC) $(CFLAGS) -c log.c

history.o:		history.c
	$(CC) $(CFLAGS) -c history.c

chat.o:			chat.c
	$(CC) $(CFLAGS) -c chat.c

timer.o:		timer.c
	$(CC) $(CFLAGS) -c timer.c

chaos.o:		chaos.c
	$(CC) $(CFLAGS) -c chaos.c
	
generate.o:		generate.c
	$(CC) $(CFLAGS) -c generate.c

tree.o:			tree.c
	$(CC) $(CFLAGS) -c tree.c

wall.o:			wall.c
	$(CC) $(CFLAGS) -c wall.c
	
blob.o:			blob.c
	$(CC) $(CFLAGS) -c blob.c
	
magic_special.o:	magic_special.c
	$(CC) $(CFLAGS) -c magic_special.c
	
magic_upgrade.o:	magic_upgrade.c
	$(CC) $(CFLAGS) -c magic_upgrade.c
	
magic_balance.o:	magic_balance.c
	$(CC) $(CFLAGS) -c magic_balance.c

magic_spell_attrib.o:	magic_spell_attrib.c
	$(CC) $(CFLAGS) -c magic_spell_attrib.c
	
magic_ranged.o: 	magic_ranged.c
	$(CC) $(CFLAGS) -c magic_ranged.c
	
input_common.o:		input_common.c
	$(CC) $(CFLAGS) -c input_common.c

input_x11.o:		input_x11.c
	$(CC) $(CFLAGS) -c input_x11.c	

input_ncurses.o:	input_ncurses.c
	$(CC) $(CFLAGS) -c input_ncurses.c	

input_hildon.o:		input_hildon.c
	$(CC) $(CFLAGS) -c input_hildon.c	

display_common.o:	display_common.c
	$(CC) $(CFLAGS) -c display_common.c

display_x11.o:		display_x11.c
	$(CC) $(CFLAGS) -c display_x11.c

display_ncurses.o:	display_ncurses.c
	$(CC) $(CFLAGS) -c display_ncurses.c

display_hildon.o:	display_hildon.c
	$(CC) $(CFLAGS) -c display_hildon.c

sound_common.o: 	sound_common.c
	$(CC) $(CFLAGS) -c sound_common.c

sound_x11.o:		sound_x11.c
	$(CC) $(CFLAGS) -c sound_x11.c

sound_ncurses.o:	sound_ncurses.c
	$(CC) $(CFLAGS) -c sound_ncurses.c

sound_hildon.o:		sound_hildon.c
	$(CC) $(CFLAGS) -c sound_hildon.c

net.o:			net.c
	$(CC) $(CFLAGS) -c net.c

net_client.o:		net_client.c
	$(CC) $(CFLAGS) -c net_client.c
	
net_server.o:		net_server.c
	$(CC) $(CFLAGS) -c net_server.c

input_net.o:		input_net.c
	$(CC) $(CFLAGS) -c input_net.c

clean:
	@echo Cleaning object files
	-rm *.o
