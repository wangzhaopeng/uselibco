
VIDEO_LIB = -L./video/lib -lmp4v2 -lvpu -lpthread -ljpeg
VIDEO_INC = -I./video -I./video/include 
VIDEO_SRC = 
VIDEO_OBJ = 

INCLUDE = -I/home/hadoop/libco

LDFLAGS += -L/home/hadoop/libco/lib -lcolib -lpthread -ldl

%.o:%.cpp
	g++ -std=c++11 -c -g -o $@ $< $(INCLUDE)

all:svr

svr:main.o link_cls.o thread_cls.o
	g++ -std=c++11 -o $@ $^ $(LDFLAGS)


clean:
	rm svr *.o

