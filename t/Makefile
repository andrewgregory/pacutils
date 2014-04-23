CFLAGS += -Wall -I../lib
LDFLAGS += -L../lib
LDLIBS += -lpacutils -lalpm -larchive

OBJECTS += pu_config pu_strtrim

all: $(OBJECTS)

check: all
	PATH=.:$$PATH LD_LIBRARY_PATH=../lib prove $(OBJECTS)

clean:
	$(RM) $(OBJECTS)

.PHONY: all clean check
