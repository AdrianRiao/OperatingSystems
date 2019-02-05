#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <err.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>


/*--------------------------CONSTANTES------------------------*/
enum{
	Maxarg = 50,
};

/*--------------------ESTRUCTURA DE DATOS-------------------*/


struct Cell_a{
	int pid;
	int unique;
	int empty;
};

typedef struct Cell_a Cell_a;

struct List_a{
	Cell_a cell[Maxarg];
};

typedef struct List_a List_a;

	List_a list;

/*-------------------------ALGORITMO-----------------------*/


int are_wrong_arg (int narg)
{
	if (narg == 0) {
		printf("There are no arguments\n");
		return 1;
	} else if (narg > Maxarg) {
		printf("Too many arguments\n");
		return 1;
	} else {
		return 0;
	}
}


void initialize_list (void)
{
	int i;
	
	for (i = 0; i < Maxarg; i++){
		list.cell[i].empty = 1;
	}
}


int is_empty_cell (Cell_a cell)
{
	return cell.empty == 1;
}


/* Devuelve la primera posición vacía de la lista*/
int empty_pos (void)
{
	int i;
	int pos;
	
	for (i = 0; i < Maxarg; i++){
		if (is_empty_cell(list.cell[i])) {
			pos = i;
			break;
		}
	}
	return pos;
}


/* Devuelve la posición de la lista en la que está pid*/
int pid_pos (int pid)
{
	int i;
	int pos;
	
	for (i = 0; i < Maxarg; i++){
		if (list.cell[i].pid == pid) {
			pos = i;
			break;
		}
	}
	return pos;
}


void save_pid (int pid)
{
	int pos;
	
	pos = empty_pos();
	list.cell[pos].pid = pid;
	list.cell[pos].empty = 0;
}


void save_status (int pid, int status)
{
	int pos;
	
	pos = pid_pos(pid);
	if (status == 0){
		list.cell[pos].unique = 1;
	} else {
		list.cell[pos].unique = 0;
	}
}


void print_results (char *files[])
{
	int wstatus;
	int pid;
	int i;
	
	/*Recojo y guardo el estado de los hijos*/
	pid = wait(&wstatus);
	while (pid != -1){
		if (WIFEXITED(wstatus)) {
			save_status (pid, WEXITSTATUS(wstatus));
		}
		pid = wait(&wstatus);
	}
	
	/*Recorro la lista e imprimo*/
	for (i = 0; i < Maxarg; i++){
		if (list.cell[i].empty == 0) {
			printf("%s ", files[i+1]);
			if (list.cell[i].unique == 1){
				printf("yes \n");
			} else {
				printf("no \n");
			}
		} else {
			break;
		}
	}
}


void cmp_files (char *file1, char *file2)
{
	execl("/usr/bin/cmp", "cmp", "-s", file1, file2, NULL);
	errx(1, "ERROR: execl failed");
}


int is_file_unique 
	(char *file, char *files[], int posfile, int nfiles)
{
	int i;
	int pid;
	int wstatus;
	int unique = 1;
	
	for (i = 1; i <= nfiles; i++) {
		if (i == posfile){
			continue;
		}
		pid = fork();
			
		if (pid == -1) {
			err(1, "ERROR: err failed");
		} else if (pid == 0){
			cmp_files (file, files[i]);
			exit(EXIT_SUCCESS);
		}
	}
	
	while (wait(&wstatus) != -1){
		/*Si el proceso ha llamado a exit y 
		 * el estado de salida es cero.*/
		if (WIFEXITED(wstatus) && WEXITSTATUS(wstatus) == 0) {
			unique = 0;
		}
	}
	return unique;
}


void cmp_all_files (int nfiles, char *files[])
{
	int i;
	int pid;
	
	for (i = 1; i <= nfiles; i++) {
		pid = fork();
		
		if (pid == -1) {
			err(1, "ERROR: err failed");
			
		} else if (pid == 0){
			if (is_file_unique(files[i], files, i, nfiles)){
				exit(EXIT_SUCCESS);
			} else {
				exit(EXIT_FAILURE);
			}
			
		} else {
			save_pid (pid);
		}
	}
}


int
main(int argc, char *argv[])
{
	int narg = argc - 1;
	
	if (are_wrong_arg(narg)){
		exit(EXIT_FAILURE);
	}
	
	initialize_list();
	cmp_all_files (narg, argv);
	print_results(argv);
	
	exit(EXIT_SUCCESS);

}
