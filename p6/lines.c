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
	MaxLines = 50,
	MaxPath = 100, //Max len of path
};


int isfile (struct dirent *ent)
{
	return DT_REG == ent->d_type;
}

char *pathname_file (char *str1, char *str2)
{
	char *pathname_file;
  	int sz = MaxPath; 
	
	pathname_file = malloc(sz);
	snprintf(pathname_file, sz, "%s/%s", str1, str2);
	return pathname_file;
}

void create_file (char *path, int *fd)
{
	if ((*fd = open(path,
				   O_CREAT|O_TRUNC|O_WRONLY, 
				   S_IRWXU|S_IRWXG|S_IRWXO)) == -1){
		errx(EXIT_FAILURE, "can´t creat file: %s\n", path);
	}
}

void close_file (char *path, int fd)
{
	if (close(fd) == -1){
		errx(EXIT_FAILURE, "can´t close file: %s\n", path);
	}
}

int ended_txt (char *name)
{
	char c = '.';
	char *end_name;
	
	end_name = rindex(name, c);
	if (end_name == NULL){
		return 0;
	}else{
		return strcmp(end_name, ".txt") == 0;
	}
}

void change_std_out (int fd)
{
	if(dup2(fd, 1) == -1){
		errx(1, "ERROR: dup2 failed");
	}
	if (close(fd) == -1){
		errx(EXIT_FAILURE, "can´t close fd\n");
	}
}

void change_err_out (void)
{
	int fd;
	
	if ((fd = open("/dev/null",O_WRONLY)) == -1){
		errx(EXIT_FAILURE, "can´t open /dev/null\n");
	}
	if(dup2(fd, 2) == -1){
		errx(1, "ERROR: dup2 failed\n");
	}
	if (close(fd) == -1){
		errx(EXIT_FAILURE, "can´t close fd\n");
	}
}

void change_fd (int fd)
{
	change_std_out(fd);
	change_err_out();
}

void fgrep_file (char *dir, char *name_file, char *str, int fd)
{
	int pid;
	char *path;
	
	pid = fork();
	if (pid == -1) {
		err(1, "ERROR: err failed");
	} else if (pid == 0){
		change_fd(fd);
		path = pathname_file(dir, name_file);
		execl("/bin/fgrep", "fgrep", str, path, NULL);
		errx(1, "ERROR: execl failed");
	}
}

void travel_dir (DIR *d, char *dir, char *str, int fd)
{
	struct dirent *ent;
	
	while ((ent = readdir(d)) != NULL){
		if (isfile(ent) && ended_txt(ent->d_name)){
			fgrep_file(dir, ent->d_name, str, fd);
		}
	}
}

void lines_dir (char *str, char *dir, int fd)
{
	DIR *d;
	
	if ((d = opendir (dir)) == NULL){
		errx(EXIT_FAILURE, "dir:%s doesn´t exists\n", dir);
	}
	travel_dir (d, dir, str, fd);
	closedir(d);
}

void finish (char *path)
{
	int wstatus;
	int children_status = 0;
	
	while (wait(&wstatus) != -1){
		if (!WIFEXITED(wstatus) ||
			(WIFEXITED(wstatus) && 
			WEXITSTATUS(wstatus) == 2)) {
			
			children_status = 1;
		}
	}
	if(children_status == 1){
		unlink(path);
		free(path);
		fprintf(stderr, "fgrep failed\n");
		exit(EXIT_FAILURE);
	}else{
		free(path);
		exit(EXIT_SUCCESS);
	}
}

int
main(int argc, char *argv[])
{
	int fd;
	char *path;
	
	if (argc != 3) {
		errx (EXIT_FAILURE, "usage: ./lines str dir");
	}
	
	//path = parent directory + lines.out
	path = pathname_file (argv[2], "../lines.out");
	create_file(path, &fd);
	lines_dir(argv[1], argv[2], fd);
	close_file(path, fd);
	
	finish(path);
}
