#define NAME_LENGTH  (20)
#define MAX_CHILD    (20)
#define PORT    "11223"

struct user {
	int no;
	int socket;
	char name[NAME_LENGTH];
};

int start_server();
int server_socket(const char *portnm);
void accept_loop(int soc);
ssize_t recv_by_byte(int socket, char *buf, size_t bufsize);
ssize_t recv_line(int socket, char **ret_buf);
int send_recv(struct user *u, int child_no, struct user *childs);
void push_to_everybody(struct user *users, char *msg);
void disconnect(struct user *u);
