CC = gcc
CFLAGS = -std=c99 

# Source files for clox
SOURCES = main.c \
          chunk.c \
          memory.c \
          debug.c \
          value.c \
          vm.c \
          compiler.c \
          scanner.c \
          object.c \
          table.c

# Header files dependencies for clox
HEADERS = common.h \
          chunk.h \
          memory.h \
          debug.h \
          value.h \
          vm.h \
          compiler.h \
          scanner.h \
          object.h \
          table.h

# Name of the output executable
OUTPUT = clox

$(OUTPUT): $(SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) -o $(OUTPUT) $(SOURCES)

# 'make clean' command to remove the compiled executable
clean:
	rm -f $(OUTPUT)
