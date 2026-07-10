CC = gcc
CFLAGS = -Wall -Wextra

TARGETS = write_app read_app

all: $(TARGETS)

write_app: write_app.c
	$(CC) $(CFLAGS) -o $@ $^

read_app: read_app.c
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(TARGETS)
