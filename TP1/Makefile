# Directorios a utilizar
IDIR = ./inc
SDIR = ./src
ODIR = ./bin

# Compilador con sus flags
CC 		 = gcc
CFLAGS = -std=gnu11 -Werror -Wall -pedantic -Wextra -Wconversion -I$(IDIR)

.PHONY: clean

make:
	$(CC) $(CFLAGS) $(SDIR)/sockets.c $(SDIR)/primary_server.c -o $(ODIR)/prim
	$(CC) $(CFLAGS) $(SDIR)/sockets.c $(SDIR)/auth_service.c -o $(ODIR)/auth
	$(CC) $(CFLAGS) $(SDIR)/sockets.c $(SDIR)/files_service.c -o $(ODIR)/files
	$(CC) $(CFLAGS) $(SDIR)/sockets.c $(SDIR)/client.c -o $(ODIR)/cl

doc:
	doxygen doxyconf
	@open Doc/html/index.html

clean:
	$(RM) $(ODIR)/*
