# Makefile for ...su/main

include $(CWPROOT)/src/Makefile.config

D = $L/libcwp.a $L/libpar.a $L/libsu.a

B = ../bin


#OPTC =-g

LFLAGS= $(PRELFLAGS) -L$L -lsu -lpar -lcwp -lm $(POSTLFLAGS)


PROGS =	\
	$B/msh2tri


INSTALL	:	$(PROGS)  
	@-rm -f INSTALL
	@touch $@

$(PROGS):	$(CTARGET) $D
	-$(CC) -fopenmp $(CFLAGS) $(@F).c $(LFLAGS) -o $@
	@chmod 755 $@
	@echo $(@F) installed in $B

remake	:
	-rm -f $(PROGS) INSTALL
	$(MAKE)

clean::
	rm -f a.out junk* JUNK* core
