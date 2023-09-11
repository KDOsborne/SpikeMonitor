CC=gcc
CFLAGS=-L./libs -I./include -Wall
BUILDDIR=build
BINDIR=$(BUILDDIR)/bin
OBJDIR=$(BUILDDIR)/objs
CSOURCEFILE=main.c
OBJECTS=$(addprefix $(OBJDIR)/, $(addsuffix .o, $(basename $(notdir $(wildcard src/*.c)))))
LIBS=-lbass -lopengl32 -lGdi32 -lcomdlg32

OUTFILE=$(BINDIR)/spikemonitor.exe

.PHONY: clean

all: dirs spikemonitor

spikemonitor: $(OBJECTS) $(CSOURCEFILE)
	$(CC) $(CSOURCEFILE) $(OBJECTS) -o $(OUTFILE) $(CFLAGS) $(LIBS)
	
$(OBJDIR)/%.o: src/%.c include/%.h
	$(CC) -c $(filter %.c, $^) -o $@ $(CFLAGS) 
	
dirs: | $(BUILDDIR) $(BINDIR) $(OBJDIR)

$(BUILDDIR):
	mkdir $(BUILDDIR)
	
$(BINDIR):
	mkdir $(BINDIR)
	
$(OBJDIR):
	mkdir $(OBJDIR)
	
clean:
	-rm $(OBJECTS)
	-rm $(OUTFILE)