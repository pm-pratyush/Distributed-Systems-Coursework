#include <bits/stdc++.h>
#include <mpi.h>

using namespace std;

#define MASTER 0
#define WORKER 1

// Function to go to the next generation
void gotoNextGeneration(vector<vector<int>> &grid, int N, int M, int rs, int re)
{
    vector<vector<int>> nextGrid(N + 2, vector<int>(M + 2, 0));
    for (int i = rs; i <= re; ++i)
    {
        for (int j = 1; j <= M; j++)
        {
            int alive = 0;
            for (int di = -1; di <= 1; ++di)
            {
                for (int dj = -1; dj <= 1; ++dj)
                {
                    if (di == 0 && dj == 0)
                        continue;
                    alive += grid[i + di][j + dj];
                }
            }
            if (grid[i][j] == 1)
            {
                if (alive < 2 || alive > 3)
                    nextGrid[i][j] = 0;
                else
                    nextGrid[i][j] = 1;
            }
            else
            {
                if (alive == 3)
                    nextGrid[i][j] = 1;
                else
                    nextGrid[i][j] = 0;
            }
        }
    }
    grid = nextGrid;
}

// Function to handle boundary exchange
void handleBoundaryExchange(vector<vector<int>> &grid, int N, int M, int rs, int re, int rank, int size)
{
    if (rank == 1 && rank + 1 < size)
    {
        // Send the last row to the next worker
        MPI_Send(&grid[re][0], M + 2, MPI_INT, rank + 1, 0, MPI_COMM_WORLD);

        // Receive the last row from the next worker
        MPI_Recv(&grid[re + 1][0], M + 2, MPI_INT, rank + 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }
    else if (rank == size - 1 && rank - 1 > 0)
    {
        // Receive the first row from the previous worker
        MPI_Recv(&grid[rs - 1][0], M + 2, MPI_INT, rank - 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        // Send the first row to the previous worker
        MPI_Send(&grid[rs][0], M + 2, MPI_INT, rank - 1, 0, MPI_COMM_WORLD);
    }
    else if (rank - 1 > 0 && rank + 1 < size)
    {
        // Receive the first row from the previous worker
        MPI_Recv(&grid[rs - 1][0], M + 2, MPI_INT, rank - 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        // Send the first row to the previous worker
        MPI_Send(&grid[rs][0], M + 2, MPI_INT, rank - 1, 0, MPI_COMM_WORLD);

        // Send the last row to the next worker
        MPI_Send(&grid[re][0], M + 2, MPI_INT, rank + 1, 0, MPI_COMM_WORLD);

        // Receive the last row from the next worker
        MPI_Recv(&grid[re + 1][0], M + 2, MPI_INT, rank + 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }
}

int main(int argc, char *argv[])
{
    // Initialize MPI
    MPI_Init(&argc, &argv);

    int rank, size;
    // Get the rank and size of the MPI world
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int N, M, T;
    if (rank == MASTER)
    {
        cin >> N >> M >> T;
    }
    MPI_Bcast(&N, 1, MPI_INT, MASTER, MPI_COMM_WORLD);
    MPI_Bcast(&M, 1, MPI_INT, MASTER, MPI_COMM_WORLD);
    MPI_Bcast(&T, 1, MPI_INT, MASTER, MPI_COMM_WORLD);

    vector<vector<int>> grid(N + 2, vector<int>(M + 2, 0));
    if (rank == MASTER)
    {
        for (int i = 1; i <= N; ++i)
        {
            for (int j = 1; j <= M; j++)
            {
                cin >> grid[i][j];
            }
        }
    }
    for (int i = 0; i < N + 2; ++i)
    {
        MPI_Bcast(&grid[i][0], M + 2, MPI_INT, MASTER, MPI_COMM_WORLD);
    }

    if (size == 1)
    {
        for (int t = 0; t < T; ++t)
        {
            gotoNextGeneration(grid, N, M, 1, N);
        }

        // Print the result
        for (int i = 1; i <= N; ++i)
        {
            for (int j = 1; j <= M; j++)
            {
                cout << grid[i][j] << " ";
            }
            cout << endl;
        }
    }
    else
    {
        for (int t = 0; t < T; ++t)
        {
            if (rank == MASTER)
            {
                // Synchrnize all the workers
                MPI_Barrier(MPI_COMM_WORLD);
                // Receive the result from the workers
                for (int i = 1; i <= N; ++i)
                {
                    MPI_Recv(&grid[i][0], M + 2, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                }

                // Print the result
                for (int i = 1; i <= N; ++i)
                {
                    for (int j = 1; j <= M; j++)
                    {
                        cout << grid[i][j] << " ";
                    }
                    cout << endl;
                }
            }
            else
            {
                int workers = size - 1;
                int rowsPerWorker = (N + 2) / workers;
                int startRow = rowsPerWorker * (rank - 1) + 1;
                int endRow = (rank == size - 1) ? N : startRow + rowsPerWorker;

                gotoNextGeneration(grid, N, M, startRow, endRow);
                handleBoundaryExchange(grid, N, M, startRow, endRow, rank, size);

                // Synchrnize all the workers
                MPI_Barrier(MPI_COMM_WORLD);
                // Send the result to the master
                for (int i = startRow; i <= endRow; ++i)
                {
                    MPI_Send(&grid[i][0], M + 2, MPI_INT, MASTER, 0, MPI_COMM_WORLD);
                }
            }
        }
    }
    
    // Finalize MPI
    MPI_Finalize();
    return 0;
}