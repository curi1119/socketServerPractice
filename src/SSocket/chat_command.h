void client_cmd_parse(const char *buf, char **cmd, char **body);
bool set_name(struct user *u, char *name);
char* generate_sayed_cmd(struct user *u, char *body);
