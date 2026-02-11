
EXECUTABLE  = elevator

COMPILER    = clang
EXCLUDE     = '*test*'

STD        	= -std=gnu11 -Wall

# Default mode is debug
MODE ?= debug

ifeq ($(MODE), debug)
    CFLAGS  = $(STD) -g -O0 -fsanitize=address -fsanitize=undefined
    LDFLAGS = -fsanitize=address -fsanitize=undefined
else
    CFLAGS  = $(STD) -O3 -DNDEBUG
    LDFLAGS = 
endif

SOURCEDIR = src
OBJECTDIR = build

SOURCES := $(patsubst $(SOURCEDIR)/%, %, $(shell find $(SOURCEDIR) -name '*.c'  ! -name $(EXCLUDE)  ! -path $(EXCLUDE)))
OBJECTS = $(addprefix $(OBJECTDIR)/, $(SOURCES:.c=.o))

all: $(EXECUTABLE) 

rebuild: clean all

clean:
	rm -f $(EXECUTABLE)
	rm -rf $(OBJECTDIR)

## executable depends on object files: link them
$(EXECUTABLE): $(OBJECTS) 
	$(COMPILER) $^ -o $@ $(LDFLAGS)

## object files depend on source files: compile them
$(OBJECTDIR)/%.o: $(SOURCEDIR)/%.c
	@mkdir -p $(@D)
	$(COMPILER) -o $@ -c $(CFLAGS) $<

	
.PHONY: all rebuild clean

