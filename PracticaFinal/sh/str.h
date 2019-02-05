
void sustlchr(char *ptr, char oldchr, char newchr);

void sustchr(char *str, char oldchr, char newchr);

int nchr(char *str, char c);

//Remove last leftover spaces from the string
void rm_lspaces(char *ptr);

//Returns a string without first leftover spaces
char *rm_fspaces(char *ptr);

//Returns a string without leftover spaces
char *rm_spaces(char *str);

char *itoc(int i, int len);

//Replaces the first occurrence of oldstr by newstr
void replace(char **line, char *oldstr, char *newstr, int len);

int is_fword(char *str, char *word);

int is_lword(char *ptr, char *word);

void rm_lword(char *ptr);
