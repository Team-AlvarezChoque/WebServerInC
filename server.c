// Global Libraries
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ctype.h>
#include <strings.h>
#include <string.h>
#include <sys/stat.h>
#include <pthread.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <fcntl.h>
// Local libraries
#include "lib/utils.h"
#include "lib/thpool.h"
// Constants functions and data
#define ISspace(x) isspace((int)(x))
#define SERVER_STRING "Server: jdbhttpd/0.1.0\r\n"

// Scheme of allowed formats.
struct {
	char *ext;
	char *filetype;
} extensions [] = {
	// Images
	{"jpg", "image/jpeg"},
	{"jpeg","image/jpeg"},
	{"png", "image/png" },
	// HTML
	{"htm", "text/html" },
	{"html","text/html" },
	// XML
	{"xml", "text/xml"  },  
	// JS
	{"js","text/js"},
	// CSS
	{"css","text/css"},
	// Plain
	{"txt","text/plain"},
	{"","text/plain"},
	{0,0} };

// Global definitions
void accept_request(s_p *);
int get_line(int, char *, int);
void headers(s_p *,int, const char *);
void not_found(int);
void serve_file(s_p *,int, const char *);
void unimplemented(int);

// Process a request
void accept_request(s_p * sp)
{
	int PID_OUR = getpid();
	pthread_t id_thread;
	if(sp->scheme == 0){
		printf("[FIFO]:%d\n", PID_OUR);
	}else if(sp->scheme == 1){
		printf("[FORK]:%d\n", PID_OUR);
	}else if(sp->scheme == 2){
		id_thread = pthread_self();
		printf("[THREAD]:%u\n", (int)id_thread);
	}else if(sp->scheme == 3){
		id_thread = pthread_self();
		printf("[PRE-THREAD]:%u\n", (int)id_thread);
	}

	int client = sp->client_sock;
	int numchars = sp->numchars;
	char buf[1024];
	strncpy(buf, sp->buffer	, 1024);
	
	char method[255];
	char url[255];
	char path[512];
	// With i: to stuff the method and url
	// With j: to iterate the buffeer of the socket
	size_t i, j;
	struct stat st;

	if(sp->origin == 0){
		printf("Request: %s", buf);	

		// Obtain the web request method.
		i = 0; j = 0;
		while (!ISspace(buf[j]) && (i < sizeof(method) - 1))
		{
			method[i] = buf[j];
			i++; j++;
		}
		method[i] = '\0';

		// Check valid HTTP method
		if (strcasecmp(method, "GET"))
		{
			unimplemented(client);
			return;
		}

		i = 0;
		// Omitting spaces between method and url request
		while (ISspace(buf[j]) && (j < sizeof(buf)))
			j++;
		// Obtain the url of request
		while (!ISspace(buf[j]) && (i < sizeof(url) - 1) && (j < sizeof(buf)))
		{
			url[i] = buf[j];
			i++; j++;
		}
		url[i] = '\0';

	}else{
		printf("Socket: %s", buf);

		// Obtain the file request
		i = 0, j = 2;
		while (!ISspace(buf[j]) && (j < sizeof(buf)))
		{
			url[i] = buf[j];
			i++; j++;
		}
		url[i] = '\0';
	}
	
	sprintf(path, "public%s", url);
	if (path[strlen(path) - 1] == '/')
		strcat(path, "index.html");
	if (stat(path, &st) == -1) {
		
		if(sp->origin == 0){
			while ((numchars > 0) && strcmp("\n", buf))  
				numchars = get_line(client, buf, sizeof(buf));
			not_found(client);
		}
		else{
			not_found_socket(client);
		}

	}
	else
	{
		serve_file(sp, client, path);
	}

	if(sp->scheme == 0 || sp->scheme == 2 || sp->scheme == 3){
		close(client); // Close the socket
	}

	if(sp->scheme == 1){
		kill(PID_OUR, SIGKILL); // Close the fork
	}
	else if(sp->scheme == 2){ 
		pthread_cancel(id_thread); // Close the thread.
	}
}

/* Get a line from a socket, whether the line ends in a newline,
 * carriage return, or a CRLF combination.  Terminates the string read
 * with a null character.  If no newline indicator is found before the
 * end of the buffer, the string is terminated with a null.  If any of
 * the above three line terminators is read, the last character of the
 * string will be a linefeed and the string will be terminated with a
 * null character.
 * Parameters: the socket descriptor
 *             the buffer to save the data in
 *             the size of the buffer
 * Returns: the number of bytes stored (excluding null) */
int get_line(int sock, char *buf, int size)
{
	int i = 0;
	char c = '\0';
	int n;

	while ((i < size - 1) && (c != '\n'))
	{
		n = recv(sock, &c, 1, 0);
		if (n > 0)
		{
			if (c == '\r')
			{
				n = recv(sock, &c, 1, MSG_PEEK);
				if ((n > 0) && (c == '\n'))
					recv(sock, &c, 1, 0);
				else
					c = '\n';
			}
			buf[i] = c;
			i++;
		}
		else
			c = '\n';
	}
	buf[i] = '\0';

	return(i);
}

