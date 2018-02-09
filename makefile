TARGET = libtcpsessionmgr.a
VPATH = .
INC_DIR = 
CC := g++
CFLAGS := -Wall -g -std=c++0x -DENABLE_SSL $(INC_DIR)

SRCS := $(wildcard *.cpp)
basename_srcs := $(notdir $(SRCS))
basename_objs := $(patsubst %.cpp,%.o,$(basename_srcs) )

%.o : %.cpp
	$(CC) -c -o $@ $(CFLAGS) $<
%.o : %.cc
	$(CC) -c -o $@ $(CFLAGS) $<

$(TARGET):$(basename_objs)
	ar cr $@ $^

clean:
	rm -f $(basename_objs)
	rm -f $(TARGET)
