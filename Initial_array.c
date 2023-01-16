#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

int main(int *argc, char *argv[])
{
	int m, n, i, j; // k denote as maximum iteration // j denote as print interation
	int *array;
	int index[2];
	FILE *fw;
	srand(time(NULL));
	
	/*
	printf("What is the number of row (m) for a array?\n");
	scanf("%d", &m);
	printf("What is the number of colume (n) for a array \n?");
	scanf("%d", &n);
	*/
	m = atoi(argv[1]);
	n = atoi(argv[2]);
	printf("After reading from the command line argument row(m) = %d, colume(n) = %d", m, n);
	
	// Dynamic allocation for an array
	array = (int *) malloc(sizeof(int)*(m*n));
	index[0] = m;
	index[1] = n;

	printf("m=%d, n=%d\n", index[0], index[1]);
	
	for (i=0; i<m; i++)
	{
		for (j=0; j<n; j++)
		{
			array[i*n+j] = (int) rand() % 2;
			//printf("the array[%d] = %d\n", i*n+j, array[i*n+j]);
		}
	}	
	
	fw = fopen("initial_array.out", "wb");
	if (fw == NULL)
	{
		printf("Error writing\n");
		exit(1);
	}	
	fwrite(&array[0], sizeof(int), m*n, fw);
	fclose(fw);
	fflush(fw);
	
	fw = fopen("index.out", "wb");
	if (fw == NULL)
	{
		printf("Error writing\n");
		exit(1);
	}	
	fwrite(&index[0], sizeof(int), 2, fw);
	fclose(fw);
	fflush(fw);
	
	
	free(array);
	return 0;
}
