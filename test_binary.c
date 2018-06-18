#include <stdio.h>
#include <stdint.h>

int main(){


	FILE *fptr;
 	fptr = fopen("bin_file.bin","wb+");
	if (fptr == NULL){
		printf("Error opening file\n");
		return -1;
	}

	uint8_t numbers[4] = {1,3,5,7}, i, read_value;

	for(i=0;i<4;i++)
	{
		fwrite(&numbers[i], sizeof(numbers[i]), 1, fptr);		
	}

	printf("OK\n");
	fseek(fptr,0,SEEK_SET);


	for(i=0;i<4;i++){
		fread(&read_value, sizeof(read_value), 1, fptr);	
		printf("Read value: %u \n", read_value);
	}
	
	fclose (fptr);
}