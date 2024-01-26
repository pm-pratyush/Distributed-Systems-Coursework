#include <bits/stdc++.h>
#include <mpi.h>

using namespace std;

#define MASTER 0
#define WORKER 1

// Function to go to the next generation: TC = O(N*M/size) SC = O(N*M/size)
void gotoNextGeneration(vector<vector<int>> &grid, int NR, int NC, int rs, int re, int rank, int size)
{
    // We only need to consider the rows from 1 to NR - 2 and columns from 1 to NC - 2
    vector<vector<int>> nextGrid(NR, vector<int>(NC, 0));
    for (int i = 1; i <= NR - 2; ++i)
    {
        for (int j = 1; j <= NC - 2; ++j)
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
void handleBoundaryExchange(vector<vector<int>> &grid, int NR, int NC, int rs, int re, int rank, int size)
{
    int prev = rank - 1, next = rank + 1;
    if (rank % 2 == 0)
    {
        if (prev >= 0)
            MPI_Send(&grid[1][0], NC, MPI_INT, prev, 0, MPI_COMM_WORLD);
        if (next < size)
            MPI_Send(&grid[NR - 2][0], NC, MPI_INT, next, 0, MPI_COMM_WORLD);
        if (prev >= 0)
            MPI_Recv(&grid[0][0], NC, MPI_INT, prev, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        if (next < size)
            MPI_Recv(&grid[NR - 1][0], NC, MPI_INT, next, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }
    else
    {
        if (next < size)
            MPI_Recv(&grid[NR - 1][0], NC, MPI_INT, next, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        if (prev >= 0)
            MPI_Recv(&grid[0][0], NC, MPI_INT, prev, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        if (next < size)
            MPI_Send(&grid[NR - 2][0], NC, MPI_INT, next, 0, MPI_COMM_WORLD);
        if (prev >= 0)
            MPI_Send(&grid[1][0], NC, MPI_INT, prev, 0, MPI_COMM_WORLD);
    }
}

// Print the grid
void printGrid(vector<vector<int>> &grid, int N, int M)
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

// TC = O(N*M/size *T) SC = O(N*M/size) for each process, SC = O(N*M) for the master process
int main(int argc, char *argv[])
{
    // Initialize MPI
    MPI_Init(&argc, &argv);
    double start_time, end_time;

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

    // Divide the rows among the workers
    int workers = size;
    int rows_per_worker = N / workers, extra_rows = N % workers;

    vector<pair<int, int>> row_segment(size);
    if (rank == MASTER)
    {
        row_segment[0] = {1, rows_per_worker};
        for (int i = 1; i < size; ++i)
        {
            int s_idx = row_segment[i - 1].second + 1;
            int e_idx = s_idx + rows_per_worker - 1;
            if (extra_rows > 0)
            {
                ++e_idx;
                --extra_rows;
            }
            row_segment[i] = {s_idx, e_idx};
        }
    }
    MPI_Bcast(&row_segment[0], size * 2, MPI_INT, MASTER, MPI_COMM_WORLD);

    if (rank == MASTER)
    {
        vector<vector<int>> grid(N + 2, vector<int>(M + 2, 0));
        for (int i = 1; i <= N; ++i)
        {
            for (int j = 1; j <= M; j++)
            {
                cin >> grid[i][j];
            }
        }

        // Start the timer
        start_time = MPI_Wtime();

        // Send the rows to the workers
        for (int i = 1; i < size; ++i)
        {
            int s_idx = row_segment[i].first, e_idx = row_segment[i].second;

            vector<int> sendBuffer;
            for (int j = s_idx - 1; j <= e_idx + 1; ++j)
            {
                for (int k = 0; k <= M + 1; ++k)
                    sendBuffer.push_back(grid[j][k]);
            }
            MPI_Send(&sendBuffer[0], sendBuffer.size(), MPI_INT, i, 0, MPI_COMM_WORLD);
        }

        int s_idx = row_segment[rank].first, e_idx = row_segment[rank].second;
        vector<vector<int>> tempGrid(e_idx - s_idx + 3, vector<int>(M + 2, 0));
        for (int i = s_idx - 1; i <= e_idx + 1; ++i)
        {
            for (int j = 0; j <= M + 1; ++j)
            {
                tempGrid[i - s_idx + 1][j] = grid[i][j];
            }
        }

        // Run the simulation for T iterations
        for (int t = 0; t < T; ++t)
        {
            // Go to the next generation
            gotoNextGeneration(tempGrid, e_idx - s_idx + 3, M + 2, s_idx, e_idx, rank, size);
            // Handle boundary exchange
            handleBoundaryExchange(tempGrid, e_idx - s_idx + 3, M + 2, s_idx, e_idx, rank, size);
            // Sync all the processes
            MPI_Barrier(MPI_COMM_WORLD);
        }

        // Copy the result back to the grid
        for (int i = s_idx; i <= e_idx; ++i)
        {
            for (int j = 0; j <= M + 1; ++j)
            {
                grid[i][j] = tempGrid[i - s_idx + 1][j];
            }
        }

        // Receive the result from the workers one by one
        for (int i = 1; i < size; ++i)
        {
            int s_idx = row_segment[i].first, e_idx = row_segment[i].second;

            // Receive the combined message from the ith worker
            vector<int> recvBuffer((e_idx - s_idx + 1) * (M + 2));
            MPI_Recv(&recvBuffer[0], recvBuffer.size(), MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            // Unpack the message into the grid using the startRow and endRow
            int index = 0;
            for (int j = s_idx; j <= e_idx; ++j)
            {
                for (int k = 0; k < M + 2; ++k)
                {
                    grid[j][k] = recvBuffer[index++];
                }
            }
        }

        // Stop the timer
        end_time = MPI_Wtime();

        // Print the final grid
        printGrid(grid, N, M);
        // Print the time taken
        // cout << "Time taken: " << end_time - start_time << " seconds" << endl;
    }
    else
    {

        int s_idx = row_segment[rank].first, e_idx = row_segment[rank].second;
        
        // Receive the rows from the master
        vector<int> recvBuffer((e_idx - s_idx + 3) * (M + 2));
        MPI_Recv(&recvBuffer[0], recvBuffer.size(), MPI_INT, MASTER, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        // Unpack the message into the grid using the startRow and endRow
        vector<vector<int>> tempGrid(e_idx - s_idx + 3, vector<int>(M + 2, 0));
        int index = 0;
        for (int i = 0; i <= e_idx - s_idx + 2; ++i)
        {
            for (int j = 0; j <= M + 1; ++j)
            {
                tempGrid[i][j] = recvBuffer[index++];
            }
        }

        // Run the simulation for T iterations
        for (int t = 0; t < T; ++t)
        {
            // Go to the next generation
            gotoNextGeneration(tempGrid, e_idx - s_idx + 3, M + 2, s_idx, e_idx, rank, size);
            // Handle boundary exchange
            handleBoundaryExchange(tempGrid, e_idx - s_idx + 3, M + 2, s_idx, e_idx, rank, size);
            // Sync all the processes
            MPI_Barrier(MPI_COMM_WORLD);
        }

        // Send the result to the master
        vector<int> sendBuffer;
        for (int i = 1; i <= e_idx - s_idx + 1; ++i)
        {
            for (int j = 0; j <= M + 1; ++j)
            {
                sendBuffer.push_back(tempGrid[i][j]);
            }
        }
        MPI_Send(&sendBuffer[0], sendBuffer.size(), MPI_INT, MASTER, 0, MPI_COMM_WORLD);
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

// 10 10 50
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