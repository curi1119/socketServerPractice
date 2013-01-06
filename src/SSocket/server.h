struct user {
	int no;
	int socket;
	char name[20];
};

int start_server();
int server_socket(const char *portnm);
void accept_loop(int soc);
ssize_t recv_by_byte(int socket, char *buf, size_t bufsize);
ssize_t recv_line(int socket, char **ret_buf);
int send_recv(int acc, int child_no, struct user *childs);
