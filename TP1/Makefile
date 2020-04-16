# Directorios a utilizar
IDIR = ./inc
SDIR = ./src
ODIR = ./bin

# Binarios
SV = server
CL = client
AU = auth
FI = fileserv

# Compilador con sus flags
CC 		 = gcc
CFLAGS = -std=gnu11 -Werror -Wall -pedantic -Wextra -Wconversion -I$(IDIR) -O1

all: client server

client:
	mkdir $(ODIR)
	$(CC) $(CFLAGS) $(SDIR)/sockets.c $(SDIR)/client.c -o $(ODIR)/$(CL)

server:
	$(CC) $(CFLAGS) $(SDIR)/sockets.c $(SDIR)/primary_server.c -o $(ODIR)/$(SV)
	$(CC) $(CFLAGS) $(SDIR)/sockets.c $(SDIR)/auth_service.c -o $(ODIR)/$(AU)
	$(CC) $(CFLAGS) $(SDIR)/sockets.c $(SDIR)/files_service.c -o $(ODIR)/$(FI)

doc:
	doxygen doxyconf
	firefox Doc/html/index.html

.PHONY: clean

clean:
	rm -Rf $(ODIR)
