SRCDIR = src
INCDIR = include
LIBDIR = lib
OBJDIR = lib/obj
BUILDDIR = build
DEBUGDIR = debug

INCS = $(INCDIR)/server.hpp $(INCDIR)/client.hpp $(INCDIR)/utils.hpp

CPPC = g++
OBJS = $(OBJDIR)/utils.o
LIBFLAGS = -lcrypto
CPPFLAGS = -std=c++20 -Wall -I$(INCDIR) -L$(LIBDIR) $(LIBFLAGS)
DBGFLAGS = -g

.PHONY: all clean

all: $(BUILDDIR)/server $(BUILDDIR)/client

clean:
	rm $(BUILDDIR)/server $(BUILDDIR)/client $(OBJS)

$(BUILDDIR)/server: $(SRCDIR)/server.cpp $(OBJS) $(INCS)
	$(CPPC) $(CPPFLAGS) $< $(OBJS) -o $@

$(BUILDDIR)/client: $(SRCDIR)/client.cpp $(OBJS) $(INCS)
	$(CPPC) $(CPPFLAGS) $< $(OBJS) -o $@

$(OBJDIR)/utils.o: $(SRCDIR)/utils.cpp $(INCS)
	$(CPPC) $(CPPFLAGS) $< -c -o $@
