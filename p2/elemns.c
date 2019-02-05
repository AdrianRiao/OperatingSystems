#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <err.h>


void print_value (char *value)
{
	char *token, *p;
	
	token = strtok_r(value, ":", &p);
	while (token != NULL) {
		printf ("%s\n", token);
		token = strtok_r(NULL, ":", &p);
	}
}

int
main(int argc, char *argv[])
{
	int narg = argc - 1, i;
	char *value;
	
	if (narg == 0) {
		printf("There are no arguments\n");
		exit(EXIT_SUCCESS);
	}
	
	for (i = 1; i <= narg; i++) {
		value = getenv (argv[i]);
		
		if (value == NULL){
			errx(1, "ERROR: var %s does not exist\n", argv[i]);
		} else {
			print_value (value);
		}
	}
}
