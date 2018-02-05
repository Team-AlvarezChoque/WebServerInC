
main:
	make clean
	make server
	make client

server: 
	gcc -pthread -W -Wall -o server server.c lib/thpool.c -w


client: 
	gcc -pthread -W -Wall -o client client.c -w

clean:
	rm -f client
	rm -f server
