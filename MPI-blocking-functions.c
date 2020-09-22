#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include "mpi.h"

/*You have to write a program with the following functionalities:
1. It works with 2, 4 and 5 processes.
2. The master process has a 10000x10000 matrix with random values from 0 to 100.
3. The master process distributes the matrix among all the processes in equal parts
including itself.
4. All processes compute the maximum of the received elements.
5. All processes send the computed maximum to all the processes; every process
knows the maximum of the matrix (the global maximum is computed).
6. All processes update their matrix elements (the ones received in step 3) by dividing
them by the global maximum (as a result, the values are normalized between 0 and
1).
7. All processes send each updated matrix region to the master process.
8. You need to measure the execution time from just before the matrix distribution (step
3) until the master receives the updated matrix (step 7).
9. You have to run 5 times the program (use the mean of these 5 runs) with different
processes (2, 4 and 5) and compare the results.
10. Write a short report with the obtained results, tables and plots with the mean obtained
times in order to compare the obtained times for different number of processes. How
many processes do you need to get the best results?
Upload both the program and the report to the moodle.
The program must be done individually.
You must use pointers.*/


#define gridsize 10000
int malloc2D(int*** array, int n, int m) {
    int i;
    /* allocate the n*m contiguous items */
    int* p = malloc(n * m * sizeof(int));
    if (!p) return -1;

    /* allocate the row pointers into the memory */
    (*array) = malloc(n * sizeof(int*));
    if (!(*array)) {
        free(p);
        return -1;
    }

    /* set up the pointers into the contiguous memory */
    for (i = 0; i < n; i++)
        (*array)[i] = &(p[i * m]);

    return 0;
}

int free2D(int*** array) {
    /* free the memory - the first element of the array is at the start */
    free(&((*array)[0][0]));

    /* free the pointers into the memory */
    free(*array);

    return 0;
}

int main(int argc, char** argv) {
    srand(time(NULL));
    int** global, ** local;
    int rank, size;        // rank of current process and no. of processes
    int i, j, p;
    global = NULL,local=NULL;

    MPI_Status status;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    
    int max = 0, globalMax=0;

    double start, end;

    //start time of executing program
    MPI_Barrier(MPI_COMM_WORLD);
    start = MPI_Wtime();

    if (rank == 0) {
        /* fill in the array, and print it */
        malloc2D(&global, gridsize, gridsize);
        for (i = 0; i < gridsize; i++) {
            for (j = 0; j < gridsize; j++)
                global[i][j] = rand() % 100;
        }
    }

    /* create the local array which we'll process */
    malloc2D(&local, gridsize, gridsize/size );

    /* create a datatype to describe the subarrays of the global array */
    int sizes[2] = { gridsize, gridsize };         /* global size */
    int subsizes[2] = { gridsize , gridsize / size };     /* local size */
    int starts[2] = { 0,0 };                        /* where this one starts */
    MPI_Datatype type, subarrtype;
    MPI_Type_create_subarray(2, sizes, subsizes, starts, MPI_ORDER_C, MPI_INT, &type);
    MPI_Type_create_resized(type, 0, gridsize / size * sizeof(int), &subarrtype);
    MPI_Type_commit(&subarrtype);

    int* globalptr = NULL;
    if (rank == 0)
        globalptr = &(global[0][0]);

    /* scatter the array to all processors */
    int** sendcounts, ** displs;
    malloc2D(&displs, size, size);
    malloc2D(&sendcounts, size, size);

    if (rank == 0) {
        for (i = 0; i < size * size; i++)
            sendcounts[i] = 1;
        int disp = 0;
        for (i = 0; i < size; i++) {
            for (j = 0; j < size; j++) {
                displs[i * size + j] = disp;
                disp += 1;
            }
            disp += ((gridsize / size) - 1) * size;
        }
    }

    MPI_Scatterv(globalptr, sendcounts, displs, subarrtype, &(local[0][0]),
        gridsize * gridsize / size, MPI_INT,
        0, MPI_COMM_WORLD);


    /* now all processors print their local data: */
    //printf("Local process on rank %d is:\n", rank);
    for (i = 0; i < gridsize ; i++) {
        for (j = 0; j < gridsize/ size; j++) {

            //printf("%2d ", local[i][j]);

            if (local[i][j] > max) {
                max = local[i][j];
            }
        }
        //printf("\n");
             
    }
    printf("Max number of rank %d is equal to %d \n", rank, max);
               

    int localMax[5];
    localMax[rank] = max;
    for (int i = 0; i < size; i++)
    {
        MPI_Bcast(&localMax[i], 1, MPI_INT, i, MPI_COMM_WORLD);
    }
    
    for (size_t i = 0; i < size; i++)
    {
        if (localMax[i] > globalMax)
            globalMax = localMax[i];
    }
  
    if (rank == 0) {
        printf("Global max number is equal to %d \n", globalMax);

    }

    /* now all processors print their local updated data: */

  //printf("Local updated process on rank %d is:\n", rank);
    for (i = 0; i < gridsize; i++) {
        for (j = 0; j < gridsize / size; j++) {

            local[i][j] /= globalMax;
            //printf("%2d ", local[i][j]);

        }
        //printf("\n");

    }

    /* it all goes back to process 0 */
    MPI_Gatherv(&(local[0][0]), gridsize * gridsize / size , MPI_INT,
        globalptr, sendcounts, displs, subarrtype,
        0, MPI_COMM_WORLD);

    //end of the executing time
    MPI_Barrier(MPI_COMM_WORLD);
    end = MPI_Wtime();

    /* don't need the local data anymore */
    free2D(&local);

    /* or the MPI data type */
    MPI_Type_free(&subarrtype);

    if (rank == 0) {
        printf("Processed grid:\n");
        for (i = 0; i < gridsize; i++) {
            for (j = 0; j < gridsize; j++) {
                //printf("%2d ", global[i][j]);
            }
            //printf("\n");
        }
        
        free2D(&global);
    }
   
    if (rank == 0) {
        printf("\nRuntime = %f\n", end - start);
    }
   
    MPI_Finalize();
    return 0;
}