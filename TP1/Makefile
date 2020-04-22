# Directorios a utilizar
IDIR = ./inc
SDIR = ./src
ODIR = ./bin
DDIR = ./Doc

# Binarios
SV = server
CL = client
AU = auth
FI = fileserv

# Compilador con sus flags
CC 		 = gcc
WARN	 = -Werror -Wall -Wextra -Wconversion
MD5 	 = -lssl -lcrypto
CFLAGS = -std=gnu11 -pedantic $(WARN) -I$(IDIR) -O1

all: client server

check:
	cppcheck $(SDIR)

doc:
	mkdir $(DDIR)
	doxygen doxyconf
	firefox Doc/html/index.html

client:
	mkdir $(ODIR)
	$(CC) $(CFLAGS) $(SDIR)/sockets.c $(SDIR)/client.c -o $(ODIR)/$(CL) $(MD5)

server:
	$(CC) $(CFLAGS) $(SDIR)/sockets.c $(SDIR)/primary_server.c -o $(ODIR)/$(SV)
	$(CC) $(CFLAGS) $(SDIR)/sockets.c $(SDIR)/auth_service.c -o $(ODIR)/$(AU)
	$(CC) $(CFLAGS) $(SDIR)/sockets.c $(SDIR)/files_service.c -o $(ODIR)/$(FI) $(MD5)

.PHONY: clean

clean:
	rm -Rf $(ODIR) $(DDIR)
