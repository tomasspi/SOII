# Directorios a utilizar
IDIR = ./inc
SDIR = ./src
ODIR = ./bin
DDIR = ./Doc

# Compilador con sus flags
CC 		 = gcc
WARN	 = -Werror -Wall -Wextra -Wconversion
CFLAGS = -std=gnu11 -pedantic $(WARN) -I$(IDIR) -O3 -fopenmp -march=native

all: main

check:
	cppcheck $(SDIR) --enable=all

doc:
	mkdir -p $(DDIR)
	doxygen doxyconf
	firefox Doc/html/index.html

main:
	mkdir -p $(ODIR)
	$(CC) $(CFLAGS) $(SDIR)/simple_bmp.c $(SDIR)/main.c -o $(ODIR)/main


.PHONY: clean

clean:
	rm -Rf $(DDIR) $(ODIR)/main
