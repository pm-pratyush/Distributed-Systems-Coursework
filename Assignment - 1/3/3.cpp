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
    int prev = rank - 1, next = rank + 1;
    if (rank % 2 == 0)
    {
        if (rank > 1)
            MPI_Recv(&grid[rs - 1][0], M + 2, MPI_INT, prev, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        if (rank < size - 1)
            MPI_Send(&grid[re][0], M + 2, MPI_INT, next, 0, MPI_COMM_WORLD);
        if (rank < size - 1)
            MPI_Recv(&grid[re + 1][0], M + 2, MPI_INT, next, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        if (rank > 1)
            MPI_Send(&grid[rs][0], M + 2, MPI_INT, prev, 0, MPI_COMM_WORLD);
    }
    else
    {
        if (rank < size - 1)
            MPI_Send(&grid[re][0], M + 2, MPI_INT, next, 0, MPI_COMM_WORLD);
        if (rank > 1)
            MPI_Recv(&grid[rs - 1][0], M + 2, MPI_INT, prev, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        if (rank > 1)
            MPI_Send(&grid[rs][0], M + 2, MPI_INT, prev, 0, MPI_COMM_WORLD);
        if (rank < size - 1)
            MPI_Recv(&grid[re + 1][0], M + 2, MPI_INT, next, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
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
        // Divide the rows among the workers
        int workers = size - 1;
        int rows_per_worker = N / workers, extra_rows = N % workers;

        vector<pair<int, int>> row_segment(size);
        row_segment[0] = {0, 0};
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

        // Run the simulation for T iterations
        for (int t = 0; t < T; ++t)
        {
            if (rank == MASTER)
            {
                // Receive the result from the workers one by one
                for (int i = 1; i < size; ++i)
                {
                    int s_idx = row_segment[i].first, e_idx = row_segment[i].second;

                    // Receive the combined message from the ith worker
                    vector<int> message((e_idx - s_idx + 1) * (M + 2));
                    MPI_Recv(&message[0], message.size(), MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                    // Unpack the message into the grid using the startRow and endRow
                    int k = 0;
                    for (int j = s_idx; j <= e_idx; ++j)
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
                int s_idx = row_segment[rank].first, e_idx = row_segment[rank].second;

                // Go to the next generation
                gotoNextGeneration(grid, N, M, s_idx, e_idx);
                // Handle boundary exchange
                handleBoundaryExchange(grid, N, M, s_idx, e_idx, rank, size);

                // Send the result to the master (pack the result into a single message)
                vector<int> message;
                for (int i = s_idx; i <= e_idx; ++i)
                {
                    for (int j = 0; j < M + 2; ++j)
                    {
                        message.push_back(grid[i][j]);
                    }
                }
                // For each worker's iteration, send a single combined message to the master
                MPI_Send(&message[0], message.size(), MPI_INT, MASTER, 0, MPI_COMM_WORLD);
            }

            // Synchronize all the workers
            MPI_Barrier(MPI_COMM_WORLD);
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