CC = gcc
TARGET = artifactor
FLAGS = -Wall $(shell sdl2-config --cflags)
LINK_FLAGS = $(shell sdl2-config --libs) $(FLAGS) -lm -lSDL2_image -lSDL2_ttf
OBJECTS = $(patsubst %.c, %.o, $(shell find . -name "*.c"))

.SILENT:

all: $(TARGET)
	./$(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) -o $@ $(OBJECTS) $(LINK_FLAGS)

%.o: %.c
	$(CC) $(FLAGS) -c $^ -o $@

clean:
	rm $(TARGET) $(OBJECTS)
