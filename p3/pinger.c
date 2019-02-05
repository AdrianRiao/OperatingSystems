#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <err.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>


/*Espera por sus hijos y retorna 0 en caso de que todos
  acaben con exito (retorna 1 en caso contrario)*/
int wait_children (void)
{
	int wstatus;
	int children_status = 0;
	
	
	while (wait(&wstatus) != -1){
		/*Si el proceso no ha llamado a exit, o si, en caso
		  de haberlo llamado, el estado de salida no es cero.*/
		if (!WIFEXITED(wstatus) ||
			(WIFEXITED(wstatus) && 
			WEXITSTATUS(wstatus) != 0)) {
				
			children_status = 1;
		}
	}
	
	return children_status;
}


void do_ping (char *ip)
{
	execl("/bin/ping", "ping", "-c 1", "-W 5", ip, NULL);
	errx(1, "ERROR: execl failed");
}


void make_children (int narg, char *argv[])
{
	int i;
	int pid;
	
	for (i = 1; i <= narg; i++) {
		pid = fork();
		
		if (pid == -1) {
			err(1, "ERROR: err failed");
		} else if (pid == 0){
			do_ping(argv[i]);
			exit(EXIT_SUCCESS);
		}
	}
}


int
main(int argc, char *argv[])
{
	int narg = argc - 1;
	int status;
	
	if (narg == 0) {
		printf("There are no arguments\n");
		exit(EXIT_SUCCESS);
	}
	
	make_children (narg, argv);
	status = wait_children();
	
	if (status == 0) {
		exit (EXIT_SUCCESS);
	} else {
		exit (EXIT_FAILURE);
	}

}
