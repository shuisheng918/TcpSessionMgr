TARGET = httpsserver_test
VPATH = .:../../
INC_DIR = -I../../
LIB_DIR = 

RPATH=

CC := g++
CFLAGS := -DENABLE_SSL -Wall -g -std=c++0x $(INC_DIR)
LDFLAG := $(RPATH)  $(LIB_DIR) -Wl,-Bstatic -lswevent -Wl,-Bdynamic -lssl -lcrypto -pthread

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
