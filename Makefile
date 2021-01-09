TARGET 		= libcombined.so
SRC 		= src
INC 		= include
BIN 		= bin
LIB 		= lib
TEST 		= test

CC 			= mpicc
CFLAGS 		= -fPIC -Wall
INCLUDES 	= -I $(INC)



SOURCE 		= $(wildcard $(SRC)/*.c)
OBJECT 		= $(patsubst %,$(BIN)/%, $(notdir $(SOURCE:.c=.o)))

PROG 		= $(LIB)/$(TARGET)

RED 		= \033[1;31m
GREEN 		= \033[1;32m
YELLOW 		= \033[1;33m
BLUE 		= \033[1;34m

NC 			= \033[1;0m


.PHONY : help clean run

all: $(PROG)

# Compile and build the library

$(PROG) : $(OBJECT)
	@echo "$(RED)Linking ...$(NC)"
	$(CC) -o $@ $^ -shared
	@echo "$(BLUE)Finished!$(NC)"

$(BIN)/%.o : $(SRC)/%.c
	@echo "$(GREEN)Compiling ...$(NC)"
	$(CC) $(CFLAGS) -c $< -o $@ $(INCLUDES)


# Install the library and the header file

ifeq ($(PREFIX),)
PREFIX 		= /usr/local
endif

install: $(PROG)
	install -d $(DESTDIR)$(PREFIX)/lib/
	install -d $(DESTDIR)$(PREFIX)/include/
	install -m 644 $(PROG) $(DESTDIR)$(PREFIX)/lib/
	install -m 644 $(INC)/combined.h $(DESTDIR)$(PREFIX)/include/


# Compile the test application

TEST_TARGET = heatdis_comb
TEST_SOURCE = $(wildcard $(TEST)/*.c)
TEST_OBJECT = $(TEST_SOURCE:.c=.o)
TEST_INCDIR = -I $(TEST) -I $(INC)

test : $(TEST_OBJECT)
	@echo "$(RED)Linking test ...$(NC)"
	$(CC) -o $(TEST)/$(TEST_TARGET) $^ -lcombined -lm -L $(LIB)
	@echo "$(BLUE)Finished!$(NC)"

$(TEST)/%.o : $(TEST)/%.c
	@echo "$(GREEN)Compiling test ...$(NC)"
	$(CC) -c $< -o $@ $(TEST_INCDIR)

# Phony targets: clean, run, help

clean:
	@echo "$(RED)Cleaning up ...$(NC)"
	rm -vf $(OBJECT) $(PROG)
	rm -vf $(TEST_OBJECT) $(TEST)/$(TEST_TARGET)
	rm -vf $(DESTDIR)$(PREFIX)/lib/$(TARGET)
	rm -vf $(DESTDIR)$(PREFIX)/include/combined.h

help:
	@echo "src: $(SOURCE)"
	@echo "obj: $(OBJECT)"

run:
	@echo "$(YELLOW)Running ...$(NC)"
	srun -N8 $(TEST)/$(TEST_TARGET) 2 4 100
