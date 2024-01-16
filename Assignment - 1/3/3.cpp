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
    if (rank - 1 > 0 && rank + 1 < size)
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
    else if (rank + 1 < size)
    {
        // Send the last row to the next worker
        MPI_Send(&grid[re][0], M + 2, MPI_INT, rank + 1, 0, MPI_COMM_WORLD);

        // Receive the last row from the next worker
        MPI_Recv(&grid[re + 1][0], M + 2, MPI_INT, rank + 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }
    else if (rank - 1 > 0)
    {
        // Receive the first row from the previous worker
        MPI_Recv(&grid[rs - 1][0], M + 2, MPI_INT, rank - 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        // Send the first row to the previous worker
        MPI_Send(&grid[rs][0], M + 2, MPI_INT, rank - 1, 0, MPI_COMM_WORLD);
    }
    else
    {
        // Do nothing
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
        // cout << "------------------------" << endl;
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
                MPI_Barrier(MPI_COMM_WORLD);
                // Receive the result from the workers one by one
                for (int i = 1; i < size; ++i)
                {
                    int workers = size - 1;
                    int rowsPerWorker = N / workers;
                    int startRow = 1 + (i - 1) * rowsPerWorker;
                    int endRow = min(N, startRow + rowsPerWorker - 1);

                    // Receive the combined message from the ith worker
                    vector<int> message((endRow - startRow + 1) * (M + 2));
                    MPI_Recv(&message[0], message.size(), MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                    // Unpack the message into the grid using the startRow and endRow
                    int k = 0;
                    for (int j = startRow; j <= endRow; ++j)
                    {
                        for (int l = 0; l < M + 2; ++l)
                        {
                            grid[j][l] = message[k++];
                        }
                    }
                }
            }
            else
            {
                int workers = size - 1;
                int rowsPerWorker = N / workers;
                int startRow = 1 + (rank - 1) * rowsPerWorker;
                int endRow = min(N, startRow + rowsPerWorker - 1);

                // Go to the next generation
                gotoNextGeneration(grid, N, M, startRow, endRow);
                // Before handling boundary exchange, wait for all workers to finish their computation
                MPI_Barrier(MPI_COMM_WORLD);
                // Handle boundary exchange
                handleBoundaryExchange(grid, N, M, startRow, endRow, rank, size);

                // Send the result to the master (pack the result into a single message)
                vector<int> message;
                for (int i = startRow; i <= endRow; ++i)
                {
                    for (int j = 0; j < M + 2; ++j)
                    {
                        message.push_back(grid[i][j]);
                    }
                }
                // For each worker's iteration, send a single combined message to the master
                MPI_Send(&message[0], message.size(), MPI_INT, MASTER, 0, MPI_COMM_WORLD);
            }
        }

        // Print the result
        if (rank == MASTER)
        {
            // cout << "------------------------" << endl;
            for (int i = 1; i <= N; ++i)
            {
                for (int j = 1; j <= M; j++)
                {
                    cout << grid[i][j] << " ";
                }
                cout << endl;
            }
        }
    }

    // Finalize MPI
    MPI_Finalize();
    return 0;
}

// 5 6 10
// 1 0 0 1 1 0
// 0 1 1 0 0 1
// 1 1 1 0 0 0
// 0 0 1 1 1 0
// 0 0 0 1 1 0

// 0 1 1 0 0 0
// 0 1 1 0 0 0
// 0 0 0 0 0 0
// 0 0 0 0 0 0
// 0 0 0 0 0 0

// 10 10 7
// 0 0 1 0 0 0 0 0 0 0
// 1 0 1 0 0 0 0 0 0 0
// 0 1 1 0 0 0 0 0 0 0
// 0 0 0 0 0 0 0 0 0 0
// 0 0 0 0 0 0 0 0 0 0
// 0 0 0 0 0 0 0 0 0 0
// 0 0 0 0 0 0 0 0 0 0
// 0 0 0 0 0 0 0 0 0 0
// 0 0 0 0 0 0 0 0 0 0
// 0 0 0 0 0 0 0 0 0 0