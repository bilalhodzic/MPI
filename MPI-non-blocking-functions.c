#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include "mpi.h"

#define SlaveSize 100
#define MasterSIze 100000

long int globalSum(int* vector, int size) {
    long int sum = 0;
    for (int i = 0; i < size; i++) {
        sum += vector[i];
    }
    return sum;
}

int ElementsSum(int vector[], int size) {
    int sum = 0;
    for (int i = 0; i < size; i++) {
        sum += vector[i];
    }
    return sum;
}

int main(int argc, char** argv[]) {
    srand(time(NULL)); 

    int MainVector[MasterSIze], vector2[SlaveSize];
    int i, j, rank, size; 
    int root = 0;
 
    int counter = 0;

    MPI_Init(&argc, &argv);
    MPI_Request req1, req2;
    MPI_Status status1, status2;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    for (int i = 0; i < SlaveSize; i++)
    {
        vector2[i] = 0;
    }

    if (rank == 0) {

        int MainSum = 0;
        int slaveSum = 0;

        int VectorSum[MasterSIze / SlaveSize];
        int TotalSum[MasterSIze / SlaveSize];

        for (i = 0; i < MasterSIze; i++) {
            MainVector[i] = rand() % 10001;
            if ((i + 1)% SlaveSize == 0) {
                MPI_Isend(&MainVector[i +1 - SlaveSize], SlaveSize, MPI_INT,
                    counter % (size - 1) + 1, 0, MPI_COMM_WORLD, &req1);

                MainSum = ElementsSum(&MainVector[i +1- SlaveSize], SlaveSize);
                TotalSum[counter] = MainSum;

                printf("\nGlobal Sum: %d\n", MainSum);
                MPI_Wait(&req1, &status1);
                MPI_Irecv(&VectorSum[counter], 1, MPI_INT, counter % (size - 1) + 1, 0, MPI_COMM_WORLD, &req2);
                
                MPI_Wait(&req2, &status2);
                printf("Sum of rank %d is: %d\n", counter % size + 1, VectorSum[counter]);
                counter++;
            }

        }
       

        printf("\n\nGlobal Sum:  %d\n", globalSum(TotalSum, counter));
        printf("Slave Sum:   %d\n\n", globalSum(VectorSum, counter));

        
    }
    else {
        long int partialSum = 0;

        for (i = rank; i <= MasterSIze/SlaveSize; i += size - 1) {
            MPI_Irecv(&vector2[0], SlaveSize, MPI_INT, 0, 0, MPI_COMM_WORLD, &req1);

            MPI_Wait(&req1, &status1);
            partialSum = ElementsSum(vector2, SlaveSize);

            MPI_Isend(&partialSum, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &req2);
            MPI_Wait(&req2, &status2);
        }
    }

    MPI_Finalize();
    return 0;
}