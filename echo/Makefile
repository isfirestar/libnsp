TARGET=nstest

build=debug
arch=x86_64

CP=cp
CPPFLAGS=-Wall -std=c++11 -Wno-narrowing
INCLUDES=-I ../ \
		-I ../icom/ \
        -I ./ \

SRCS+=$(wildcard *.cpp) $(wildcard ../*.cpp)
OBJS:=$(patsubst %.cpp,%.o,$(SRCS))

ifeq ($(build),debug)
	CPPFLAGS+=-g3
else
	CPPFLAGS+=-g -O2
endif

ifeq ($(arch),arm)
	CC=arm-linux-gnueabihf-g++
	LDFLAGS:=/usr/local/lib/motion_net.so /usr/local/lib/nshost.so -Wl,-rpath=/usr/local/lib -L /usr/local/lib/
else ifeq ($(arch),arm64)
	CC=aarch64-linux-gnu-g++
	LDFLAGS:=/usr/local/lib/aarch64-linux-gnu/motion_net.so /usr/local/lib/aarch64-linux-gnu/nshost.so -Wl,-rpath=/usr/local/lib/aarch64-linux-gnu/
else
	CC=g++
	LDFLAGS:=/usr/local/lib64/motion_net.so /usr/local/lib64/nshost.so -Wl,-rpath=/usr/local/lib64/
endif

LDFLAGS+=-lm -lpthread -ldl -lrt

$(TARGET):$(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

all:
	$(TARGET)

.cpp.o:
	$(CC) -c $< $(CPPFLAGS) $(INCLUDES) -o $@

.PHONY:clean all install
clean:
	$(RM) $(OBJS) $(TARGET)
