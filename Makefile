ARMV8_FASTFLAGS = -O2               \
                  -march=armv8-a    \
                  -mtune=-mtune=cortex-a53

ARMV8_CFLAGS =   -std=c++20                     \
                 -pedantic                      \
                 -Wall                          \
                 -Wno-deprecated-declarations   \
				 -DARMV8_BUILD					\
                 ${ARMV8_FASTFLAGS}

ARMV8_CXXFLAGS = ${ARMV8_CFLAGS}

ARMV8_LDFLAGS =  ${LIBS}       	\
                 -flto         	\
                 -O2           	\
                 -std=c++20		\
				 -DARMV8_BUILD

FASTFLAGS = \
	-O3 					\
	-march=native			\
	-funroll-loops			\
	-Rpass=loop-vectorize 	\
	-flto 					\
	-m64

CFLAGS = \
	-std=c++20 						\
	-pedantic 						\
	-Wall 							\
	-Wno-deprecated-declarations	\
	${FASTFLAGS}

CXXFLAGS = ${CFLAGS}

LIBS = \
	-lxcb         	\
	-lxcb-keysyms	\
	-lxcb-cursor 	\
	-lxcb-icccm 	\
	-lxcb-ewmh 		\
	-lxcb-xinput	\
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
	-liw			\
	-lpulse

LDFLAGS = \
	${LIBS} 				\
	-flto 					\
	-O3 					\
	-march=native 			\
	-std=c++20				\
	-funroll-loops			\
	-Rpass=loop-vectorize 	\
	-flto 					\
	-m64

# -std=c++20

cc  = clang
CXX = clang++

SRC = src/main.cpp src/tools.cpp src/xcb.cpp src/prof.cpp

OBJ  = $(SRC:../src/%.cpp=%.o)
DEPS = $(OBJ:.o=.d)

all: test

%.o: %.cpp
	${CXX} -c ${CXXFLAGS} $< -o $@ -MMD -MP

test: $(OBJ)
	${CXX} -o $@ $^ ${LDFLAGS}

clean:
	sudo rm test
	unset CXXFLAGS
	unset LDFLAGS

backup:

depends:
	sudo chmod u+x tools/install_depends.sh
	sudo chmod u+x tools/check_and_install.sh
	./tools/check_and_install.sh

conf:
	sudo chmod u+x tools/configure.sh
	./tools/configure.sh

dist: clean

install: all
	mkdir -p      /bin
	cp    -f test /bin/mwm
	chmod 755     /bin/mwm

install-armv8: ARMV8
	mkdir -p      /bin
	cp    -f test /bin/mwm
	chmod 755     /bin/mwm

ARMV8: CXXFLAGS = ${ARMV8_CXXFLAGS} -DARMV8_BUILD
ARMV8: LDFLAGS = ${ARMV8_LDFLAGS}
ARMV8: test

uninstall:

.PHONY: all depends conf clean dist install uninstall
