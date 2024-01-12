#include <bits/stdc++.h>
#include <mpi.h>

using namespace std;

// Global variables for the grid boundaries
double minx = -1.5, maxx = 1.0, miny = -1.0, maxy = 1.0;

// Function to check if a point is in the Multibrot set
int isMultibrot(const complex<double> &c, int maxIterations, int exponent)
{
    complex<double> z(0, 0);
    for (int i = 0; i < maxIterations; ++i)
    {
        z = pow(z, exponent) + c;
        if (abs(z) > 2.0)
        {
            return 0;
        }
    }
    return 1;
}

// Function to generate the grid using MPI parallel programming
void generateMultibrotGridMPI(int N, int M, int D, int K, int rank, int size, vector<vector<int>> &grid)
{
    // Calculate the step size
    double stepx = (maxx - minx) / (double)(N - 1);
    double stepy = (maxy - miny) / (double)(M - 1);

    // Distribute rows among MPI processes
    int rowsPerProcess = M / size;
    int startRow = rank * rowsPerProcess;
    int endRow = (rank == size - 1) ? M : startRow + rowsPerProcess;

    for (double i = startRow; i < endRow; ++i)
    {
        double imagPart = maxy - (i * stepy);
        for (double j = 0; j < N; ++j)
        {
            double realPart = minx + (j * stepx);
            complex<double> c(realPart, imagPart);

            // Check if the point is in the Multibrot set after K iterations
            int result = isMultibrot(c, K, D);
            grid[i][j] = result;    
        }
    }
}

int main(int argc, char *argv[])
{
    // Initialize MPI
    MPI_Init(&argc, &argv);

    int rank, size;
    int N, M, D, K;
    // You are expected to output a Grid of M x N
    // N: Number of columns
    // M: Number of rows
    // D: Exponent
    // K: Maximum number of iterations

    // Get the rank and size of the MPI world
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Process 0 reads the input, computes its part and produces the output
    // Other processes receive their part of the grid from process 0, compute it and send it back to process 0

    if (rank == 0)
    {
        // Read the input
        cin >> N >> M >> D >> K;
    }

    // Broadcast the input to all processes
    MPI_Bcast(&N, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&M, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&D, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&K, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // Initialize the grid and generate the part of the grid for the current process
    vector<vector<int>> grid(M, vector<int>(N, 0));
    generateMultibrotGridMPI(N, M, D, K, rank, size, grid);

    if (rank == 0)
    {
        // Process 0 receives the parts of the grid from all other processes and outputs the grid
        for (int i = 1; i < size; ++i)
        {
            int rowsPerProcess = M / size;
            int startRow = i * rowsPerProcess;
            int endRow = (i == size - 1) ? M : startRow + rowsPerProcess;
            for (int j = startRow; j < endRow; ++j)
            {
                MPI_Recv(&grid[j][0], N, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            }
        }

        // Output the grid
        for (int i = 0; i < M; ++i)
        {
            for (int j = 0; j < N; ++j)
            {
                cout << grid[i][j] << " ";
            }
            cout << endl;
        }
    }
    else
    {
        // Send the part of the grid generated by the current process to process 0
        int rowsPerProcess = M / size;
        int startRow = rank * rowsPerProcess;
        int endRow = (rank == size - 1) ? M : startRow + rowsPerProcess;
        for (int i = startRow; i < endRow; ++i)
        {
            MPI_Send(&grid[i][0], N, MPI_INT, 0, 0, MPI_COMM_WORLD);
        }
    }

    // Finalize MPI
    MPI_Finalize();

    return 0;
}
