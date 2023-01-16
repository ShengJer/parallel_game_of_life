#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>

#define BLOCK_LOW(id, p, n) ((id)*(n)/(p))
#define BLOCK_HIGH(id, p, n) (BLOCK_LOW((id)+1, p, n)-1)
#define BLOCK_SIZE(id, p, n) (BLOCK_LOW((id)+1, p, n)-BLOCK_LOW(id, p ,n))
#define DATA_MSG 0

int main(int argc, char *argv[])
{
	int k; // k denote as maximum iteration
	int j; // j denote as print interation
	int p, id;
	int m, n;
	int i, i0, i1, i2; // index
	int count;
	char file_name[50];
	
	int *local_array;
	int *temp;
	int **local_index_array;
	int **lptr; // pointer to local_index_array
	int *rptr; // pointer to local_array
	
	int *upper_relation;
	int *lower_relation;
	
	int index[2];
	int low_value, high_value, local_size;
	FILE *fr;
	int x;
	double elapsed;
	double max_elapsed;

	// direction
	int up;
	int down;
	int left;
	int right;
	int upper_right;
	int upper_left;
	int lower_left;
	int lower_right;
	// additional variable
	int change;
	
	// MPI I/O
	MPI_File fh;
	
	// function
	int check(int count, int local, int i1, int i2, int n);
	void communication(int iter, int id, int p, int local_size, int ***local_index_array, int **upper_relation, int **lower_relation, int n, MPI_Comm comm, MPI_Status status);
	MPI_Status status;
	
	MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &id);
    MPI_Comm_size(MPI_COMM_WORLD, &p);
	
	if (id == p-1)
	{
		
		// read the array size from file
		fr = fopen("index.out", "rb");
		fread(&index[0], sizeof(int), 2, fr);
		fflush(fr);
		fclose(fr);
		
		printf("After reading the m=%d and n=%d\n", index[0], index[1]);
		
		m = index[0];
		n = index[1];
		
		k = atoi(argv[1]);
		j = atoi(argv[2]);
		printf("After reading from command line, k=%d, j=%d\n", k, j);
		
	}
	MPI_Bcast(&n, 1, MPI_INT, p-1, MPI_COMM_WORLD);
	MPI_Bcast(&m, 1, MPI_INT, p-1, MPI_COMM_WORLD);
	MPI_Bcast(&k, 1, MPI_INT, p-1, MPI_COMM_WORLD);
	MPI_Bcast(&j, 1, MPI_INT, p-1, MPI_COMM_WORLD);
	
	low_value = BLOCK_LOW(id, p, m);
	high_value = BLOCK_HIGH(id, p, m);
	local_size = BLOCK_SIZE(id, p, m);
	//printf("I am id=%d, low_value=%d, high_value=%d, size=%d\n", id, low_value, high_value, local_size);
	
	temp = (int *) malloc(sizeof(int)*(local_size*n));
	local_array = (int *) malloc(sizeof(int)*(local_size*n));
	local_index_array = (int **) malloc(local_size*sizeof(int *));
	upper_relation = (int *) malloc(sizeof(int)*n);
	lower_relation = (int *) malloc(sizeof(int)*n);
	
	if (id == (p-1))
	{
		system("rm -rf output/*");
		fr = fopen("initial_array.out", "rb");
		for (i = 0; i < p-1; i++) 
		{
			fread (local_array, sizeof(int), BLOCK_SIZE(i,p,m)*n, fr);
			/*
			for (i1=0; i1< BLOCK_SIZE(i,p,m)*n; i1++)
			{
				printf("send to id=%d, local_array[%d] = %d\n", i, i1, local_array[i1]);
			}
			*/
			MPI_Send (local_array, BLOCK_SIZE(i,p,m)*n, MPI_INT, i, DATA_MSG, MPI_COMM_WORLD);
		}
		fread (local_array, sizeof(int), BLOCK_SIZE(i,p,m)*n , fr);
		fclose (fr);
		/*
		for(i=0; i<local_size*n; i++)
		{
			printf("I am id=%d, my local_array[%d] = %d\n", id, i, local_array[i]);
		}
		*/
	} 
	else
	{
		MPI_Recv (local_array, local_size*n, MPI_INT, p-1, DATA_MSG, MPI_COMM_WORLD, &status);
		/*
		for(i=0; i<local_size*n; i++)
		{
			printf("I am id=%d, my local_array[%d] = %d\n", id, i, local_array[i]);
		}
		*/
	}
	
	
	lptr = &(local_index_array[0]);
	rptr = local_array;
	//printf("I am id=%d, the initial address of local_array = %d\n", id, rptr);
	for (i = 0; i < local_size; i++) {
	  *(lptr++)= rptr;
	  //printf("I am id=%d, the local_index_array[%d] = %d\n", id, i, local_index_array[i]);
	  rptr += n ; // the integer type have been define so do not need sizeof(int)
	  //printf("I am id=%d, rptr=%d\n",id, rptr);
	   
	}
	
	//initialize the upper and lower array
   	communication(0, id, p, local_size, &local_index_array, &upper_relation, &lower_relation, n, MPI_COMM_WORLD, status);
	
	// remove all previous things in output file;
	
	MPI_Barrier(MPI_COMM_WORLD);
	elapsed = -MPI_Wtime();
	for (i0=1; i0<=k; i0++)
	{
		//iterate through the array
		for (i1=0; i1<local_size; i1++) // local index from 0  to size-1
		{		
			for (i2=0; i2<n; i2++)
			{
				// the case of first row: 
				if (id != 0 && i1==0 && i2==0)
				{
					right = local_array[i1*n+i2+1];
					up = upper_relation[i2];
					upper_right = upper_relation[i2+1];
					down = *(local_index_array[i1+1]);
					lower_right = *(local_index_array[i1+1]+1);
					count = right+up+upper_right+down+lower_right;
				}
				
				else if (id != 0 && i1==0 && i2==n-1)
				{
					left = local_array[i1*n+i2-1];
					up = upper_relation[i2];
					upper_left = upper_relation[i2-1];
					down = *(local_index_array[i1+1]+i2);
					lower_left = *(local_index_array[i1+1]+i2-1);
					count = left+up+upper_left+down+lower_left;
				}
				else if (id != 0 && i1==0)
				{
					left = local_array[i1*n+i2-1];
					right = local_array[i1*n+i2+1];
					up = upper_relation[i2];
					down = *(local_index_array[i1+1]+i2);
					upper_left = upper_relation[i2-1];
					upper_right = upper_relation[i2+1];
					lower_left = *(local_index_array[i1+1]+i2-1);
					lower_right = *(local_index_array[i1+1]+i2+1);
					count = left+right+up+down+upper_left+upper_right+lower_left+lower_right;
				}
				
				else if (id == 0 && i1==0 && i2==0)
				{
					right = local_array[i1*n+i2+1];
					down = *(local_index_array[i1+1]);
					lower_right = *(local_index_array[i1+1]+1);
					count = right + down +lower_right;
				}
				else if (id == 0 && i1==0 && i2==n-1)
				{
					left = local_array[i1*n+i2-1];
					down = *(local_index_array[i1+1]+i2);
					lower_left = *(local_index_array[i1+1]+i2-1);
					count = left + down +lower_left;
				}
				else if (id == 0 && i1==0)
				{
					right = local_array[i1*n+i2+1];
					left = local_array[i1*n+i2-1];
					down = *(local_index_array[i1+1]+i2);
					lower_right = *(local_index_array[i1+1]+i2+1);
					lower_left = *(local_index_array[i1+1]+i2-1);
					count = right + left + down+ lower_right + lower_left;
				}
				
				// the case of last row: 
				if (id != p-1 && i1==local_size-1 && i2==0)
				{
					right = local_array[i1*n+i2+1];
					up = *(local_index_array[i1-1]);
					upper_right = *(local_index_array[i1-1]+1);
					down = lower_relation[i2];
					lower_right = lower_relation[i2+1];
					count = right+up+upper_right+down+lower_right;
				}
				else if (id != p-1 && i1==local_size-1 && i2==n-1)
				{
					left = local_array[i1*n+i2-1];
					up = *(local_index_array[i1-1]+i2);
					upper_left = *(local_index_array[i1-1]+i2-1);
					down = lower_relation[i2];
					lower_left = lower_relation[i2-1];
					count = left+up+upper_left+down+lower_left;
				}
				else if (id != p-1 && i1==local_size-1)
				{
					left = local_array[i1*n+i2-1];
					right = local_array[i1*n+i2+1];
					up = *(local_index_array[i1-1]+i2);
					down = lower_relation[i2];
					upper_left = *(local_index_array[i1-1]+i2-1);
					upper_right = *(local_index_array[i1-1]+i2+1);
					lower_left = lower_relation[i2-1];
					lower_right = lower_relation[i2+1];
					count = left+right+up+down+upper_left+upper_right+lower_left+lower_right;
				}
				
				else if (id == p-1 && i1==local_size-1 && i2==0)
				{
					right = local_array[i1*n+i2+1];
					up = *(local_index_array[i1-1]);
					upper_right = *(local_index_array[i1-1]+1);
					count = right + up + upper_right;
				}
				else if (id == p-1 && i1==local_size-1 && i2==n-1)
				{
					left = local_array[i1*n+i2-1];
					up = *(local_index_array[i1-1]+i2);
					upper_left = *(local_index_array[i1-1]+i2-1);
					count = left + up + upper_left;
				}
				else if (id == p-1 && i1==local_size-1)
				{
					right = local_array[i1*n+i2+1];
					left = local_array[i1*n+i2-1];
					up = *(local_index_array[i1-1]+i2);
					upper_right = *(local_index_array[i1-1]+i2+1);
					upper_left = *(local_index_array[i1-1]+i2-1);
					count = right + left + up+ upper_right + upper_left;
				}
				
				else if (i2==0 && i1 != local_size-1 && i1 != 0)
				{
					up = *(local_index_array[i1-1]);
					upper_right = *(local_index_array[i1-1]+1);
					right = local_array[i1*n+i2+1];
					down = *(local_index_array[i1+1]);
					lower_right = *(local_index_array[i1+1]+1);
					count = up+upper_right+ right+down+lower_right;
				}
				
				else if (i2==n-1 && i1 != local_size-1 && i1 != 0)
				{
					up = *(local_index_array[i1-1]+i2);
					upper_left = *(local_index_array[i1-1]+i2-1);
					left = local_array[i1*n+i2-1];
					down = *(local_index_array[i1+1]+i2);
					lower_left = *(local_index_array[i1+1]+i2-1);
					count = up+upper_left+ left+down+lower_left;
				}
				else if (i2 != n-1&& i2 != 0 && i1 != local_size-1 && i1 != 0)
				{
					left = local_array[i1*n+i2-1];
					right = local_array[i1*n+i2+1];
					up = *(local_index_array[i1-1]+i2);
					down = *(local_index_array[i1+1]+i2);
					upper_left = *(local_index_array[i1-1]+i2-1);
					upper_right = *(local_index_array[i1-1]+i2+1);
					lower_left = *(local_index_array[i1+1]+i2-1);
					lower_right = *(local_index_array[i1+1]+i2+1);
					count = left+right+up+down+upper_left+upper_right+lower_left+lower_right;
				}
				//printf("id=%d, i1=%d, i2=%d, count=%d\n", id, i1, i2, count);
				change = check(count, local_array[i1*n+i2], i1, i2, n);
				temp[i1*n+i2] = change;
			}	
		}
		// updata loop
		for (i1=0; i1<local_size; i1++) // local index from 0  to size-1
		{		
			for (i2=0; i2<n; i2++)
			{
				local_array[i1*n+i2] = temp[i1*n+i2];
			}
		}
		communication(i0, id, p, local_size, &local_index_array, &upper_relation, &lower_relation, n, MPI_COMM_WORLD, status);
		
		// print out the thing every j iteration
		if (i0 % j == 0)
		{
			// gather and initialize composition by MPI/IO
			sprintf(file_name, "output/iter=%d.dat", i0);
			MPI_File_open(MPI_COMM_WORLD, file_name, MPI_MODE_CREATE | MPI_MODE_WRONLY, MPI_INFO_NULL, &fh);
			MPI_File_write_ordered(fh, local_array, local_size*n, MPI_INT, &status);
			MPI_File_close(&fh);
		}
	}
	
	elapsed += MPI_Wtime();
	printf("I am id=%d. I finish after time = %8f", id, elapsed);
	MPI_Reduce(&elapsed, &max_elapsed, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
	if (!id)
	{printf("After the program, the maximum time usage=%8f (s) \n", max_elapsed);}

	
	free(local_index_array);
	free(upper_relation);
	free(lower_relation);
	free(temp);
	free(local_array);
	MPI_Finalize();
	return 0;
}

