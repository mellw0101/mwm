FASTFLAGS = -O3	 			\
			-march=native

CFLAGS =	-std=c++20 						\
			-pedantic 						\
			-Wall 							\
			-Wno-deprecated-declarations	\
			${FASTFLAGS}

CXXFLAGS = ${CFLAGS}

LIBS = 	-lxcb 			\
		-lxcb-keysyms	\
		-lxcb-cursor 	\
		-lxcb-icccm 	\
		-lxcb-ewmh 		\
		-lpng 			\
		-lxcb-image 	\
		-lxcb-render 	\
		-lxcb-composite \
		-lxcb-xinerama 	\
		-lxcb-xkb 		\
		-lxcb-xfixes 	\
		-lxcb-shape 	\
		-lxcb-randr     \
		-lImlib2		\
		-lXau			\
		-lpthread		\
		-liw

LDFLAGS = 	${LIBS} 		\
			-flto 			\
			-O3 			\
			-march=native 	\
			-std=c++20		


cc = clang
CXX = clang++

SRC = 	src/main.cpp

OBJ = $(SRC:../src/%.cpp=%.o)
DEPS = $(OBJ:.o=.d)

all: test

%.o: %.cpp
	${CXX} -c ${CXXFLAGS} $< -o $@ -MMD -MP


test: $(OBJ)
	${CXX} -o $@ $^ ${LDFLAGS}

clean:

backup:

depends:
	sudo chmod u+x tools/install_depends.sh
	./tools/install_depends.sh

dist: clean

install: all
	mkdir	-p 							/bin
	cp		-f 	test 					/bin/mwm
	chmod	755 						/bin/mwm
	
uninstall:

.PHONY: all depends clean dist install uninstall
