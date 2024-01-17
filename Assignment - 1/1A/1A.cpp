#include <bits/stdc++.h>
#include <mpi.h>

using namespace std;

#define MASTER 0
#define WORKER 1

// Global variables for the grid boundaries
double minx = -1.5, maxx = 1.0, miny = -1.0, maxy = 1.0;

// Function to check if a point is in the Multibrot set
int isMultibrot(complex<double> &c, int maxIterations, int exponent)
{
    // Initialize z to 0
    complex<double> z(0, 0);
    for (int i = 0; i < maxIterations; ++i)
    {
        // Calculate z = z^exponent + c
        z = pow(z, exponent) + c;
        // Check if the magnitude of z is greater than 2
        if (abs(z) > 2.0)
        {
            return 0;
        }
    }
    return 1;
}

// Function to generate the grid using MPI parallel programming
void generateMultibrotGridMPI(int N, int M, int D, int K, int rank, int size, vector<vector<int>> &grid, int s_idx, int e_idx)
{
    // Calculate the step size
    double stepx = (maxx - minx) / (double)(N - 1);
    double stepy = (maxy - miny) / (double)(M - 1);

    int index = 0;
    for (int i = s_idx; i < e_idx; ++i)
    {
        int row = i / N, col = i % N;
        double realPart = minx + (col * stepx);
        double imaginaryPart = maxy - (row * stepy);

        // Check if the point is in the Multibrot set after K iterations
        complex<double> c(realPart, imaginaryPart);
        grid[row][col] = isMultibrot(c, K, D);
    }
}

// Function to print the grid
void printGrid(vector<vector<int>> &grid, int M, int N)
{
    for (int i = 0; i < M; ++i)
    {
        for (int j = 0; j < N - 1; ++j)
        {
            cout << grid[i][j] << " ";
        }
        cout << grid[i][N - 1] << endl;
    }
}

int main(int argc, char *argv[])
{
    // Initialize MPI
    MPI_Init(&argc, &argv);
    double start_time, end_time;

    int rank, size;
    // Get the rank and size of the MPI world
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int N, M, D, K;
    // You are expected to output a Grid of M x N
    // N: Number of columns
    // M: Number of rows
    // D: Exponent
    // K: Maximum number of iterations

    // Process 0 reads the input, computes its part and produces the output
    // Other processes receive their part of the grid from process 0, compute it and send it back to process 0

    if (rank == MASTER)
    {
        // Read the input
        cin >> N >> M >> D >> K;
    }

    // Broadcast the input to all processes
    MPI_Bcast(&N, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&M, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&D, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&K, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // Divide the points among the processes
    int totalPoints = N * M;
    int pointsPerProcess = totalPoints / size, remainingPoints = totalPoints % size;

    vector<pair<int, int>> point_segment(size);
    point_segment[0] = {0, pointsPerProcess};
    for (int i = 1; i < size; ++i)
    {
        int s_idx = point_segment[i - 1].second;
        int e_idx = s_idx + pointsPerProcess;
        if (remainingPoints > 0)
        {
            ++e_idx;
            --remainingPoints;
        }
        point_segment[i] = {s_idx, e_idx};
    }

    // Sync all processes and start the timer
    MPI_Barrier(MPI_COMM_WORLD);
    start_time = MPI_Wtime();

    // Initialize the grid and generate the part of the grid for the current process
    vector<vector<int>> grid(M, vector<int>(N, 0));
    generateMultibrotGridMPI(N, M, D, K, rank, size, grid, point_segment[rank].first, point_segment[rank].second);

    if (rank == MASTER)
    {
        // Process 0 receives the parts of the grid from all other processes and outputs the grid
        for (int i = 1; i < size; ++i)
        {
            // Receive the part of the grid generated by process i
            int s_idx = point_segment[i].first, e_idx = point_segment[i].second;

            vector<int> buffer((e_idx - s_idx));
            MPI_Recv(&buffer[0], (e_idx - s_idx), MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            int index = 0;
            for (int j = s_idx; j < e_idx; ++j)
            {
                int row = j / N, col = j % N;
                grid[row][col] = buffer[index++];
            }
        }
    }
    else
    {
        // Send the part of the grid generated by the current process to process 0
        int s_idx = point_segment[rank].first, e_idx = point_segment[rank].second;

        int index = 0;
        vector<int> buffer((e_idx - s_idx));
        for (int i = s_idx; i < e_idx; ++i)
        {
            int row = i / N, col = i % N;
            buffer[index++] = grid[row][col];
        }
        MPI_Send(&buffer[0], (e_idx - s_idx), MPI_INT, 0, 0, MPI_COMM_WORLD);
    }

    // Sync all processes and stop the timer
    MPI_Barrier(MPI_COMM_WORLD);
    end_time = MPI_Wtime();

    // Output the time taken
    if (rank == MASTER)
    {
        // Print the grid
        printGrid(grid, M, N);
        // Print the time taken
        // cout << "Time taken: " << end_time - start_time << " seconds" << endl;
    }

    // Finalize MPI
    MPI_Finalize();
    return 0;
}