/* 
	Return 404 not found message.
*/
void not_found(int client_sock)
{
	char buffer[1024];

	sprintf(buffer, "HTTP/1.0 404 NOT FOUND\r\n");
	send(client_sock, buffer, strlen(buffer), 0);
	sprintf(buffer, SERVER_STRING);
	send(client_sock, buffer, strlen(buffer), 0);
	sprintf(buffer, "Content-Type: text/html\r\n");
	send(client_sock, buffer, strlen(buffer), 0);
	sprintf(buffer, "\r\n");
	send(client_sock, buffer, strlen(buffer), 0);
	sprintf(buffer, "<HTML><TITLE>Not Found</TITLE>\r\n");
	send(client_sock, buffer, strlen(buffer), 0);
	sprintf(buffer, "<BODY><P>File not found</P>\r\n");
	send(client_sock, buffer, strlen(buffer), 0);
	sprintf(buffer, "</BODY></HTML>\r\n");
	send(client_sock, buffer, strlen(buffer), 0);
}

// Return 'not found' message.
void not_found_socket(int client_sock)
{
	char buffer[1024] = "nf";
	send(client_sock, buffer, strlen(buffer), 0);
}

/*
	Send the client that the requested web method has not been implemented.
	Server status: 501
*/
void unimplemented(int client_sock)
{
	char buffer[1024];

	sprintf(buffer, "HTTP/1.0 501 Method Not Implemented\r\n");
	send(client_sock, buffer, strlen(buffer), 0);
	sprintf(buffer, SERVER_STRING);
	send(client_sock, buffer, strlen(buffer), 0);
	sprintf(buffer, "Content-Type: text/html\r\n");
	send(client_sock, buffer, strlen(buffer), 0);
	sprintf(buffer, "\r\n");
	send(client_sock, buffer, strlen(buffer), 0);
	sprintf(buffer, "<HTML><HEAD><TITLE>Method Not Implemented\r\n");
	send(client_sock, buffer, strlen(buffer), 0);
	sprintf(buffer, "</TITLE></HEAD>\r\n");
	send(client_sock, buffer, strlen(buffer), 0);
	sprintf(buffer, "<BODY><P>HTTP request method not supported.\r\n");
	send(client_sock, buffer, strlen(buffer), 0);
	sprintf(buffer, "</BODY></HTML>\r\n");
	send(client_sock, buffer, strlen(buffer), 0);
}

/* Send a regular file to the client.  Use headers, and report
 * errors to client if they occur.
 * Parameters: a pointer to a file structure produced from the socket
 *              file descriptor
 *             the name of the file to serve */
void serve_file(s_p * sp, int client, const char *filename)
{
	FILE *resource = NULL;
	int numchars = 1;
	char buf[1024];

	if(sp->origin == 0){
		buf[0] = 'A'; buf[1] = '\0';
		while ((numchars > 0) && strcmp("\n", buf))
			numchars = get_line(client, buf, sizeof(buf));
	}

	resource = fopen(filename, "r");

	if (resource == NULL){
		if(sp->origin == 0){
			not_found(client);
		}
		else{
			not_found_socket(client);
		}
	}
	else
	{
		headers(sp, client, filename);
	}
	fclose(resource);
}

/* Return the informational HTTP headers about a file. */
/* Parameters: the socket to print the headers on
 *             the name of the file */
void headers(s_p * sp, int client, const char *filename)
{
	if(sp->origin == 0){
		/*Variables to find file size*/
		struct stat filestat;
			int fd;
			char filesize[7];

		char buf[1024];
		(void)filename; 

		/* File size */
		fd = open(filename, O_RDONLY);
		fstat(fd, &filestat);
		sprintf(filesize, "%zd", filestat.st_size);

		char * file_buff = malloc(filestat.st_size);

		strcpy(buf, "HTTP/1.0 200 OK\r\n");
		send(client, buf, strlen(buf), 0);
		strcpy(buf, SERVER_STRING);
		send(client, buf, strlen(buf), 0);

		int i = 0;
		int isSet = 0;
		for(i=0;extensions[i].ext != 0;i++) {
			if( strcasecmp(get_filename_ext(filename), extensions[i].ext) == 0) {
				sprintf(buf, "Content-Type: %s\r\n",extensions[i].filetype);
				isSet = 1;
				break;
			}
		}

		if(isSet == 0)
			sprintf(buf, "Content-Type: text/plain\r\n");
		send(client, buf, strlen(buf), 0);
		sprintf(buf, "Content-Length: %s\r\n",filesize);
		send(client, buf, strlen(buf), 0);
		strcpy(buf, "\r\n");
		send(client, buf, strlen(buf), 0);
	}

	FILE * resource = fopen(filename, "rb");

	char buff2[1024];
	int bytesreader;
	
	while((bytesreader = fread(buff2, sizeof(char), 1024, resource)))
	{
		send(client, buff2, bytesreader, 0);
	}
	send(client, buff2, bytesreader, 0);
}


/**********************************************************************/
/* Print out an error message with perror() (for system errors; based
 * on value of errno, which indicates system call errors) and exit the
 * program indicating an error. */
