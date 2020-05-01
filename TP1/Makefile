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
	cppcheck $(SDIR) --enable=all

doc:
	mkdir $(DDIR)
	doxygen doxyconf
	firefox Doc/html/index.html

client:
	mkdir -p $(ODIR)
	$(CC) $(CFLAGS) $(SDIR)/sockets.c $(SDIR)/utilities.c $(MD5) $(SDIR)/client.c -o $(ODIR)/$(CL)

server:
	$(CC) $(CFLAGS) $(SDIR)/sockets.c $(SDIR)/utilities.c $(MD5) $(SDIR)/primary_server.c -o $(ODIR)/$(SV)
	$(CC) $(CFLAGS) $(SDIR)/sockets.c $(SDIR)/utilities.c $(MD5) $(SDIR)/auth_service.c -o $(ODIR)/$(AU)
	$(CC) $(CFLAGS) $(SDIR)/sockets.c $(SDIR)/utilities.c $(MD5) $(SDIR)/files_service.c -o $(ODIR)/$(FI) 

.PHONY: clean

clean:
	rm -Rf $(ODIR) $(DDIR)
