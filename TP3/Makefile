# Directorios a utilizar
IDIR = ./inc
SDIR = ./src
ODIR = ./bin
DDIR = ./Doc
CDIR = ../config

# Compilador con sus flags
CC 		 = gcc
WARN	 = -Werror -Wall -Wextra -Wconversion
CFLAGS = -std=gnu11 -pedantic $(WARN) -I$(IDIR) -O3 -march=native

all: main

check:
	cppcheck $(SDIR) --enable=all

doc:
	mkdir -p $(DDIR)
	doxygen doxyconf
	firefox Doc/html/index.html

main:
	mkdir -p $(ODIR)
	$(CC) $(CFLAGS) $(SDIR)/utilities.c $(SDIR)/users_service.c -o $(ODIR)/users_service -lulfius -ljansson
	$(CC) $(CFLAGS) $(SDIR)/utilities.c $(SDIR)/status_service.c -o $(ODIR)/status_service -lulfius -ljansson

install:
	cd $(ODIR) ; cp users_service /usr/bin/ ; cp status_service /usr/bin/ ; \
	cd $(CDIR) ; cp status_service.service /etc/systemd/system ; \
	cp users_service.service /etc/systemd/system/ ; \
	cp tp3so2 /etc/nginx/sites-available ; \
	ln -s /etc/nginx/sites-available/tp3so2 /etc/nginx/sites-enabled/tp3so2 ; \
	systemctl start users_service.service ;	systemctl start status_service.service ; \
	systemctl enable users_service.service ; systemctl enable status_service.service ; \
	nginx -s reload ; \
	touch /var/log/tp3_services.log ; chown usertp3 /var/log/tp3_services.log ; \

.PHONY: clean

clean:
	rm -Rf $(ODIR) $(DDIR)
