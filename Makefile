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
		-lpthread		

LDFLAGS = 	${LIBS} 		\
			-flto 			\
			-O3 			\
			-march=native 	\
			-std=c++20		


cc = clang
CXX = clang++

SRC = 	src/main.cpp 	\
		src/tools.cpp	\
		src/mxb.cpp

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
	mkdir 		-p 							/bin
	cp 			-f 	conf/MGaming 			/bin
	cp 			-f 	conf/mwm-X 				/bin
	cp 			-f 	conf/mwm-KILL 			/bin
	cp 			-f 	test 					/bin/mwm

	chmod 		755 						/bin/MGaming
	chmod 		755 						/bin/mwm-X
	chmod 		755 						/bin/mwm-KILL
	chmod 		755 						/bin/mwm

	mkdir 		-p 							/etc/systemd/system
	cp 			-f 	conf/mwm.service 		/etc/systemd/system/mwm.service
	cp 			-f 	conf/mwm-X.service 		/etc/systemd/system/mwm-X.service

	mkdir 		-p 							/etc/polkit-1/rules.d
	cp 			-f 	conf/mwm.rules 			/etc/polkit-1/rules.d/mwm.rules

	systemctl 	restart 					polkit
	systemctl 	daemon-reload

uninstall:

.PHONY: all depends clean dist install uninstall
