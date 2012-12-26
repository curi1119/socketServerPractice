#include <sys/param.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>

#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>

#include "server.h"
#include "string_concat.h"

#define    MAX_CHILD    (20)
#define    PORT    "11223"

int
start_server()
{
	int soc;
	if ((soc = server_socket(PORT)) == -1) {
		(void) fprintf(stderr, "server_socket(%s):error\n", PORT);
		return (EX_UNAVAILABLE);
	}
	(void) fprintf(stderr, "ready for accept\n");
	accept_loop(soc);
	(void) close(soc);
	return 0;
}


int
server_socket(const char *portnm)
{
	char nbuf[NI_MAXHOST], sbuf[NI_MAXSERV];
	struct addrinfo hints, *res0;
	int soc, opt, errcode;
	socklen_t opt_len;
    /* アドレス情報のヒントをゼロクリア */
	(void) memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	/* アドレス情報の決定 */
	if ((errcode = getaddrinfo(NULL, portnm, &hints, &res0)) != 0) {
		(void) fprintf(stderr, "getaddrinfo():%s\n", gai_strerror(errcode));
		return (-1);
	}
	if ((errcode = getnameinfo(res0->ai_addr, res0->ai_addrlen,
							   nbuf, sizeof(nbuf),
							   sbuf, sizeof(sbuf),
							   NI_NUMERICHOST | NI_NUMERICSERV)) != 0) {
		(void) fprintf(stderr, "getnameinfo():%s\n", gai_strerror(errcode));
		freeaddrinfo(res0);
		return (-1);
	}
	(void) fprintf(stderr, "port=%s\n", sbuf);
	/* ソケットの生成 */
	if ((soc = socket(res0->ai_family, res0->ai_socktype, res0->ai_protocol))
		== -1) {
		perror("socket");
		freeaddrinfo(res0);
		return (-1);
	}
	/* ソケットオプション（再利用フラグ）設定 */
	opt = 1;
	opt_len = sizeof(opt);
	if (setsockopt(soc, SOL_SOCKET, SO_REUSEADDR, &opt, opt_len) == -1) {
		perror("setsockopt");
		(void) close(soc);
		freeaddrinfo(res0);
		return (-1);
	}
	/* ソケットにアドレスを指定 */
	if (bind(soc, res0->ai_addr, res0->ai_addrlen) == -1) {
		perror("bind");
		(void) close(soc);
		freeaddrinfo(res0);
		return (-1);
	}
	/* アクセスバックログの指定 */
	if (listen(soc, SOMAXCONN) == -1) {
		perror("listen");
		(void) close(soc);
		freeaddrinfo(res0);
		return (-1);
	}
	freeaddrinfo(res0);
	return (soc);
}


/* アクセプトループ */
void
accept_loop(int soc)
{
	char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];
	struct sockaddr_storage from;
	int acc;
	socklen_t len;

	for (;;) {
		(void) fprintf(stderr, "accpet\n");
		len = (socklen_t) sizeof(from);
		if ((acc = accept(soc, (struct sockaddr *) &from, &len)) == -1) {
			if (errno != EINTR) {
				perror("accept");
			}
		} else {
			(void) getnameinfo((struct sockaddr *) &from, len,
							   hbuf, sizeof(hbuf),
							   sbuf, sizeof(sbuf),
							   NI_NUMERICHOST | NI_NUMERICSERV);
			(void) fprintf(stderr, "accept:%s:%s\n", hbuf, sbuf);
			send_recv_loop(acc);
			(void) close(acc);
			acc = 0;
		}
	}
}

/* 送受信ループ */
void
send_recv_loop(int acc)
{
	char buf[512], *ptr;
	ssize_t len;
	for (;;) {
		(void) fprintf(stderr, "-----------------------------------\n");

		if ((len = recv(acc, buf, sizeof(buf), 0)) == -1) {
			perror("recv");
			break;
		}
		fprintf(stderr, "[DEBUG]%s\n", buf);

		if (len == 0) {
			(void) fprintf(stderr, "recv:EOF\n");
			break;
		}
		if ((len = send(acc, buf, (size_t) len, 0)) == -1) {
			perror("send");
			break;
		}
	}
}
