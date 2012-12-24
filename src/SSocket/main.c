#include <stdio.h>
#include <sysexits.h>
#include "server.h"

int
main(int argc, char *argv[])
{
	if(argc != 1){
		fprintf(stderr, "no arguments accpeted.\n");
		return (EX_USAGE);
	}
	start_server();

	return EX_OK;
}
