#include <bits/stdc++.h>
#include <mpi.h>

using namespace std;

#define MASTER 0
#define WORKER 1
#define INF INT_MAX

// Find the owner of the kth row
int findOwner(vector<pair<int, int>> &row_segment, int k)
{
    for (int i = 0; i < row_segment.size(); ++i)
    {
        if (k >= row_segment[i].first && k <= row_segment[i].second)
            return i;
    }
    return -1;
}

// Implement Floyd Warshall Algorithm
void floydWarshall(vector<vector<int>> &graph, int N, int rank, int size, vector<pair<int, int>> &row_segment)
{
    for (int k = 0; k < N; ++k)
    {
        // Find the owner of the kth row
        int owner = findOwner(row_segment, k);

        // Broadcast the kth row to all processes
        vector<int> kth_row(N, INF);
        if (rank == owner)
        {
            kth_row = graph[k];
        }
        MPI_Bcast(&kth_row[0], N, MPI_INT, owner, MPI_COMM_WORLD);

        // Update the graph
        for (int i = row_segment[rank].first; i <= row_segment[rank].second; ++i)
        {
            for (int j = 0; j < N; ++j)
            {
                if (graph[i][k] != INF && kth_row[j] != INF)
                {
                    graph[i][j] = min(graph[i][j], graph[i][k] + kth_row[j]);
                }
            }
        }
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

    int N;
    if (rank == MASTER)
    {
        cin >> N;
    }
    MPI_Bcast(&N, 1, MPI_INT, MASTER, MPI_COMM_WORLD);

    vector<vector<int>> graph(N, vector<int>(N, 0));
    if (rank == MASTER)
    {
        for (int i = 0; i < N; ++i)
        {
            for (int j = 0; j < N; j++)
            {
                cin >> graph[i][j];
                if (graph[i][j] == -1)
                    graph[i][j] = INF;
            }
        }
    }
    for (int i = 0; i < N; ++i)
    {
        MPI_Bcast(&graph[i][0], N, MPI_INT, MASTER, MPI_COMM_WORLD);
    }

    // Divide the rows among the workers
    int workers = size;
    int rows_per_worker = N / workers, remaining_rows = N % workers;

    vector<pair<int, int>> row_segment(size);
    row_segment[0] = {0, rows_per_worker - 1};
    for (int i = 1; i < size; ++i)
    {
        int s_idx = row_segment[i - 1].second + 1;
        int e_idx = s_idx + rows_per_worker - 1;
        if (remaining_rows > 0)
        {
            ++e_idx;
            --remaining_rows;
        }
        row_segment[i] = {s_idx, e_idx};
    }

    // Implement Floyd Warshall Algorithm
    floydWarshall(graph, N, rank, size, row_segment);

    // Exchange the updated rows
    if (rank != MASTER)
    {
        // Send the updated rows to the master
        vector<int> buffer;
        for (int i = row_segment[rank].first; i <= row_segment[rank].second; ++i)
        {
            for (int j = 0; j < N; ++j)
                buffer.push_back(graph[i][j]);
        }
        MPI_Send(&buffer[0], buffer.size(), MPI_INT, MASTER, 0, MPI_COMM_WORLD);
    }
    else
    {
        // Receive the updated rows from the workers
        for (int i = 1; i < size; ++i)
        {
            int s_idx = row_segment[i].first, e_idx = row_segment[i].second;
            vector<int> buffer((e_idx - s_idx + 1) * N);
            MPI_Recv(&buffer[0], buffer.size(), MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            int index = 0;
            for (int j = s_idx; j <= e_idx; ++j)
            {
                for (int k = 0; k < N; ++k)
                {
                    graph[j][k] = buffer[index++];
                }
            }
        }
    }

    // Output the graph
    if (rank == MASTER)
    {
        for (int i = 0; i < N; ++i)
        {
            for (int j = 0; j < N; j++)
            {
                if (graph[i][j] == INF)
                    cout << -1 << " ";
                else
                    cout << graph[i][j] << " ";
            }
            cout << endl;
        }
    }

    // Finalize MPI
    MPI_Finalize();
    return 0;
}