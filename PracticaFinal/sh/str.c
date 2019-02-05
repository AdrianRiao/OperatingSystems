#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "str.h"

void sustlchr(char *ptr, char oldchr, char newchr)
{
	ptr = strrchr(ptr, oldchr);
	*ptr = newchr;
}

void sustchr(char *str, char oldchr, char newchr)
{
	char *ptr;
	
	while((ptr = strchr(str, oldchr)) != NULL){
		*ptr = newchr;
	}
}

int nchr(char *str, char c)
{
	int cont;
	int i;
	
	cont = 0;
	for(i = 0; i < strlen(str); i++){
		if(str[i] == c){
			cont++;
		}
	}
	return cont;
}

void rm_lspaces(char *ptr)
{
	ptr = strchr(ptr, '\0');
	--ptr;
	while(*ptr == ' ' || *ptr == '	'){
		*ptr = '\0';
		--ptr;
	}
}

char *rm_fspaces(char *ptr)
{
	while(*ptr == ' ' || *ptr == '	'){
		ptr++;
	}
	return strdup(ptr);
}

char *rm_spaces(char *str)
{
	char *newstr;
	
	newstr = rm_fspaces(str);
	rm_lspaces(newstr);
	return newstr;
}

char *itoc(int i, int len)
{
	char *value;
	
	value = malloc(len);
	snprintf(value, len, "%d", i);
	return value;
}

void replace(char **line, char *oldstr, char *newstr, int len)
{
	char *aux;
	char *newline;
	
	newline = malloc(len);
	aux = strstr(*line, oldstr);
	aux[0] = '\0';
	aux++;
	aux = strpbrk(aux, " 	");
	if(aux == NULL){
		snprintf(newline, len, "%s%s", *line, newstr);
	}else{
		snprintf(newline, len, "%s%s%s", *line, newstr, aux);
	}
	free(*line);
	*line = newline;
}

int is_fword(char *str, char *word)
{
	int success;
	
	str = strdup(str);
	str = strsep(&str, " 	");
	if(strcmp(str, word) == 0){
		success = 1;
	}else{
		success = 0;
	}
	free(str);
	return success;
}

int is_lword(char *ptr, char *word)
{
	if(nchr(ptr, ' ') != 0 ||
	   nchr(ptr, '	') != 0){
		ptr = strchr(ptr, '\0');
		while(*ptr != ' ' && *ptr != '	'){
			--ptr;
		}
		ptr++;
	}
	return strcmp(ptr, word) == 0;
}

void rm_lword(char *ptr)
{
	if(nchr(ptr, ' ') != 0 ||
	   nchr(ptr, '	') != 0){
		ptr = strchr(ptr, '\0');
		while(*ptr != ' ' && *ptr != '	'){
			--ptr;
		}
		ptr++;
	}
	*ptr = '\0';
}
