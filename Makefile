CC = clang
CFLAGS = -std=c99 -Wall -Wextra
EXEC_NAME = gb
SOURCES = src/main.c src/emulator.c src/cpu.c src/bus.c src/cartridge.c src/window.c
INCLUDE = -Iinclude
LINK = -lSDL2

all:
	${CC} ${SOURCES} ${INCLUDE} ${LINK} ${CFLAGS} -o ${EXEC_NAME}

clean:
	rm -f ${EXEC_NAME}