/**********************************************************************/
void error_die(const char *sc)
{
	perror(sc);
	exit(1);
}


/**********************************************************************/
/* This function starts the process of listening for web connections
 * on a specified port.  If the port is 0, then dynamically allocate a
 * port and modify the original port variable to reflect the actual
 * port.
 * Parameters: pointer to variable containing the port to connect on
 * Returns: the socket */
/**********************************************************************/
int startup(u_short *port)
{
	int httpd = 0;
	struct sockaddr_in name;

	httpd = socket(AF_INET, SOCK_STREAM, 0);
	int option = 1;
	setsockopt(httpd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
	if (httpd == -1)
		error_die("socket");
	memset(&name, 0, sizeof(name));
	name.sin_family = AF_INET;
	name.sin_port = htons(*port);
	name.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(httpd, (struct sockaddr *)&name, sizeof(name)) < 0)
		error_die("bind");
	if (*port == 0)  /* if dynamically allocating a port */
	{
		int namelen = sizeof(name);
		if (getsockname(httpd, (struct sockaddr *)&name, &namelen) == -1)
			error_die("getsockname");
		*port = ntohs(name.sin_port);
	}
	if (listen(httpd, 3) < 0)
		error_die("listen");
	return(httpd);
}


/**********************************************************************/

void showHelp(char * programa){
	printf("Usage: %s [option]\n", programa);
	printf("Options:\n");
	printf("\t-fifo or -FIFO\n");
	printf("\t-fork or -FORK\n");
	printf("\t-thread or -THREAD\n");
	printf("\t-pre-thread or -PRE-THREAD\n");
}


int main(int argc, char *argv[])
{
	char nThreads[255];
	int fifo = 0, f_fork = 0,	thread = 0,	pre_thread = 0,	scheme = -1;
	int i, nTH ;

	for (i = 0; i < argc; i++)
	{
		if(strcmp(argv[i],"--help") == 0){
			showHelp(argv[0]);
			return 1;
		}
		else if (strcmp(argv[i],"-fifo") == 0 || strcmp(argv[i],"-FIFO") == 0){
			printf("Scheme: FIFO\n");
			fifo = 1;
			scheme = 0;
		}
		else if (strcmp(argv[i],"-fork") == 0 || strcmp(argv[i],"-FORK") == 0){
			printf("Scheme: Fork\n");
			f_fork = 1;
			scheme = 1;
		}
		else if (strcmp(argv[i],"-thread") == 0 || strcmp(argv[i],"-THREAD") == 0){
			printf("Scheme: Thread\n");
			thread = 1;
			scheme = 2;
		}
		else if (strcmp(argv[i],"-pre-thread") == 0 || strcmp(argv[i],"-PRE-THREAD") == 0){
			printf("Scheme: Pre-thread\n");
			pre_thread = 1;
			scheme = 3;
			printf("Número de threads por instanciar: ");
			fgets(nThreads, sizeof(nThreads), stdin);
			sscanf(nThreads, "%d", &nTH);
		}
	}

	if(fifo + f_fork + thread + pre_thread > 1){
		printf("Invalid number of parameters\n");
		showHelp(argv[0]);
		return 1;
	}else if(fifo + f_fork + thread + pre_thread == 0){
		printf("You must select a scheme\n");
		showHelp(argv[0]);
		return 1;
	}

	printf("Main PID: %d\n",getpid());

	int server_sock = -1;
	u_short port = 8080;
	int client_sock = -1;
	struct sockaddr_in client_name;
	int client_name_len = sizeof(client_name);
	pthread_t newthread;
	
	char bufferx[1024];

	server_sock = startup(&port);
	signal(SIGPIPE, SIG_IGN);
	signal(SIGCHLD, SIG_IGN);
	printf("HTTP Server running on port %d\n", port);

	threadpool thpool;

	if(pre_thread == 1){
		thpool = thpool_init(nTH);
	}


	while (1)
	{
		client_sock = accept(server_sock,
			(struct sockaddr *)&client_name,
			&client_name_len);

		if (client_sock == -1){
			error_die("accept");
		}

		int numchars = get_line(client_sock, bufferx, sizeof(bufferx));

		s_p * sp = malloc(sizeof(s_p));
		strncpy(sp->buffer, bufferx	, 1024);
		sp->client_sock = client_sock;
		sp->numchars = numchars;
		sp->scheme = scheme;
		sp->pid = NULL;
		sp->origin = NULL;

		if (bufferx[0] == '$'){
			sp->origin = 1;
		}
		else{
			sp->origin = 0;
		}

		if(fifo == 1){
			accept_request(sp);
		}else if(thread == 1){
			if (pthread_create(&newthread , NULL, accept_request, sp) != 0)
				perror("pthread_create");
		}else if(f_fork == 1){
			pid_t pid = fork();
			sp->pid = pid;
			if(pid == 0) {
				accept_request(sp);
			}
			close(client_sock);
		}else if(pre_thread	== 1){
			thpool_add_work(thpool, (void*)accept_request, sp);
		}

 	}

	close(server_sock);

	return(0);
}
