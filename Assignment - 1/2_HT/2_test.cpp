#include <bits/stdc++.h>
#include <mpi.h>

using namespace std;

#define MASTER 0
#define WORKER 1
#define INF INT_MAX

// Find the owner of the kth row: TC = O(size) SC = O(1)
pair<int, int> findOwner(vector<pair<int, int>> &row_segment, int k)
{
    for (int i = 0; i < row_segment.size(); ++i)
    {
        if (k >= row_segment[i].first && k <= row_segment[i].second)
        {
            return {i, k - row_segment[i].first};
        }
    }
    return {-1, -1};
}

// Implement Floyd Warshall Algorithm: TC = O(N*N*N/size) SC = O(N) for each process
void floydWarshall(vector<vector<int>> &graph, int N, int rank, int size, vector<pair<int, int>> &row_segment)
{
    int s_idx = row_segment[rank].first, e_idx = row_segment[rank].second;

    for (int k = 0; k < N; ++k)
    {
        vector<int> kth_row(N, INF);

        // Find the owner of the kth row
        pair<int, int> owner = findOwner(row_segment, k);
        if (owner.first == rank)
        {
            for (int i = 0; i < N; ++i)
            {
                kth_row[i] = graph[owner.second][i];
            }
        }
        MPI_Bcast(&kth_row[0], N, MPI_INT, owner.first, MPI_COMM_WORLD);

        for (int i = s_idx; i <= e_idx; ++i)
        {
            for (int j = 0; j < N; ++j)
            {
                if (graph[i - s_idx][k] != INF && kth_row[j] != INF)
                    graph[i - s_idx][j] = min(graph[i - s_idx][j], graph[i - s_idx][k] + kth_row[j]);
            }
        }
    }
}

// Print the distance matrix
void printDist(vector<vector<int>> &dist, int N)
{
    ofstream fout("output.txt");
    for (int i = 0; i < N; ++i)
    {
        for (int j = 0; j < N; j++)
        {
            if (dist[i][j] == INF)
                fout << -1 << " ";
            else
                fout << dist[i][j] << " ";
        }
        fout << endl;
    }
}

// TC: O(N * N * N / size) SC: O(N * N / size) for each process, O(N * N) for process 0
int main(int argc, char *argv[])
{
    // Initialize MPI
    MPI_Init(&argc, &argv);
    double start_time, end_time;

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

    // Divide the rows among the workers
    int workers = size;
    int rows_per_worker = N / workers, remaining_rows = N % workers;

    vector<pair<int, int>> row_segment(size);
    if (rank == MASTER)
    {
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
    }
    MPI_Bcast(&row_segment[0], size * 2, MPI_INT, MASTER, MPI_COMM_WORLD);

    if (rank == MASTER)
    {
        // Read the graph
        vector<vector<int>> graph(N, vector<int>(N, 0));
        for (int i = 0; i < N; ++i)
        {
            for (int j = 0; j < N; j++)
            {
                cin >> graph[i][j];
                if (i == j)
                    graph[i][j] = 0;
                if (graph[i][j] == -1)
                    graph[i][j] = INF;
            }
        }

        // Start the timer
        start_time = MPI_Wtime();

        // Send the rows to the workers
        for (int i = 1; i < size; ++i)
        {
            int s_idx = row_segment[i].first, e_idx = row_segment[i].second;

            vector<int> sendBuffer;
            for (int j = s_idx; j <= e_idx; ++j)
            {
                for (int k = 0; k < N; ++k)
                    sendBuffer.push_back(graph[j][k]);
            }
            MPI_Send(&sendBuffer[0], sendBuffer.size(), MPI_INT, i, 0, MPI_COMM_WORLD);
        }

        // Apply Floyd Warshall Algorithm
        floydWarshall(graph, N, rank, size, row_segment);

        // Initialize the distance matrix
        vector<vector<int>> dist(N, vector<int>(N, 0));
        // Fill the distance matrix with the parts generated by process 0
        int s_idx = row_segment[rank].first, e_idx = row_segment[rank].second;
        for (int i = s_idx; i <= e_idx; ++i)
        {
            for (int j = 0; j < N; ++j)
            {
                dist[i][j] = graph[i][j];
            }
        }

        // Receive the updated rows from the workers
        for (int i = 1; i < size; ++i)
        {
            int s_idx = row_segment[i].first, e_idx = row_segment[i].second;

            vector<int> recvBuffer((e_idx - s_idx + 1) * N);
            MPI_Recv(&recvBuffer[0], recvBuffer.size(), MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            int index = 0;
            for (int j = s_idx; j <= e_idx; ++j)
            {
                for (int k = 0; k < N; ++k)
                {
                    dist[j][k] = recvBuffer[index++];
                }
            }
        }

        // Stop the timer
        end_time = MPI_Wtime();

        // Print the final distance matrix
        printDist(dist, N);
        // Print the time taken
        cout << "Time taken: " << end_time - start_time << " seconds" << endl;
    }
    else
    {
        int s_idx = row_segment[rank].first, e_idx = row_segment[rank].second;

        // Receive the rows from the master
        vector<int> recvBuffer((e_idx - s_idx + 1) * N);
        MPI_Recv(&recvBuffer[0], recvBuffer.size(), MPI_INT, MASTER, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        int index = 0;
        vector<vector<int>> graph(e_idx - s_idx + 1, vector<int>(N));
        for (int i = s_idx; i <= e_idx; ++i)
        {
            for (int j = 0; j < N; ++j)
            {
                graph[i - s_idx][j] = recvBuffer[index++];
            }
        }

        // Apply Floyd Warshall Algorithm
        floydWarshall(graph, N, rank, size, row_segment);

        // Send the updated rows to the master
        vector<int> sendBuffer;
        for (int i = s_idx; i <= e_idx; ++i)
        {
            for (int j = 0; j < N; ++j)
                sendBuffer.push_back(graph[i - s_idx][j]);
        }
        MPI_Send(&sendBuffer[0], sendBuffer.size(), MPI_INT, MASTER, 0, MPI_COMM_WORLD);
    }

    // Finalize MPI
    MPI_Finalize();
    return 0;
}