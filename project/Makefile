
EXECUTABLE  = elevator

COMPILER    = clang
EXCLUDE     = '*test*'

STD        	= -std=gnu11 -Wall

# Default mode is debug
MODE ?= rel

ifeq ($(MODE), debug)
    CFLAGS  = $(STD) -g -O0 -fsanitize=address -fsanitize=undefined
    LDFLAGS = -fsanitize=address -fsanitize=undefined
else ifeq ($(MODE), rel)
  CFLAGS  = $(STD) -O3 -DDEBUG -DLOG_USE_COLOR
  # CFLAGS  = $(STD) -O3 -DNDEBUG -DLOG_USE_COLOR
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

## link files the executable depends on
$(EXECUTABLE): $(OBJECTS) 
	$(COMPILER) $^ -o $@ $(LDFLAGS)

## compile the source files which object files depend on
$(OBJECTDIR)/%.o: $(SOURCEDIR)/%.c
	@mkdir -p $(@D)
	$(COMPILER) -o $@ -c $(CFLAGS) $<

run: $(EXECUTABLE)
	./$(EXECUTABLE)
	
.PHONY: all rebuild clean

