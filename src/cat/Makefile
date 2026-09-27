CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -pedantic
TARGET = s21_cat
SOURCES = s21_cat.c cat_options.c ../common/error_utils.c

.PHONY: all clean rebuild test

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) $(SOURCES) -o $(TARGET)

clean:
	rm -f $(TARGET)

rebuild: clean all

test: $(TARGET)
	./test_cat.sh
