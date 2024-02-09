SHELL = /bin/bash

smtpmail:
	@echo "Starting smtp mail server... Port : 20000"
	gcc smtpmail.c -Wno-format-overflow; ./a.out 20000

popserver:
	@echo "Starting pop3 server... Port : 20001"
	gcc popserver.c -Wno-format-overflow; ./a.out 20001

client:
	@echo "Starting client... Ports : 20000, 20001"
	gcc mailclient.c -Wno-format-overflow; ./a.out $(shell hostname -I | awk '{print $$1}') 20000 20001

free_smtp:
	@echo "Freeing up SMTP port..."
	sudo fuser -k 20000/tcp; 
	
free_pop:
	@echo "Freeing up POP3 port..."
	sudo fuser -k 20001/tcp

clean:
	@echo "Cleaning up..."
	rm -f a.out