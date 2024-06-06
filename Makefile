EXENAME = awtp

OBJS= wtp.o

ALL=awtp

all: awtp

#$(OBJDIR)/%.o:%.c

.c.o:
	$(CC) -c $(R_CFLAGS) $<

clean: 
	rm -f *.o
	rm -f awtp
