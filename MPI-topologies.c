#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include "mpi.h"

/*Write a program sumMatrix.c that calculates the sum of the elements of
an array A of size nxn fills with random integers between 0 and 9.
▫ We create a topology of processors of pxp size,
where n is divisible by p.
▫ Process P0 initializes and distributes matrix A by
two-dimensional blocks between processes These
make the sum of the elements of the corresponding
block and return the result to P0.
▫ To send the sub-matrices we will use derived types
(MPI_Type_vector)
▫ Run the program with 4, 9, 16 processes for values
of n multiples of 12 (120, 1200, 12000 ...)*/


#define Asize 120
#define ndims 2 

void printMatrix(int *matrix, int size) {
    printf("Printing matrix: \n");
    for (size_t i = 0; i < size; i++)
    {
        for (size_t j = 0; j < size; j++)
        {
            printf(" %d ", matrix[i*size+j]);
        }
        printf("\n");
    }
}
int sumOfElements(int* matrix, int size1, int size2) {
    int sum = 0;
    for (size_t i = 0; i < size1; i++)
    {
        for (size_t j = 0; j < size2; j++)
        {
            sum += matrix[i * size2 + j];
        }
    }
    return sum;
}

int main(int argc, char** argv) {
    srand(time(NULL));
    int arrayA[Asize][Asize];
    int globalSum = 0, localSum=0;
    int rank, size;        // rank of current process and no. of processes
    int i, j;
    int wrap_around[ndims];
    int dims[ndims], coord[ndims];
    dims[0] = dims[1] = 0;
    int cart_rank;

    MPI_Comm comm2D;
    MPI_Status status;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    int NumOfBlocks = Asize / (size - 1);

    //creates two vectors, one for sending, one for receiving submatrix
    //got error if I use only one vector
    MPI_Datatype subMatrix;
    MPI_Datatype subMatrixReceive;
    MPI_Type_vector(Asize, NumOfBlocks, Asize, MPI_INT, &subMatrix);
    MPI_Type_vector(Asize, NumOfBlocks, NumOfBlocks, MPI_INT, &subMatrixReceive);
    MPI_Type_commit(&subMatrix);
    MPI_Type_commit(&subMatrixReceive);

    //Create Cartesian topology of processes of size ndims=2
    MPI_Dims_create(size, ndims, dims);

    if (rank == 0)
        printf("\nRank[%d]/[%d%]: Process dims = [%d x %d] \n",
            rank, size, dims[0], dims[1]);

    wrap_around[0] = wrap_around[1] = 0; //set periodicity
    int reorder = 1;
    int ierr = 0;
    ierr = MPI_Cart_create(MPI_COMM_WORLD, ndims, dims, wrap_around, reorder, &comm2D);
    if(ierr!=0)
        printf("ERROR[%d] creating CART\n", ierr);

    MPI_Cart_coords(comm2D, rank, ndims, coord);

    MPI_Cart_rank(comm2D, coord,&cart_rank);

    printf("\nRank[%d]: cart_rank[%d], my coords = (%d,%d)\n",
        rank, cart_rank, coord[0], coord[1]);
         
    if (rank == 0) {
        for (i = 0; i < Asize; i++) {
            for (j = 0; j < Asize; j++)
                arrayA[i][j] = rand() % 10;
        }
        //printMatrix(&arrayA, Asize);
       
        //sending matrix to other processes by blocks 
        int coords = 0;
        for (size_t i = 1; i < size; i++)
        {
            MPI_Send(&arrayA[0][coords], 1, subMatrix, i, 0, MPI_COMM_WORLD);
            coords += NumOfBlocks;
           
        }
     
    }
    else {
        //needs to be [Asize] [Asize/(size-1)]
        int localArray[Asize][Asize/3];

       MPI_Recv(&(localArray), 1, subMatrixReceive, 0, 0, MPI_COMM_WORLD, MPI_STATUSES_IGNORE);
       
       localSum = sumOfElements(&localArray, Asize, NumOfBlocks);
       printf("Sum of received matrix of rank %d is: %d \n", rank,localSum);

       /*printf("\nReceived matrix of process %d is: \n", rank);
       for (int i = 0; i < Asize; i++) {
           for (int j = 0; j < Asize/(size-1); j++) {
               printf("%d ", localArray[i][j]);
           }
          printf("\n");
       }*/
    }

    //send back sum calculated by each process to root
    MPI_Reduce(&localSum, &globalSum, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        printf("\nGlobal sum is %d", globalSum);
    }

    //got error when I try to free comm2D
   //MPI_Comm_free(comm2D);

    MPI_Finalize();
    return 0;
}