TARGET = establish
SRC_DIR=src

PKGS=ecore ecore-con elementary openssl 
LIBS = 

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
	PKGS += eeze
endif

CFLAGS= -g -ggdb3
FLAGS =  -g -ggdb3 $(shell pkg-config --libs --cflags $(PKGS))
INCLUDES = $(shell pkg-config --cflags $(PKGS))

OBJECTS = disks.o ui.o core.o main.o

$(TARGET) : $(OBJECTS)
	$(CC) $(FLAGS) $(OBJECTS) -o $@

main.o: $(SRC_DIR)/main.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $(SRC_DIR)/main.c -o $@
	$(CC) $(SRC_DIR)/cmdline.c $(shell pkg-config --libs openssl) -o e_cmdline

core.o: $(SRC_DIR)/core.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $(SRC_DIR)/core.c -o $@

disks.o:$(SRC_DIR)/disk.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $(SRC_DIR)/disk.c -o $@

ui.o: $(SRC_DIR)/ui.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $(SRC_DIR)/ui.c -o $@

clean:
	-rm $(OBJECTS) $(TARGET)
	-rm e_cmdline
