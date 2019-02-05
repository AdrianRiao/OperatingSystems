/* 
 * This program reads from standard input to
 * demultiplex blocks of x bytes in several files
 */
 
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

//-------------------DATA TYPES AND ENUMS-----------------//

enum{
	Maxfds = 100,
	SzBuf = 8*1024,
};

struct Cell_a{
	FILE *strm;
};
typedef struct Cell_a Cell_a;

struct List_a{
	Cell_a cell[Maxfds];
	int nelem;
};
typedef struct List_a List_a;

//-----------------------CODE----------------------------//

void init_list (List_a *pipes)
{
	pipes->nelem = 0;
}

FILE *first (List_a pipes)
{
	if(pipes.nelem == 0){
		return NULL;
	}
	return pipes.cell[0].strm;
}

int pos_strm(List_a pipes, FILE *current)
{
	int i;
	int pos;
	FILE *strm;
	
	for(i = 0; i < pipes.nelem; i++){
		strm = pipes.cell[i].strm;
		if (strm == current){
			pos = i;
			break;
		}
	}
	return pos;
}

FILE *next_pipe (List_a pipes, FILE *current)
{
	int pos;
	
	if(pipes.nelem == 0){
		return NULL;
	}
	pos = pos_strm(pipes, current);
	if(pos == pipes.nelem-1){
		return pipes.cell[0].strm;
	}else{
		return pipes.cell[pos+1].strm;
	}
}

void save_pipe (List_a *pipes, int fd)
{
	int pos;
	FILE *strm;
	
	strm = fdopen(fd, "w");
	if (strm == NULL){
		errx(1, "fdopen failed\n");
	}
	pos = pipes->nelem;
	pipes->cell[pos].strm = strm;
	pipes->nelem = pipes->nelem + 1;
}

void close_files (List_a *pipes)
{
	int i;
	FILE *strm;
	
	for(i = 0; i < pipes->nelem; i++){
		strm = pipes->cell[i].strm;
		if (fclose(strm) != 0){
			err(EXIT_FAILURE, "can´t close file\n");
		}
	}
	pipes->nelem = 0;
}

/*Creates a pipe that conect the process with a file through gzip.
 * Returns the corresponding fd from pipe. */
int new_pipe (int out, List_a *pipes)
{
	int pid;
	int fds[2];
	
	pipe(fds);
	pid = fork();
	if (pid == -1) {
		errx(1, "ERROR: fork failed\n");
	} else if (pid == 0){
		dup2(out, 1);
		dup2(fds[0], 0);
		close(out);
		close(fds[0]);
		close(fds[1]);
		close_files(pipes);
		execl("/bin/gzip", "gzip", NULL);
		errx(1, "ERROR: gzip failed\n");
	}
	close(fds[0]);
	return fds[1];
}

void write_in_files (char data[], int lendata, List_a pipes, 
					 int sz, FILE **strm, int *nbytes)
{
	while(lendata != 0){
		if (*nbytes > lendata){
			if(fwrite(data, 1, lendata, *strm) != lendata){
				errx(1, "ERROR: can't write in file\n");
			}
			*nbytes = *nbytes - lendata;
			break;
		}
		if(fwrite(data, 1, *nbytes, *strm) != *nbytes){
			errx(1, "ERROR: can't write in file\n");
		}
		data = data + *nbytes;
		lendata = lendata - *nbytes;
		*nbytes = sz;
		*strm = next_pipe(pipes, *strm);
	}
}

void demux (int sz, List_a pipes)
{
	char buf[SzBuf];
	int lendata; //num of bytes that buff have readed
	FILE *strm; //first file where I have to write
	int nbytes; //num of bytes that I have to write in first file
	
	strm = first(pipes);
	nbytes = sz;
	while((lendata = fread(buf, 1, SzBuf, stdin)) != 0){
		write_in_files(buf, lendata, pipes, sz, &strm, &nbytes);
	}
	if(ferror(stdin)){
		errx(1, "fread failed\n");
	}
}

/*Wait for children. Returns 0 when all children 
 * have ended successfully. otherwise returns 1.*/
int wait_children (void)
{
	int wsts;
	int children_sts = 0;
	
	while (wait(&wsts) != -1){
		if (!WIFEXITED(wsts) ||
			(WIFEXITED(wsts) && 
			WEXITSTATUS(wsts) != 0)) {
				
			children_sts = 1;
		}
	}
	return children_sts;
}

int
main(int argc, char *argv[])
{
	List_a pipes;
	int sts;
	int i;
	int fd;
	int pipe_fd;
	
	if (argc < 3){
		errx(EXIT_FAILURE, "usage: ./demux sizeblock file1 file2 ...");
	}
	if (argc > Maxfds+2){
		errx(EXIT_FAILURE, "Too many arguments");
	}
	
	init_list(&pipes);
	for(i = 2; i < argc; i++){
		fd = open(argv[i], O_CREAT|O_TRUNC|O_WRONLY, S_IRWXU|S_IRWXG|S_IRWXO);
		if (fd == -1){
			err(EXIT_FAILURE, "can´t creat file: %s\n", argv[i]);
		}
		pipe_fd = new_pipe(fd, &pipes);
		save_pipe(&pipes, pipe_fd);
		if (close(fd) == -1){
			errx(EXIT_FAILURE, "can´t close file: %s\n", argv[i]);
		}
	}
	demux(atoi(argv[1]), pipes);
	close_files(&pipes);

	sts = wait_children();
	exit(sts);
	
}
