TARGET = httpserver_test
VPATH = .:../../
INC_DIR = -I/opt/libswevent/include -I../../
LIB_DIR = -L/opt/libswevent/lib

RPATH=

CC := g++
CFLAGS := -Wall -g -std=c++0x $(INC_DIR)
LDFLAG := $(RPATH)  $(LIB_DIR) -Wl,-Bstatic -lswevent -Wl,-Bdynamic

SRCS := $(wildcard *.cpp ../../*.cpp)
$(warning  $(SRCS))
basename_srcs := $(notdir $(SRCS))
basename_objs := $(patsubst %.cpp,%.o,$(basename_srcs) )

%.o : %.cpp
	$(CC) -c -o $@ $(CFLAGS) $<

$(TARGET):$(basename_objs)
	$(CC) -o $@ $^ $(LDFLAG)

clean:
	rm -f $(basename_objs)
	rm -f $(TARGET)
