CC = clang
CFLAGS = -std=c99 -Wall -Wextra
EXEC_NAME = gb
SOURCES = src/main.c src/emulator.c src/cpu.c src/memory.c
INCLUDE = -Iinclude

all:
	${CC} ${SOURCES} ${INCLUDE} ${CFLAGS} -o ${EXEC_NAME}

clean:
	rm -f ${EXEC_NAME}