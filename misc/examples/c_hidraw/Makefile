PROGRAM = hidraw

CFLAGS  += $(shell pkg-config ncurses --cflags)
LDFLAGS += $(shell pkg-config ncurses --libs)

SRC = hidraw.c

OBJ = $(SRC:.c=.o)

.PHONY: all clean

all: $(PROGRAM)

$(PROGRAM): $(OBJ)
	$(CC) $< $(LDFLAGS) -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(PROGRAM) $(OBJ)
