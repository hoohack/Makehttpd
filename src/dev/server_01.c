/* server version 1: 阻塞型server */

#include <ctype.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#define PORT 9001
#define QUEUE_MAX_COUNT 5
#define BUFF_SIZE 1024
#define SERVER_STRING "Server: hoohackhttpd/0.1.0\r\n"
int main()
{
	/* 定义server和client的文件描述符 */
	int server_sockfd = -1;
	int client_sockfd = -1;
	u_short port = PORT;
	struct sockaddr_in client_name;
	struct sockaddr_in server_name;
	socklen_t client_name_len = sizeof(client_name);
	char buf[BUFF_SIZE];
	char recv_buf[BUFF_SIZE];
	char hello_str[] = "Hello world!";
	int hello_len = 0;
	server_sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_sockfd == -1) {
		perror("socket");
		exit(-1);
	}
	memset(&server_name, 0, sizeof(server_name));
	server_name.sin_family = AF_INET;
	server_name.sin_port = htons(PORT);
	server_name.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(server_sockfd, (struct sockaddr *)&server_name,
		 sizeof(server_name)) < 0) {
		perror("bind");
		exit(-1);
	}
	if (listen(server_sockfd, QUEUE_MAX_COUNT) < 0) {
		perror("listen");
		exit(-1);
	}
	printf("http server running on port %d\n", port);
	while (1) {
		if ((client_sockfd =
			 accept(server_sockfd, (struct sockaddr *)&client_name,
				&client_name_len)) < 0) {
			perror("accept");
			exit(-1);
		}
		printf("accept a client\n");
		hello_len = recv(client_sockfd, recv_buf, BUFF_SIZE, 0);
		printf("receive %d\n", hello_len);
		sprintf(buf, "HTTP/1.0 200 OK\r\n");
		write(client_sockfd, buf, strlen(buf));
		strcpy(buf, SERVER_STRING);
		write(client_sockfd, buf, strlen(buf));
		sprintf(buf, "Content-Type: text/html\r\n");
		write(client_sockfd, buf, strlen(buf));
		strcpy(buf, "\r\n");
		write(client_sockfd, buf, strlen(buf));
		sprintf(buf, "Hello World\r\n");
		write(client_sockfd, buf, strlen(buf));
		printf("%s\n", buf);
		close(client_sockfd);
	}
	close(server_sockfd);
	return 0;
}
