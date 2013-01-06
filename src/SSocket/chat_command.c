#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "server.h"
#include <stdbool.h>

void
client_cmd_parse(char *buf, char **cmd, char **body)
{
	char *sp, *np;
	np = strchr(buf, '\n');
	*np = '\0';

	sp = strchr(buf, ' ');
	*sp = '\0';
	sp++;
	*cmd = malloc(sp-buf);
	strcpy(*cmd, buf);
	*body = sp;
}

bool
set_name(struct user *u, char *name)
{
    int name_len = strlen(name);
	if(name_len == 0){
		return(false);
	}
	if(name_len > (NAME_LENGTH-1)){
		name_len = NAME_LENGTH-1;
	}

	strncpy(u->name, name, name_len);
	if(name_len == NAME_LENGTH-1){
		u->name[NAME_LENGTH-1] = '\0';
	}
	fprintf(stderr, "name set %s\n", u->name);
	return(true);
}

char*
generate_sayed_cmd(struct user *u, char *body)
{
	char *cmd;
	char prefix[] = "SAYED ";
	int prefix_len = strlen(prefix);
	int name_len = strlen(u->name)+1;
	int body_len = strlen(body);
	(void) fprintf(stderr, "malloc=%d\n", (prefix_len + name_len + body_len));
	cmd = malloc(prefix_len + name_len + body_len);
	strncat(cmd, prefix, prefix_len);
	(void) fprintf(stderr, "sayed cmd-name: '%s'\n", u->name);
	strncat(cmd, u->name, name_len);
	strncat(cmd, " ", 1);
	strncat(cmd, body, body_len);

	(void) fprintf(stderr, "sayed cmd: '%s'\n", cmd);
	return cmd;
}
