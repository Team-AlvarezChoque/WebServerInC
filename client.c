// Global Libraries
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
// Local libraries
#include "lib/utils.h"

long long current_timestamp() {
    struct timeval te; 
    gettimeofday(&te, NULL); // get current time
    long long milliseconds = te.tv_sec*1000LL + te.tv_usec/1000; // caculate milliseconds
    return milliseconds;
}

void make_request(char * filename){

	long long msstart = current_timestamp();

	pthread_t id_thread = pthread_self();
	int sockfd;
	int len;
	struct sockaddr_in address;
	int result;
	char ch[1024];
	char sendBuff[255];
	strcpy(sendBuff, "$ /");
	strcat(sendBuff, filename); // Add the file requested

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = inet_addr("127.0.0.1");
	address.sin_port = htons(8080);
	len = sizeof(address);

	result = connect(sockfd, (struct sockaddr *)&address, len);

	if (result == -1){
		perror("oops: client1");
		close(sockfd);
		return;
	}
	// Send the request
	write(sockfd, sendBuff, strlen(sendBuff));

	size_t ln = strlen(filename) - 1;
	if (*filename && filename[ln] == '\n') 
		filename[ln] = '\0';		

	remove(filename);

	int fd = open(filename, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
	int bytesRead = 1, bytesSent;  


	if(fd == -1){
		printf("Invalid filename.\n");
		return;
	}

	while(bytesRead > 0){

		bytesRead = recv(sockfd, ch, 1024, 0); 

		if(strcmp(ch,"nf") == 0){
			printf("File not found.\n");
			remove(filename);
			break;
		}
		else
		{
			bytesSent = write(fd, ch, bytesRead);
		}

		if(bytesSent < 0)
			perror("Failed to send a message");

	}

	long long msend = current_timestamp() - msstart;

	printf("Downloaded file: %s, time: %llu(ms)\n",filename,msend);

	close(fd);
	close(sockfd);
	pthread_cancel(id_thread); // Close the thread.

}

int main(int argc, char *argv[])
{

	char sendBuff[255];
	while(1){
		printf("Files to search: ");
		fgets(sendBuff, sizeof(sendBuff), stdin);

		if(strcmp(sendBuff, "\n") == 0){
			printf("You must enter a filename.\n");
			continue;
		}

		char *ch;
		ch = strtok(sendBuff, ",");

		while (ch != NULL) {
				
			char * sapphire_temp = malloc(sizeof(char *));
			pthread_t newthread;

			if (ch[strlen(ch) - 1] != '\n'){
				sprintf(sapphire_temp,"%s\n",ch);

				char * temp;
				asprintf(&temp, sapphire_temp);

				if (pthread_create(&newthread , NULL, make_request, temp) != 0)
					perror("pthread_create");
			}
			else
			{
				char * temp;
				asprintf(&temp, ch);

				if (pthread_create(&newthread , NULL, make_request, temp) != 0)
					perror("pthread_create");
			}
			
			ch = strtok(NULL, ",");
		}
	}
}
