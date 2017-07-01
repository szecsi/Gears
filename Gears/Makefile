CPP_FILES 	:= $(filter-out src/core/Experiment.cpp src/core/Event.cpp src/video/FrameRenderer.cpp, src/Gears.cpp src/stdafx.cpp $(wildcard src/core/*.cpp) $(wildcard src/event/*.cpp) $(wildcard src/video/*.cpp) $(wildcard src/gpu/*.cpp) $(wildcard src/math/*.cpp) $(wildcard src/fft/*.cpp))
OBJ_FILES 	:= $(addprefix obj/, $(CPP_FILES:src/%.cpp=%.o))
LD_FLAGS 	:= -shared -Wl,--export-dynamic -Wl,--no-undefined
LINK_LIBS	:= -lX11 -lGL -lGLU -lGLEW -lpthread -lboost_python-py34 -lboost_filesystem -lboost_system -lpython3.4m -lavcodec -lavfilter -lavformat -lavdevice -lavutil -lswscale -lIL -lILU
CC_FLAGS 	:= -fPIC --std=c++11
INC		 	:= -I/usr/include/python3.4 -I/usr/include/boost -Isrc -Isrc/core -Isrc/gpu -Isrc/fft -Isrc/font -Isrc/video -I/usr/include/IL -I/usr/include/libavcodec -I../libs/eigen-eigen-07105f7124f9

default: dirs bin/linux/Gears.so

dirs:
	mkdir -p obj/core
	mkdir -p obj/event
	mkdir -p obj/math
	mkdir -p obj/gpu
	mkdir -p obj/video
	mkdir -p obj/fft
	mkdir -p bin/linux

bin/linux/Gears.so: $(OBJ_FILES)
	g++ $(LD_FLAGS) -o $@ $^ $(LINK_LIBS)
	cp bin/linux/Gears.so ../GearsPy/

obj/%.o: src/%.cpp
	g++ $(CC_FLAGS) $(INC) -c -o $@ $<

.PHONY: clean all write

write:
	@echo $(CPP_FILES)
	@echo "========================================="
	@echo $(OBJ_FILES)

clean:
	rm -rf obj/

all: clean default
