PROJDIRS := src/ obj/
SRCFILES := $(shell find $(PROJDIRS) -type f -name "*.cpp")
HDRFILES := $(shell find $(PROJDIRS) -type f -name "*.h")
CODEFILES := $(SRCFILES) $(HDRFILES)
OBJFILES := $(patsubst src/%.cpp,obj/%.o,$(SRCFILES))
GDBMISRCFILES := $(shell find src/gdbmi/ -type f -name "*.cpp")
GDBMIOBJFILES := $(patsubst src/gdbmi/%.cpp,obj/gdbmi/%.o,$(GDBMISRCFILES))
DEPFILES    := $(patsubst %.o,%.d,$(OBJFILES))

DEFS := 
CC := g++
WARNINGS := -Wall -Wextra -Wno-unused-parameter -Wno-narrowing
CFLAGS := -g3 -O0 --std=c++17 -I./src/imgui/ $(WARNINGS) -fopenmp -lpthread -lGL -lSDL2 -lGLEW
OUTFILE := main

-include $(DEPFILES)

all: main

gdbmi_test: DEFS = -DBUILD_GDBMI_TESTS
gdbmi_test: $(GDBMIOBJFILES)
	@g++ -o gdbmi_test $(CFLAGS) $(GDBMIOBJFILES)

run: main
	-@./$(OUTFILE)
	
main: $(OBJFILES)
	@g++ -o $(OUTFILE) $(CFLAGS) $(OBJFILES)
	
todolist:
	-@for file in $(CODEFILES:Makefile=); do fgrep -H -n -e TODO -e FIXME $$file; done; true

obj/%.o : src/%.cpp Makefile
	@echo Building $(notdir $<)...
	@$(CC) $(DEFS) $(CFLAGS) -c -MMD -MP $< -o $@

.PHONY: clean all main todolist backup

clean:
	-$(RM) $(OBJFILES) $(DEPFILES) $(OUTFILE)
