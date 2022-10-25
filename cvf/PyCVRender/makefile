DLIBS=/home/aa/libs/
DOPENCV=/usr/local/include
DCONDA=~/anaconda3/

CFLAGS=-DFINTEGER=int -fPIC -fvisibility=hidden -Wall -g -msse4  -I$(DOPENCV) -I../ -w -fpermissive -std=c++11 
LDFLAGS=-fPIC -Wall -g -msse4  
LIBS=-l pthread -lOpenGL -lEGL -lassimp -L $(DLIBS)/bin/ -L /home/aa/anaconda3/lib/ -lpython3.7m -lstdc++fs -lGLU

PyFLAGS=-I $(DCONDA)/include/python3.7m -I $(DCONDA)/lib/python3.7/site-packages/numpy/core/include/ -I $(DLIBS)/pybind11/include -I $(DLIBS)/assimp-4.1.0/include -I $(DLIBS)/glm0963/include

all: dirs cvrender


DObjs=./objs/
Objs=$(addprefix $(DObjs), bind.o impl.o my_converter.o RenderDataset.o src_CVRender.o)

dirs:
	mkdir -p $(DObjs)

cvrender: $(Objs)
	g++ -shared $^  $(LDFLAGS)  $(LIBS) -L ./ $(shell pkg-config --libs opencv)  -o ../Python/cvf/cvrender.so 


$(DObjs)%.o:./%.cpp
	g++ -static -c -o $@ $<  $(CFLAGS) $(PyFLAGS) -fPIC

