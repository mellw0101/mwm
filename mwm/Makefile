FASTFLAGS = \
-O3 -march=native

CFLAGS = \
-std=c++20 -pedantic -Wall -Wno-deprecated-declarations ${FASTFLAGS}
CXXFLAGS = \
${CFLAGS}

LIBS = -lxcb -lxcb-keysyms -lxcb-cursor -lxcb-icccm -lxcb-ewmh
LDFLAGS  = ${LIBS} -flto -O3 -march=native
# -s

cc = clang
CXX = clang++

SRC = main.cpp tools.cpp
OBJ = $(SRC:../src/%.cpp=%.o)
DEPS = $(OBJ:.o=.d)

all: test

%.o: %.cpp
	${CXX} -c ${CXXFLAGS} $< -o $@ -MMD -MP


test: $(OBJ)
	${CXX} -o $@ $^ ${LDFLAGS}

clean:

backup:

dist: clean

install: all
	mkdir 	-p 						/bin
	cp 		-f 	conf/MGaming 			/bin
	cp 		-f 	conf/mwm-X 			/bin
	cp 		-f 	conf/mwm-KILL 			/bin
	cp 		-f 	test 				/bin/mwm

	chmod 		755 					/bin/MGaming
	chmod 		755 					/bin/mwm-X
	chmod 		755 					/bin/mwm-KILL
	chmod 		755 					/bin/mwm

	mkdir 	-p 						/etc/systemd/system
	cp 		-f 	conf/mwm.service 		/etc/systemd/system/mwm.service
	cp 		-f 	conf/mwm-X.service 		/etc/systemd/system/mwm-X.service

	mkdir 	-p 						/etc/polkit-1/rules.d
	cp 		-f 	conf/mwm.rules 			/etc/polkit-1/rules.d/mwm.rules

	systemctl 	restart 				polkit
	systemctl 	daemon-reload

uninstall:

.PHONY: all clean dist install uninstall
