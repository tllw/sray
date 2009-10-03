PREFIX = /usr/local

obj = obj2sray.o
bin = obj2sray

CXX = g++
CXXFLAGS = -pedantic -Wall -g `pkg-config --cflags henge`
LDFLAGS = `pkg-config --libs henge`

$(bin): $(obj)
	$(CXX) -o $@ $(obj) $(LDFLAGS)


.PHONY: clean
clean:
	rm -f $(obj) $(bin)

.PHONY: install
install:
	cp $(bin) $(PREFIX)/bin/$(bin)

.PHONY: uninstall
uninstall:
	rm -f $(PREFIX)/bin/$(bin)
