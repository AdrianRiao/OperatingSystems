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
	MaxFiles = 50,
	LenInt = 11 , // max len of an integer in a string
	ExtraChars = 3, // extra chars for line len:  \0, \n and tab
};


struct Data_file{
	int nzeros;
	char *file;
	int full;
};

typedef struct Data_file Data_file;


struct List{
	Data_file cell[MaxFiles];
};

typedef struct List List;


void free_memory (List *list)
{
	int i;
	
	for (i = 0; i < MaxFiles; i++){
		if (list->cell[i].full == 1) {
			free(list->cell[i].file);
		} else {
			break;
		}
	}
}


int isfile (struct dirent *ent)
{
	return DT_REG == ent->d_type;
}


void initialize_list (List *list)
{
	int i;
	
	for (i = 0; i < MaxFiles; i++){
		list->cell[i].full = 0;
	}
}


/*devuelve el pathname dado el directorio y el fichero*/
char * pathname_file (char *dir, char *name_file)
{
	char *pathname;
	char aux[] = "/";
	
	pathname = malloc (strlen(dir) + strlen(aux) + strlen(name_file)+2);
	pathname[0] = 0;
	
	strcat(pathname, dir); /*[dir]*/
	strcat(pathname, aux); /*[dir/]*/
	strcat(pathname, name_file); /*[dir/file]*/
	return pathname;
}


int nzeros_buf (char *buf, int nbytes)
{
	int nzeros = 0;
	int i;
	
	for (i = 0; i < nbytes; i++) {
		if(buf[i] == 0){
			nzeros++;
		}
	}
	
	return nzeros;
}


int nzeros_file (struct dirent *ent, char *dir)
{
	int fd;
	int nzeros = 0;
	int nbytes;
	char buf[SizeBuf];
	char *pathname;
	
	pathname = pathname_file (dir, ent->d_name);
	
	/*Abro fichero*/
	if ((fd = open(pathname,O_RDONLY)) == -1){
		errx(EXIT_FAILURE, "can´t open file: %s\n", ent->d_name);
	}

	/*Cuento número de ceros*/
	while ((nbytes = read (fd, buf, SizeBuf)) != 0){
		nzeros = nzeros + nzeros_buf(buf, nbytes);
	}
	
	/*Cierro fichero*/
	if (close(fd) == -1){
		errx(EXIT_FAILURE, "can´t close file: %s\n", ent->d_name);
	}
	
	free(pathname);
	return nzeros;
}


/* Devuelve la primera posición vacía de la lista*/
int empty_pos (List *list)
{
	int i;
	int pos;
	
	for (i = 0; i < MaxFiles; i++){
		if (list->cell[i].full == 0) {
			pos = i;
			break;
		}
	}
	return pos;
}


void save_nzeros (int nzeros, char *name_file, List *list)
{
	int pos;
	
	pos = empty_pos(list);
	list->cell[pos].nzeros = nzeros;
	list->cell[pos].file = strdup (name_file);
	list->cell[pos].full = 1;
}


void travel_dir (DIR *d, char *dir, List *list)
{
	struct dirent *ent;
	int nzeros;
	
	while ((ent = readdir(d)) != NULL){
		if (isfile(ent)){
			nzeros = nzeros_file(ent, dir);
			save_nzeros(nzeros, ent->d_name, list);
		}
	}
}


void zcount_dir (char *dir, List *list)
{
	DIR *d;
	
	/*Abro directorio*/
	if ((d = opendir (dir)) == NULL){
		errx (EXIT_FAILURE, "dir:%s doesn´t exists\n", dir);
	}
	
	/*Recorro directorio*/
	travel_dir (d, dir, list);
	
	/*Cierro directorio*/
	closedir(d);
}


char *data_file (int nzeros, char *name_file)
{
	char *data;
  	int sz = LenInt + ExtraChars + strlen(name_file); 
	
	data = malloc(sz);	
	snprintf(data, sz, "%d	%s\n", nzeros, name_file);
	return data;
}


void write_data(int nzeros, char *name_file, int fd)
{
	char *buf;
	
	buf = data_file(nzeros, name_file);
	
	if ((write(fd, buf, strlen(buf))) != strlen(buf)){
		errx(EXIT_FAILURE, "can´t write in z.txt\n");
	}
	free(buf);
}


void save_data (char *dir, List list)
{
	int i;
	int fd;
	char *pathname;
	
	pathname = pathname_file (dir, "z.txt");
	
	/*Creo fichero*/
	if ((fd = open(pathname, 
				   O_CREAT|O_TRUNC|O_WRONLY, 
				   S_IRWXU|S_IRWXG)) == -1){
		errx(EXIT_FAILURE, "can´t creat file: %s\n", pathname);
	}
	
	for (i = 0; i < MaxFiles; i++){
		if (list.cell[i].full == 1) {
			write_data(list.cell[i].nzeros, 
					   list.cell[i].file,
					   fd);
		} else {
			break;
		}
	}
	
	/*Cierro fichero*/
	if (close(fd) == -1){
		errx(EXIT_FAILURE, "can´t close file: %s\n", pathname);
	}
	
	free(pathname);
}


int
main(int argc, char *argv[])
{
	List list;
	
	if (argc != 2) {
		errx (EXIT_FAILURE, "usage: ./zcount dir");
	}
	
	initialize_list(&list);
	zcount_dir (argv[1], &list);
	save_data(argv[1], list); /*Guarda los datos en z.txt*/
	free_memory (&list);
	
	exit(EXIT_SUCCESS);
}
