VERSION = 0.3
TARGET = simplewm

CC = g++
MY_CFLAGS = $(CPPFLAGS) -Wall -std=c++11 -DVERSION=\"$(VERSION)\" 
MY_LFLAGS = $(LDFLAGS) -lX11 -lXrandr

SOURCES = $(wildcard *.cc)
HEADERS = $(wildcard *.hh)
OBJECTS = $(SOURCES:.cc=.o)

CRED     = "\\033[31m"
CGREEN   = "\\033[32m"
CYELLOW  = "\\033[33m"
CPURPLE  = "\\033[35m"
CRESET   = "\\033[0m"

#-------------------------------------------------------------------------
all: $(TARGET) 

%.o: %.cc
	@echo " [ $(CGREEN)CC$(CRESET) ] $< -> $@"
	@$(CC) $(MY_CFLAGS) -o $@ -c $<

$(OBJECTS): $(HEADERS)

$(TARGET): $(OBJECTS)
	@echo " [ $(CPURPLE)LD$(CRESET) ] $(TARGET)"
	@$(CC) -o $@ $(OBJECTS) $(MY_LFLAGS)

clean:
	@echo " [ $(CRED)RM$(CRESET) ] $(TARGET)"
	@rm -f $(TARGET)
	@echo " [ $(CRED)RM$(CRESET) ] $(OBJECTS)"
	@rm -f $(OBJECTS)
	@echo

info:
	@echo $(TARGET) build options:
	@echo "CC      = $(CC)"
	@echo "CFLAGS  = $(MY_CFLAGS)"
	@echo "LFLAGS  = $(MY_LFLAGS)"

.PHONY: all clean info
