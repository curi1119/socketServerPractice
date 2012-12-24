/* 最大同時処理数 */

int start_server();
int server_socket(const char *portnm);
void accept_loop(int soc);
void send_recv_loop(int acc);
//int send_recv(int acc, int child_no);
