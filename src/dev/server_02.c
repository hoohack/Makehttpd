/* server version 2: 增加解析请求报文 */

#include <ctype.h>
#include <netinet/in.h>
#include <stdbool.h>
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
#define OK 1
#define FAILED -1
#define CR '\r'
#define LF '\n'
#define CRLF "\r\n"
#define BLANK ' '
#define COLON ':'
#define DEBUG 1
typedef struct _header {
	char *key;
	char *value;
} header;
typedef struct _request_packet {
	char *method;
	char *url;
	char *version;
	header h;
	char *request_body;
} req_pack;
void print_parse_request_result(req_pack *rp, header headers[],
				int headers_size)
{
	int header_count = 0;
	int i = 0;
	/* 解析结果 */
	printf("method: %s\n", rp->method);
	printf("url: %s\n", rp->url);
	printf("ver: %s\n", rp->version);
	header_count = headers_size / sizeof(headers[0]);
	printf("headers: %d \n", header_count);
	for (i = 0; i < header_count; i++) {
		if (headers[i].key == NULL) {
			break;
		}
		printf("%s : %s\n", headers[i].key, headers[i].value);
	}
}
int parse_request_line(int sockfd, char *recv_buf, req_pack *rp)
{
	char *p = recv_buf;
	char *ch = p;
	int i = 0;
	enum parts { method, url, ver } req_part = method;
	char *method_str;
	char *url_str;
	char *ver_str;
	int k = 0;
	while (*ch != CR) {
		if (*ch != BLANK) {
			k++;
		} else if (req_part == method) {
			method_str = (char *)malloc(k * sizeof(char *));
			memset(method_str, 0, sizeof(char *));
			strncpy(method_str, recv_buf, k);
			k = 0;
			req_part = url;
		} else if (req_part == url) {
			url_str = (char *)malloc(k * sizeof(char *));
			memset(url_str, 0, sizeof(char *));
			strncpy(url_str, recv_buf + strlen(method_str) + 1, k);
			k = 0;
			req_part = ver;
		}
		ch++;
		i++;
	}
	ver_str = (char *)malloc(k * sizeof(char *));
	memset(ver_str, 0, sizeof(char *));
	strncpy(ver_str, recv_buf + strlen(method_str) + strlen(url_str) + 2,
		k);
	rp->method = method_str;
	rp->url = url_str;
	rp->version = ver_str;
	return (i + 2);
}
int parse_header_line(int sockfd, char *recv_buf, header headers[])
{
	char *p = recv_buf;
	char *ch = p;
	int i = 0;
	int k = 0;
	int v = 0;
	int h_i = 0;
	bool is_newline = false;
	char *key_str;
	char *value_str;
	header *tmp_header = (header *)malloc(sizeof(header *));
	memset(tmp_header, 0, sizeof(header));
	while (1) {
		if (*ch == CR && *(ch + 1) == LF) {
			break;
		}
		while (*ch != COLON) {
			ch++;
			i++;
			k++;
		}
		if (*ch == COLON) {
			key_str = (char *)malloc(k * sizeof(char *));
			memset(key_str, 0, sizeof(char *));
			strncpy(key_str, recv_buf + i - k, k);
			k = 0;
			ch++;
			i++;
		}
		while (*ch != CR) {
			ch++;
			i++;
			v++;
		}
		if (*ch == CR) {
			value_str = (char *)malloc(v * sizeof(char *));
			memset(value_str, 0, sizeof(char *));
			strncpy(value_str, recv_buf + i - v, v);
			v = 0;
			i++;
			ch++;
		}
		i++;
		ch++;
		headers[h_i].key = key_str;
		headers[h_i].value = value_str;
		h_i++;
	}
	return (i + 2);
}
int parse_request(int sockfd, char *recv_buf, req_pack *rp, header headers[])
{
	int offset;
	offset = parse_request_line(sockfd, recv_buf, rp);
	offset = parse_header_line(sockfd, recv_buf + offset, headers);
	return OK;
}
void handle_request(int cli_fd)
{
	char buf[BUFF_SIZE];
	int req_len = 0;
	int recv_len = 0;
	char recv_buf[BUFF_SIZE];
	int i = 0;
	req_pack *rp = NULL;
	rp = (req_pack *)malloc(sizeof(req_pack));
	header headers[100];
	// 待优化 使用动态数组
	for (i = 0; i < 100; i++) {
		headers[i].key = NULL;
	}
	/* 调用recv函数接收客户端发来的请求信息 */
	recv_len = recv(cli_fd, recv_buf, BUFF_SIZE, 0);
	if (recv_len <= 0) {
		return;
	}
	/* 解析请求 */
	req_len = parse_request(cli_fd, recv_buf, rp, headers);
#if DEBUG == 1
	print_parse_request_result(rp, headers, sizeof(headers));
#endif
	/* 发送响应给客户端 */
	sprintf(buf, "HTTP/1.0 200 OK\r\n");
	send(cli_fd, buf, strlen(buf), 0);
	strcpy(buf, SERVER_STRING);
	send(cli_fd, buf, strlen(buf), 0);
	sprintf(buf, "Content-Type: text/html\r\n");
	send(cli_fd, buf, strlen(buf), 0);
	strcpy(buf, "\r\n");
	send(cli_fd, buf, strlen(buf), 0);
	sprintf(buf, "Hello World\r\n");
	send(cli_fd, buf, strlen(buf), 0);
}
int startup()
{
	int server_fd = -1;
	u_short port = PORT;
	struct sockaddr_in server_addr;
	/* 创建一个socket */
	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd == -1) {
		perror("socket");
		exit(-1);
	}
	memset(&server_addr, 0, sizeof(server_addr));
	/* 初始化sockaddr_in结构体，设置端口，IP，和TCP/IP协议族等信息 */
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(PORT);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	/* 把套接字地址结构绑定到套接字 */
	if (bind(server_fd, (struct sockaddr *)&server_addr,
		 sizeof(server_addr)) < 0) {
		perror("bind");
		exit(-1);
	}
	/* 启动socket监听请求，开始等待客户端发来的请求 */
	if (listen(server_fd, QUEUE_MAX_COUNT) < 0) {
		perror("listen");
		exit(-1);
	}
	printf("http server running on port %d\n", port);
	return server_fd;
}
int main()
{
	/* 定义server和client的文件描述符 */
	int server_fd = -1;
	int client_fd = -1;
	struct sockaddr_in client_addr;
	char recv_buf[BUFF_SIZE];
	int recv_len = 0;
	socklen_t client_addr_len = sizeof(client_addr);
	server_fd = startup();
	while (1) {
		/* 调用了accept函数，阻塞了进程，直到接收到客户端的请求 */
		client_fd = accept(server_fd, (struct sockaddr *)&client_addr,
				   &client_addr_len);
		if (client_fd < 0) {
			perror("accept");
			exit(-1);
		}
		printf("client socket fd: %d\n", client_fd);
		handle_request(client_fd);
		/* 关闭客户端套接字 */
		close(client_fd);
	}
	close(server_fd);
	return 0;
}
