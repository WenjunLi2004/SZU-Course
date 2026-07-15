CC := gcc
CFLAGS := -std=c11 -Wall -Wextra -O2 -pthread
TARGET := sfs

.PHONY: all clean run

all: $(TARGET)

$(TARGET): sfs.c
	$(CC) $(CFLAGS) sfs.c -o $(TARGET)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET) demo_output.txt
