csrc = $(wildcard src/*.c)
ccsrc = $(wildcard src/*.cc)
obj = $(ccsrc:.cc=.o) $(csrc:.c=.o)
depfiles = $(obj:.o=.d)
bin = sray

opt = -O3 -ffast-math
warn = -Wall -Wno-strict-aliasing
dbg = -g
#prof = -pg

CC = gcc
CXX = g++
CFLAGS = -pedantic $(warn) $(dbg) $(opt) $(prof) $(def) `pkg-config --cflags vmath imago`
CXXFLAGS = -pedantic $(warn) -Wno-deprecated $(dbg) $(opt) $(prof) $(def) `pkg-config --cflags vmath imago`
LDFLAGS = $(prof) `pkg-config --libs vmath imago` $(wsys_libs) -lm -lpthread -lexpat -lkdtree

ifeq ($(shell uname -s),MINGW32)
	#fill this up
	wsys_libs = 
else
	wsys_libs = -lX11 -lXext
endif

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
	rm -f $(obj) $(bin) $(depfiles)

.PHONY: cleandep
cleandep:
	rm -f $(depfiles)
