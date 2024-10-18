CC = gcc
CFLAGS = -Wall -Werror -Wextra -pedantic
TARGET = hnm
SOURCES = hnm.c

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCES)

clean:
	rm -f $(TARGET)
