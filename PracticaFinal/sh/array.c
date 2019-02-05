#include <string.h>
#include <err.h>
#include "array.h"

//Move the slice that goes from first to last acoording indicated pos
void mv_slice(char *arr[], int last, int first, int pos, int len)
{
	int i;
	
	if(last+pos > len-1){
		errx(1, "Argv data structure isn´t enough to expand");
	}
	for(i = last; i >= first; --i){
		arr[i+pos] = arr[i];
	}
}

void ins_slice(char *arr[], char *slice[], int nitems, int pos)
{
	int i;
	
	for(i = 0; i < nitems; i++){
		arr[pos+i] = strdup(slice[i]);
	}
}
