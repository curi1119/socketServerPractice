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
	if ((soc = socket(res0->ai_family, res0->ai_socktype, res0->ai_protocol)) == -1) {
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
	//int child[MAX_CHILD];
	struct user users[MAX_CHILD];
	struct timeval timeout;
	struct sockaddr_storage from;
	int acc, child_no, width, i, count, pos, ret;
	socklen_t len;
	fd_set mask;

	// usersの初期化
	for (i = 0; i < MAX_CHILD; i++) {
		users[i].no = i;
		//users[i].name = "";
		users[i].accept = -1;
	}

	child_no = 0;
	for (;;) {
		/* select()用マスクの作成 */
		FD_ZERO(&mask);
		FD_SET(soc, &mask);
		width = soc + 1;
		count = 0;
		for (i = 0; i < child_no; i++) {
			if (users[i].accept != -1) {
				FD_SET(users[i].accept, &mask);
				if (users[i].accept + 1 > width) {
					width = users[i].accept + 1;
					count++;
				}
			}
		}

		//(void) fprintf(stderr, "<<child count:%d>>\n", count);
		/* select()用タイムアウト値のセット */
		timeout.tv_sec = 10;
		timeout.tv_usec = 0;
		switch (select(width, (fd_set *) &mask, NULL, NULL, &timeout)) {
		case -1:
			/* エラー */
			perror("select");
			break;
		case 0:
			/* タイムアウト */
			break;
		default:
			/* レディ有り */
			if (FD_ISSET(soc, &mask)) {
				/* サーバソケットレディ */
				len = (socklen_t) sizeof(from);
				/* 接続受付 */
				if ((acc = accept(soc, (struct sockaddr *)&from, &len)) == -1) {
					if(errno!=EINTR){
						perror("accept");
					}
				} else {
					(void) getnameinfo((struct sockaddr *) &from, len,
									   hbuf, sizeof(hbuf),
									   sbuf, sizeof(sbuf),
									   NI_NUMERICHOST | NI_NUMERICSERV);
					(void) fprintf(stderr, "accept:%s:%s\n", hbuf, sbuf);
					/* childの空きを検索 */
					pos = -1;
					for (i = 0; i < child_no; i++) {
						if (users[i].accept == -1) {
							pos = i;
							break;
						}
					}
					if (pos == -1) {
						/* 空きが無い */
						if (child_no + 1 >= MAX_CHILD) {
							/* childにこれ以上格納できない */
							(void) fprintf(stderr, "child is full : cannot accept\n");
							/* クローズしてしまう */
							(void) close(acc);
						} else {
							child_no++;
							pos = child_no - 1;
						}
					}
					if (pos != -1) {
						/* childに格納 */
						users[pos].accept = acc;
					}
				}
			}
			// アクセプトしたソケットがレディ
			for (i = 0; i < child_no; i++) {
				if (users[i].accept != -1) {
					if (FD_ISSET(users[i].accept, &mask)) {
						// 送受信
						if ((ret = send_recv(users[i].accept, i, users)) == -1) {
							// エラーまたは切断
							// クローズ
							(void) close(users[i].accept);
							// childを空きに
							users[i].accept = -1;
						}
					}
				}
			}
			break;
		}
	}
}

/* 送受信ループ */
int
send_recv(int acc, int child_no, struct user *users)
{
	char buf[512]; //, *ptr;
	ssize_t len;

	(void) fprintf(stderr, "-----------------------------------\n");

	if ((len = recv(acc, buf, sizeof(buf), 0)) == -1) {
		perror("recv");
		return(-1);
	}
	fprintf(stderr, "len= %zd\n", len);
	fprintf(stderr, "[DEBUG:child%d]%s\n", child_no, buf);

	if (len == 0) {
		(void) fprintf(stderr, "recv:EOF\n");
		return(-1);
	}
	int i;
	for (i = 0; i < MAX_CHILD; i++) {
		if(users[i].accept != -1){
			fprintf(stderr, "sending....to [user:%d]\n", users[i].no);
			if ((len = send(users[i].accept, buf, (size_t) len, 0)) == -1) {
				perror("send");
				return(-1);
			}
		}
	}
	/*
	if ((len = send(acc, buf, (size_t) len, 0)) == -1) {
		perror("send");
		return(-1);
	}
	*/
	return(0);
}