int check(int count, int local, int i1, int i2, int n)
{
	//printf("count=%d\n", count);
	//printf("local[%d]\n", *local[0]);
	//printf("i1=%d\n", i1);
	//printf("i2=%d\n", i2);
	//printf("I am id=%d, *local[i1*n+i2]=%d\n",id, (*local)[i1*n+i2]);
	//*local[i1*n+i2] = 0;
	
	if (count == 3 && local == 0)
	{
		return 1;
	}
	
	
	else if (count < 2 && local == 1)
	{
		return 0;
	}
	else if (count >3 && local == 1)
	{
		return 0;
	}
	
	else
	{
		return local;
	}

}

void communication(int iter, int id, int p, int local_size, int ***local_index_array, int **upper_relation, int **lower_relation, int n, MPI_Comm comm, MPI_Status status)
{
	int i;
	
	if (id != p-1)
	{
		MPI_Send((*local_index_array)[local_size-1], n, MPI_INT, id+1, DATA_MSG, MPI_COMM_WORLD);
	}
	if (id != 0)
	{
		MPI_Recv(*upper_relation, n, MPI_INT, id-1, DATA_MSG, MPI_COMM_WORLD, &status);
		/*
		for (i=0; i<n; i++)
		{
			printf("After iter = %d, I am id=%d, I receive message upper[%d]=%d\n", iter, id, i, (*upper_relation)[i]);
		}
		*/
	}
	if (id != 0)
	{
		MPI_Send((*local_index_array)[0], n, MPI_INT, id-1, DATA_MSG, MPI_COMM_WORLD);
	}
	if (id != p-1)
	{
		MPI_Recv(*lower_relation, n, MPI_INT, id+1, DATA_MSG, MPI_COMM_WORLD, &status);
		/*
		for (i=0; i<n; i++)
		{
			printf("After iter = %d, I am id=%d, I receive message lower[%d]=%d\n", iter, id, i, (*lower_relation)[i]);
		}
		*/
	}
	
	return;
}