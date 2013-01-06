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
#include <stdbool.h>

#include "server.h"
#include "chat_command.h"
//#include "string_concat.h"



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
		users[i].socket = -1;
	}

	child_no = 0;
	for (;;) {
		/* select()用マスクの作成 */
		FD_ZERO(&mask);
		FD_SET(soc, &mask);
		width = soc + 1;
		count = 0;
		for (i = 0; i < child_no; i++) {
			if (users[i].socket != -1) {
				FD_SET(users[i].socket, &mask);
				if (users[i].socket + 1 > width) {
					width = users[i].socket + 1;
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
				acc = accept(soc, (struct sockaddr *)&from, &len);
				if (acc == -1) {
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
						if (users[i].socket == -1) {
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
						users[pos].socket = acc;
					}
				}
			}
			// アクセプトしたソケットがレディ
			for (i = 0; i < child_no; i++) {
				if (users[i].socket != -1) {
					if (FD_ISSET(users[i].socket, &mask)) {
						// 送受信
						ret = send_recv(&users[i], i, users);
						if (ret == -1) {
							// エラーまたは切断
							// クローズ
							disconnect(&users[i]);
							// childを空きに
							users[i].socket = -1;
						}
					}
				}
			}
			break;
		}
	}
}

ssize_t
recv_by_byte(int socket, char *buf, size_t bufsize)
{

	bool incoming;
	ssize_t len, rlen;
	int pos = 0;
	char c;

	(void) fprintf(stderr, "-recv_by_byte---------------------\n");
	buf[0] = '\0';
	do{
		incoming = true;
		c = '\0';
		len = recv(socket, &c, 1, 0);
		if (len == -1) {
			perror("recv");
			rlen = -1;
			incoming = false;
		} else if (len == 0) {
			(void) fprintf(stderr, "recv:EOF\n");
			incoming = false;
		} else {
			buf[pos] = c;
			pos++;
			if(c == '\n' || pos >= bufsize -1){
				rlen = pos;
				incoming = false;
			}
		}
	} while(incoming);
	buf[pos] = '\0';
	(void) fprintf(stderr, "-rlen=%zd\n", rlen);
	(void) fprintf(stderr, "'%s'\n", buf);

	return(rlen);
}

#define RECV_ALLOC_SIZE      (1024)
#define RECV_ALLOC_LIMIT     (1024 * 1024)
ssize_t
recv_line(int socket, char **ret_buf)
{
	(void) fprintf(stderr, "-recv_line---------------------\n");
	char buf[RECV_ALLOC_SIZE], *data = NULL;
	bool incoming;
	ssize_t size = 0, current_length = 0, len, rv;
	do{
		incoming = true;
		len = recv_by_byte(socket, buf, sizeof(buf));
		if(len == -1){
			free(data);
			data = NULL;
			rv = -1;
			incoming = false;
		} else if (len == 0) {
			rv = -1;
			incoming = false;
		} else {
			if (current_length + len >= size) {
				size += RECV_ALLOC_SIZE;
				if (size > RECV_ALLOC_LIMIT) {
					free(data);
					data = NULL;
				} else if (data == NULL) {
					data = malloc(size);
				} else {
					data=realloc(data,size);
				}
			}
			if (data == NULL) {
				perror("malloc or realloc or limit-over");
				rv = -1;
				incoming = false;
			} else {
				/* データ格納 */
				(void) memcpy(&data[current_length], buf, len);
				current_length += len;
				data[current_length] = '\0';
				if (data[current_length-1] == '\n') {
					rv = current_length;
					incoming = false;
				}
			}
		}
	}while(incoming);
    *ret_buf = data;
	(void) fprintf(stderr, "** complete line, len=%zd\n", rv);
	(void) fprintf(stderr, "'%s'\n", *ret_buf);
    return(rv);
}

int
send_recv(struct user *u, int child_no, struct user *users)
{
	//struct user *up = &u;
	char *buf;
	ssize_t len;

	(void) fprintf(stderr, "-----------------------------------\n");
	len = recv_line(u->socket, &buf);
	if (len == -1){ // 受信失敗。無視する
		(void) fprintf(stderr, "error");
		return(0);
	}
	if(len == 0){
		(void) fprintf(stderr, "recv:EOF\n");
		return(-1);
	}
	char *cmd, *body;
	client_cmd_parse(buf, &cmd, &body);

	if(strncmp(cmd, "JOIN", sizeof("JOIN")) == 0){
		fprintf(stderr, "JOIN CMD..\n");
		if(!set_name(u, body)){
			disconnect(u);
		}
		(void) fprintf(stderr, "name= '%s'\n", u->name);
	}else if(strncmp(cmd, "SAY", sizeof("SAY")) == 0){
		fprintf(stderr, "SAY CMD..\n");

		push_to_everybody(users, generate_sayed_cmd(u, body));
		/*
		int send_len = strlen(body);
		fprintf(stderr, "body...'%s', len=%d\n", body, send_len);
		generate_sayed_cmd(u, body);
		int i;
		for (i = 0; i < MAX_CHILD; i++) {
			if(users[i].socket != -1){
				fprintf(stderr, "sending....to [user:%d]\n", users[i].no);
				len = send(users[i].socket, body, (size_t) send_len, 0);
				if (len == -1) {
					perror("send");
					return(-1);
				}
			}
		}
		*/
	}else if(strncmp(cmd, "LEAVE", sizeof("LEAVE")) == 0){
		fprintf(stderr, "LEAVE CMD..\n");
	}
	//free(body);
	free(cmd);
	free(buf);
	return(0);
}
void push_to_everybody(struct user *users, char *msg){
	int i, len;
	int send_len = strlen(msg);
	for (i = 0; i < MAX_CHILD; i++) {
		if(users[i].socket != -1){
			fprintf(stderr, "sending....to [user:%d]\n", users[i].no);
			len = send(users[i].socket, msg, (size_t) send_len, 0);
			if (len == -1) {
				perror("send");
			}
		}
	}
}

void disconnect(struct user *u){
	u->name[0] = '\0';
	(void)close(u->socket);
	u->socket = -1;
}
