EXENAME = awtp

OBJS= wtp.o

ALL=awtp

all: awtp

#$(OBJDIR)/%.o:%.c

.c.o:
#	@echo "  $(CC) $(CFLAGS) "$<
#	@echo "  mipsel-openwrt-linux-musl-gcc $(CFLAGS) "$<
	@$(CC) $(INCL_DIRS) -g -c $(CFLAGS) $< -o $@
#	mipsel-openwrt-linux-musl-gcc $(INCL_DIRS) -c $(CFLAGS) $< -o $@
#	arm-openwrt-linux-muslgnueabi-gcc $(INCL_DIRS) -c $(CFLAGS) $< -o $@
#	gcc $(INCL_DIRS) -c $(CFLAGS) $< -o $@

awtp: $(OBJS)
#	echo "  $(CC) $(LDFLAGS) $(LIBS) $(EXENAME)"
	$(CC) $(LDFLAGS) -o $(EXENAME) $(OBJS)
#	$(CC) $(LDFLAGS) -o $(EXENAME) $(OBJS) $(LIBS) $(STATICLIBS)
#	echo "  mipsel-openwrt-linux-musl-gcc $(LDFLAGS) $(LIBS) $(EXENAME)"
#	mipsel-openwrt-linux-musl-gcc $(LDFLAGS) -o $(EXENAME) $(OBJS) $(LIBS) $(STATICLIBS)
#	arm-openwrt-linux-muslgnueabi-gcc $(LDFLAGS) -o $(EXENAME) $(OBJS) $(LIBS) $(STATICLIBS)
#	gcc $(LDFLAGS) -o $(EXENAME) $(OBJS) $(LIBS) $(STATICLIBS)

clean: 
	rm -f *.o
	rm -f awtp
