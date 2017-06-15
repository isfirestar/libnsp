TARGET=libnsp.a
CFLAGS+=-Wall -I ../icom
LDFLAGS=

ifeq ($(DEBUG_SYMBOLS),TRUE)
	CFLAGS+=-g
else
	CFLAGS+=-o3
endif

SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)

all:$(TARGET)

$(TARGET):$(OBJS)
	$(AR) rcs $@ $^
%.d:%.c
	$(CC) $(CFLAGS) -c $< -o $@ 
clean:
	$(RM) $(OBJS) ../$(TARGET)

.PHONY:clean all
