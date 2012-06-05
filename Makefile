PREFIX = /usr/local

csrc = $(wildcard src/*.c)
ccsrc = $(wildcard src/*.cc)
obj = $(ccsrc:.cc=.o) $(csrc:.c=.o)
depfiles = $(obj:.o=.d)
bin = sray

opt = -O3 -ffast-math
warn = -Wall -Wno-strict-aliasing
dbg = -g
#prof = -pg

CFLAGS = -pedantic $(warn) $(dbg) $(opt) $(prof) $(def)
CXXFLAGS = -pedantic $(warn) -Wno-deprecated $(dbg) $(opt) $(prof) $(def)
LDFLAGS = $(prof) -lm -lpthread -lexpat -lkdtree -lrt -lvmath -limago

# XXX work-arround for a bug in my cache manager implementation which
# won't compile properly with thread-specific data on MacOSX at the moment.
ifeq ($(shell uname -s), Darwin)
	def = -DNO_THREADS
endif

$(bin): $(obj)
	$(CXX) -o $@ $(obj) $(LDFLAGS)

-include $(depfiles)

%.d: %.cc
	@$(CPP) $(CXXFLAGS) -MM -MT $(@:.d=.o) $< >$@

%.d: %.c
	@$(CPP) $(CFLAGS) -MM -MT $(@:.d=.o) $< >$@

.PHONY: clean
clean:
	rm -f $(obj) $(bin)

.PHONY: cleandep
cleandep:
	rm -f $(depfiles)

.PHONY: install
install:
	install -m 775 $(bin) $(PREFIX)/bin/$(bin)

.PHONY: uninstall
uninstall:
	rm -f $(PREFIX)/bin/$(bin)
