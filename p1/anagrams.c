
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

enum{
	Maxwords = 100,
	Maxanagrams = Maxwords/2,
};


/*--------------------ESTRUCTURA DE DATOS-------------------*/

struct Cell_a{
	char *word;
	struct Cell_a *next;
};

typedef struct Cell_a Cell_a;


struct Listwords_a{
	Cell_a cell[Maxanagrams];
};

typedef struct Listwords_a Listwords_a;



struct List_a{
	int nelements;
	Listwords_a *p_first;
};
	
typedef struct List_a List_a;


/*----------------LIBERACIÓN DE MEMORIA DINÁMICA-------------*/

void
free_memory_cell (Cell_a *cell)
{
	Cell_a *p_aux1, *p_aux2;
	p_aux1 = cell;
	p_aux2 = p_aux1->next;
	
	while (p_aux1->next != NULL){
		
		while (p_aux2->next != NULL){
			p_aux1 = p_aux2;
			p_aux2 = p_aux1->next;
		}
		
		free(p_aux2);
		p_aux1->next = NULL;
		p_aux1 = cell;
		p_aux2 = p_aux1->next;
	}
}


void
free_memory_cells (Listwords_a *listwords, int nelements)
{
	int i;
	for (i = 0; i < nelements; i++){
		free_memory_cell(&listwords->cell[i]);
	}
}


void
free_memory (List_a *list)
{
	free_memory_cells(list->p_first, list->nelements);
	free(list->p_first);
	free(list);
}


/*------------------------ALGORITMO-----------------------*/

int
empty_list (List_a *list)
{
	return list->nelements == 0;
}


int
same_words(char *word1, char *word2)
{
	return !strcmp(word1, word2);
}


int
same_letters (char c1, char c2)
{
	return c1 == c2;
}


/*Indica si la palabra ya ha sido comprobada
  anteriormente y es anagrama de otra*/
int
checked (char *word1)
{
	char *word2 = "\0";
	
	return same_words(word1, word2);
}


/*Devuelve la posicion del char c en la cadena str
  (debemos comprobar previamente que el char está en la cadena*/
int
pos_char (char *str, char c)
{
	int j=0, done=0, pos;
	
	while (!done) {
		if (str[j] == c) {
			pos = j;
			done = 1;
		}
		j = j + 1;
	}
	return pos;
}


/*Indica si está el carácter c en la cadena str*/
int
is_char (char *str, char c)
{
	int j=0, found=0;
	
	while (!found && j < strlen(str)) {
		if (str[j] == c) {
			found = 1;
		}
		j = j + 1;
	}
	return found;
}


/*Comprueba si dos palabras de la misma 
  longitud tienen las mismas letras*/
int
same_chars (char *word1, char *word2)
{
	int i, pos, fulfilled=1;
	char c;
	
	for (i = 0; i < strlen(word1); i++) {
		c = word1[i];
		if (is_char (word2,c)){
			pos = pos_char (word2,c);
			word2[pos] = ' '; /*Inserto caracter blanco para
							   que no se vuelva a comprobar*/
		} else {
			fulfilled = 0;
			break;
		}
	}
	return fulfilled;
}


int
same_length (char *word1, char *word2)
{
	return strlen(word1) == strlen(word2);
}


int
are_anagrams (char *word1, char *word2)
{
	char *aux1;
	char *aux2;
	aux1 = strdup(word1);
	aux2 = strdup(word2);
	
	if (same_length (aux1, aux2) &&
		same_chars (aux1, aux2)) {
		free(aux1);
		free(aux2);
		return 1;
	} else {
		free(aux1);
		free(aux2);
		return 0;
	}
}


int
pos_word (char *word, List_a *list)
{
	int pos, i=0, found=0;
	Listwords_a *listwords = list->p_first;
	
	while (!found) {
		if (same_words(listwords->cell[i].word, word)) {
			pos = i;
			found = 1;
		}
		i = i + 1;
	}
	
	return pos;
}


void
save_word (char *word, List_a *list)
{
	int pos = list->nelements;
	
	if (empty_list (list)) {
		list->p_first = malloc (sizeof(Listwords_a));
	}
	
	list->p_first->cell[pos].word = word; /*Añado al final de la lista*/
	list->p_first->cell[pos].next = NULL;
	list->nelements = list->nelements + 1;
}


void
save_anagram (char *anagram, Cell_a *cell)
{
	Cell_a *p_aux = cell->next;
	
	cell->next = malloc (sizeof(Cell_a));
	cell->next->word = anagram;
	cell->next->next = p_aux;
}


int
have_anagrams (char *word, int pos, char *argv[], int narg)
{
	int i, found = 0;
	
	for (i = pos+1; i <= narg; i++) {
		if (are_anagrams(word, argv[i])) {
			found = 1;
			break;
		}
	}
	
	return found;
}


void
search_anagrams (char *word, int i, 
				 char *argv[], int narg, List_a *list)
{
	int j, pos;
	
	for (j = i+1; j <= narg; j++) {
				
		if (are_anagrams(word, argv[j])) {
			pos = pos_word (word, list);							
			save_anagram (argv[j], &(list->p_first->cell[pos]));
			argv[j] = "\0"; /*Para que no se vuelva a comprobar*/
		}
	} 
}


void
find_anagrams (int narg, char *argv[], List_a *list)
{
	int i;
	
	for (i = 1; i < narg; i++) {
		
		if (have_anagrams(argv[i], i, argv, narg) &&
			!checked(argv[i])) {
				
			save_word (argv[i], list);
			search_anagrams (argv[i], i, argv, narg, list);
		}
	}
}


void
print_misplaced_letters (Cell_a cell)
{
	int i, fulfilled;
	Cell_a *p_aux;

	printf("[");
	for (i = 0; i < strlen(cell.word); i++) {
		fulfilled = 1;
		p_aux = cell.next;
		
		while (fulfilled && p_aux != NULL) {
	
			if (!same_letters(cell.word[i], p_aux->word[i])) {
				fulfilled = 0;
				printf("%c", cell.word[i]);
			}
			
			p_aux = p_aux->next;
		}
	}
	printf("]");
}


void
print_anagrams (Cell_a *cell)
{
	Cell_a *p_aux = cell;
	while (p_aux != NULL) {
		printf("%s ", p_aux->word);
		p_aux = p_aux->next;
	}
}


void
print_all_anagrams (List_a *list)
{
	int i;
	Listwords_a *listwords = list->p_first;
	for (i=0; i < list->nelements; i++)  {
		printf("%s ", listwords->cell[i].word);
		print_anagrams (listwords->cell[i].next);
		print_misplaced_letters (listwords->cell[i]);
		printf("\n");
	}
	
}


int
main(int argc, char *argv[])
{
	int narg = argc - 1;
	
	if (narg == 0) {
		printf("There are no arguments\n");
		exit(EXIT_SUCCESS);
	}
	
	List_a *list;
	list = malloc (sizeof(List_a)); 
	/*Inicializo la lista*/
	list->nelements = 0;
	list->p_first = NULL;
	
	find_anagrams (narg, argv, list);
	print_all_anagrams (list);
	free_memory (list);
	exit(EXIT_SUCCESS);
}
