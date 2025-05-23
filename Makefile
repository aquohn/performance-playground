SRCDIR = src
INCDIR = include
LIBDIR = lib
OBJDIR = lib/obj
BUILDDIR = build
DEBUGDIR = debug

INCS = $(INCDIR)/server.hpp $(INCDIR)/client.hpp $(INCDIR)/utils.hpp $(INCDIR)/loop.hpp $(INCDIR)/cache.hpp $(INCDIR)/backend.hpp

CPPC = g++
BASEOBJS = $(OBJDIR)/utils.o $(OBJDIR)/backend.o
OBJS = $(BASEOBJS) $(OBJDIR)/loop.o
ifeq ($(DEBUG), 1)
	DBGFLAGS = -g -DDEBUG=1
else
	DBGFLAGS =
endif

INCFLAGS = -I.
LIBFLAGS = -lcrypto
CPPFLAGS = -std=c++20 -Wall -I$(INCDIR) $(INCFLAGS) -L$(LIBDIR) $(LIBFLAGS) $(DBGFLAGS) -fdiagnostics-all-candidates

.PHONY: all clean

all: $(BUILDDIR)/server $(BUILDDIR)/client

clean:
	rm $(BUILDDIR)/server $(BUILDDIR)/client $(OBJS)

$(BUILDDIR)/%: $(SRCDIR)/%.cpp $(OBJS) $(INCS)
	$(CPPC) $(CPPFLAGS) $< $(OBJS) -o $@

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp $(INCS)
	$(CPPC) $(CPPFLAGS) $< -c -o $@
