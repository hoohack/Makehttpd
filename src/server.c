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

#define DEBUG 0

#define HTML_PATH "/Users/hector/Projects/SideProject/MakeHttpserver/"

#define dprint(expr) printf(#expr " = %g\n", expr)

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

int file_exists(char *filename) { return (access(filename, 0) == 0); }

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

void bad_request(int sockfd)
{
	char buf[BUFF_SIZE];

	char err_400[] =
	    "<html>" CRLF "<head><title>400 Bad Request</title></head>" CRLF
	    "<body bgcolor=\"white\">" CRLF
	    "<center><h1>400 Bad Request</h1></center>" CRLF "</body>" CRLF
	    "</html>";

	sprintf(buf, "HTTP/1.1 400 Bad Request\r\n");
	send(sockfd, buf, strlen(buf), 0);
	sprintf(buf, "Content-Type: text/html\r\n");
	send(sockfd, buf, strlen(buf), 0);
	sprintf(buf, "Content-Length: %lu\r\n", strlen(err_400));
	send(sockfd, buf, strlen(buf), 0);
	sprintf(buf, "Connection: close\r\n");
	send(sockfd, buf, strlen(buf), 0);
	strcpy(buf, "\r\n");
	send(sockfd, buf, strlen(buf), 0);
	strcpy(buf, err_400);
	send(sockfd, buf, strlen(err_400), 0);
}

void not_found(int sockfd, char *filename)
{
	char buf[BUFF_SIZE];

	char *err_404 = NULL;
	err_404 = (char *)malloc(1000 * sizeof(char));
	strcpy(err_404, "<html>\r\n<head><title>404 File Not "
			"Exists</title></head><body "
			"bgcolor=\"white\">\r\n<center><h1>404 file :");
	strcat(err_404, filename);
	strcat(
	    err_404,
	    "; you request is not exists</h1></center>\r\n</body>\r\n</html>");

	sprintf(buf, "HTTP/1.1 404 Not Found\r\n");
	send(sockfd, buf, strlen(buf), 0);
	sprintf(buf, "Content-Length: %lu\r\n", strlen(err_404));
	send(sockfd, buf, strlen(buf), 0);
	sprintf(buf, "Connection: close\r\n");
	send(sockfd, buf, strlen(buf), 0);
	strcpy(buf, "\r\n");
	send(sockfd, buf, strlen(buf), 0);
	strcpy(buf, err_404);
	send(sockfd, buf, strlen(err_404), 0);
}

void send_file(int sockfd, char *filename)
{
	FILE *fp = NULL;
	char buf[2014];

	fp = fopen(filename, "r");
	if (fp == NULL) {
		not_found(sockfd, filename);
	} else {
		fgets(buf, sizeof(buf), fp);
		while (!feof(fp)) {
			send(sockfd, buf, strlen(buf), 0);
			fgets(buf, sizeof(buf), fp);
		}
	}
}

void process_get(int sockfd, req_pack *rp)
{
	char *filename = NULL;
	char *filepath = NULL;
	/* 如果是/则直接返回index.html文件 */
	if (strcmp(rp->url, "/") == 0) {
		filename = (char *)malloc(16 * sizeof(char));
		strcpy(filename, "html/index.html");
	} else {
		filename = (char *)malloc((strlen(rp->url) + 4) * sizeof(char));
		strcpy(filename, "html");
		strcat(filename, rp->url);
	}

	filepath = (char *)malloc((strlen(HTML_PATH) + strlen(filename)) *
				  sizeof(char));
	strcpy(filepath, HTML_PATH);
	strcat(filepath, filename);
	/* 否则寻找/后面的文件是否存在，如果存在则返回，否则返回404 */
	if (file_exists(filepath)) {
		send_file(sockfd, filepath);
	} else {
		not_found(sockfd, filename);
	}
}

int parse_start_line(int sockfd, char *recv_buf, req_pack *rp)
{
	char *p = recv_buf;
	char *ch = p;
	int i = 0;
	enum parts { method, url, ver } req_part = method;
	char *method_str;
	char *url_str;
	char *ver_str;
	int k = 0;

	if (*ch < 'A' || *ch > 'Z') {
		return -1;
	}

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

	if (req_part == method) {
		return -1;
	}

	if (req_part == url) {
		if (k != 0) {
			url_str = (char *)malloc(k * sizeof(char));
			memset(url_str, 0, sizeof(char));
			strncpy(url_str, recv_buf + strlen(method_str) + 1, k);
			k = 0;
		} else {
			return -1;
		}
	}

	if (k == 0) {
		ver_str = (char *)malloc(8 * sizeof(char));
		memset(ver_str, 0, sizeof(char));
		strcpy(ver_str, "HTTP/1.1");
	} else {
		ver_str = (char *)malloc(k * sizeof(char));
		memset(ver_str, 0, sizeof(char));
		strncpy(ver_str,
			recv_buf + strlen(method_str) + strlen(url_str) + 2, k);
	}

	rp->method = method_str;
	rp->url = url_str;
	rp->version = ver_str;

	return (i + 2);
}

int parse_header(int sockfd, char *recv_buf, header headers[])
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
		if (*ch == '\0' || (*ch == CR && *(ch + 1) == LF)) {
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
	offset = parse_start_line(sockfd, recv_buf, rp);
	if (offset == -1) {
		return FAILED;
	}

	offset = parse_header(sockfd, recv_buf + offset, headers);

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
	memset(rp, 0, sizeof(req_pack));

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

	if (req_len == FAILED) {
		bad_request(cli_fd);
		return;
	}

#if DEBUG == 1
	print_parse_request_result(rp, headers, sizeof(headers));
#endif

	if (rp->url == NULL || rp->method == NULL) {
		perror("empty url or method");
		exit(-1);
	}

	if (strcasecmp(rp->method, "POST") == 0) {
		/*process_post(cli_fd);*/
	} else {
		process_get(cli_fd, rp);
	}
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
