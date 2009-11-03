PREFIX = /usr/local

obj = 3ds2sray.o
bin = 3ds2sray

CC = gcc
CFLAGS = -pedantic -Wall -g
LDFLAGS = -l3ds

$(bin): $(obj)
	$(CC) -o $@ $(obj) $(LDFLAGS)

.PHONY: clean
clean:
	rm -f $(obj) $(bin)

.PHONY: install
install:
	cp $(bin) $(PREFIX)/bin/$(bin)

.PHONY: uninstall
uninstall:
	rm -f $(PREFIX)/bin/$(bin)
