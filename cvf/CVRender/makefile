DLIBS=/home/aa/libs/
DOPENCV=/usr/local/include
DCONDA=~/anaconda3/

CFLAGS=-DFINTEGER=int -fPIC -Wall -g -msse4  -I$(DOPENCV) -I../ -w -fpermissive -std=c++11 
LDFLAGS=-fPIC -Wall -g -msse4  
LIBS=-l pthread -lOpenGL -lEGL -lassimp -L $(DLIBS)/bin/ -L /home/aa/anaconda3/lib/ -lpython3.7m -lstdc++fs -lGLU

CFLAGSX= $(CFLAGS) -I $(DLIBS)/assimp-4.1.0/include -I $(DLIBS)/glm0963/include

all: dirs cvrender


DObjs=./objs/
Objs=$(addprefix $(DObjs), dllmain.o  cvrender.o cvrm.o _cvrender.o)

dirs:
	mkdir -p $(DObjs)

cvrender: $(Objs)
	g++ -shared $^  $(LDFLAGS)  $(LIBS) -L ./ $(shell pkg-config --libs opencv)  -o ../../bin/libcvrender.so 


$(DObjs)%.o:./%.cpp
	g++ -static -c -o $@ $<  $(CFLAGSX) -fPIC

