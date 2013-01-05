struct user {
	int no;
	int accept;
	char name[20];
};

int start_server();
int server_socket(const char *portnm);
void accept_loop(int soc);
int send_recv(int acc, int child_no, struct user *childs);
