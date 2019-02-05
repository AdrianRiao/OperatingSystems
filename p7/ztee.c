#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <err.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/wait.h>
#include <unistd.h>


enum{
	SizeBuf = 8*1024,
};


void read_pipe(int fd)
{
	int pid;
	int fds[2];
	char buf[SizeBuf];
	int nbytes;
	
	pipe(fds);
	pid = fork();

	if (pid == -1) {
		err(1, "ERROR: err failed");
	} else if (pid == 0){
		close(fds[0]);
		dup2(fds[1], 1);
		close(fds[1]);
		execl("/bin/gunzip", "gunzip", NULL);
		errx(1, "ERROR: gunzip failed");
	} else {
		close(fds[1]);
		while ((nbytes = read(fds[0], buf, SizeBuf)) != 0){
			if(write(fd, buf, nbytes) != nbytes){
				errx(EXIT_FAILURE, "can´t write in file\n");
			}
			if(write(1, buf, nbytes) != nbytes){
				errx(EXIT_FAILURE, "can´t write in file\n");
			}
		}
		close(fds[0]);
	}
}


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


int
main(int argc, char *argv[])
{
	int status;
	int fd;
	
	if (argc != 2) {
		errx (EXIT_FAILURE, "usage: ./ztee outfile");
	}
	
	fd = open(argv[1], O_CREAT|O_TRUNC|O_WRONLY, S_IRWXU|S_IRWXG|S_IRWXO);
	if (fd == -1){
		errx(EXIT_FAILURE, "can´t creat file: %s\n", argv[1]);
	}
	
	read_pipe(fd);
	
	if (close(fd) == -1){
		errx(EXIT_FAILURE, "can´t close file: %s\n", argv[1]);
	}
	
	status = wait_children();
	if (status == 0) {
		exit (EXIT_SUCCESS);
	} else {
		exit (EXIT_FAILURE);
	}
}
