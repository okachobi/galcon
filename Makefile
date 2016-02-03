#
# Please see copyright and contact information in qsniff.c
#

#
# this has to be the simplest makefile I've ever written.
#

CFLAGS=-O2
BINDIR=/usr/local/bin
VER=6.0

all: galcon

galcon: galcon.o

clean:
	rm -f *.o

realclean:
	rm -f galcon DEADJOE core *.o 

install:
	install --owner=root --group=root --mode=0711 --strip galcon $(BINDIR)/galcon
	chmod u+s /usr/local/bin/galcon

dist:
	rm -f galcon-$(VER).tar.gz
	tar zcvf galcon-$(VER).tar.gz galcon.c Makefile 
