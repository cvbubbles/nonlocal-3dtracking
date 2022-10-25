DOPENCV=/usr/local/include/
DCVF=../
DLIBX=/fan/local/Re3D/
DASSIMP=$(DLIBX)/assimp-4.1.0/

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/lib/x86_64-linux-gnu

CFLAGS=-DFINTEGER=int -fPIC -Wall -g -msse4  -O0 
LDFLAGS=-fPIC -Wall -g -msse4  
#LIBS=-l glog -l gflags -l boost_system -lboost_filesystem -l pthread -l glut -L /usr/lib/x86_64-linux-gnu -l:/usr/lib/x86_64-linux-gnu/libOpenGL.so -l:libEGL.so.1.1.0 -l GLU -L $(DASSIMP)/lib/ -l assimp -lX11 -lGLX
LIBS=-l glog -l gflags -l boost_system -lboost_filesystem -l pthread -l glut -lOpenGL -l:libEGL.so.1.1.0 -l GLU -L $(DASSIMP)/lib/ -l assimp -lX11 -lGLX


all: dirs test

DCVR=/fan/dev/cvf-01/cvf/CVRender/
DObjs=./objs/
CVRObjs=$(addprefix $(DObjs), _cvrender.o cvrender.o cvrm.o)

MYObjs=$(addprefix $(DObjs), main.o impl.o RenderExamples.o RenderVOC.o)

dirs:
	mkdir -p $(DObjs)

test: $(CVRObjs) $(MYObjs)
	g++ -o $@ $^  $(shell pkg-config --libs opencv) $(LDFLAGS)  $(LIBS) $(CFLAGS)

$(DObjs)%.o:$(DCVR)/%.cpp
	g++ -c -o $@ $< -I$(DOPENCV) -I$(DCVF) -I$(DASSIMP)/include/ -I$(DLIBX)/glm-0.9.6.3/glm/ -w -fpermissive -std=c++11 $(CFLAGS) 

$(DObjs)%.o:./%.cpp
	g++ -c -o $@ $< -I$(DOPENCV) -I$(DCVF) -w -fpermissive -std=c++11 $(CFLAGS) 

clean:
	rm _cvrender.o cvrender.o cvrm.o test.o dllmain.o

