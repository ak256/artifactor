CC = gcc
TARGET = artifactor
DEBUG_FLAGS = -g
FLAGS = -Wall -pedantic $(shell sdl2-config --cflags)
LINK_FLAGS = $(shell sdl2-config --libs) $(FLAGS) -lSDL2_image -lSDL2_ttf
OBJECTS = $(patsubst %.c, %.o, $(shell find . -name "*.c"))

.SILENT:

all: $(TARGET)
	./$(TARGET)

debug: $(FLAGS) += $(DEBUG_FLAGS)
debug: $(TARGET)
	gdb --args $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) -o $@ $(OBJECTS) $(LINK_FLAGS)

%.o: %.c
	$(CC) $(FLAGS) -c $^ -o $@

clean:
	rm $(TARGET) $(OBJECTS)
