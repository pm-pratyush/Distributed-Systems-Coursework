#include <bits/stdc++.h>
#include <mpi.h>

using namespace std;

#define MASTER 0
#define WORKER 1

#define INV -1
#define INF INT_MAX

int main(int argc, char *argv[])
{
    // Initialize MPI
    MPI_Init(&argc, &argv);

    int rank, size;
    // Get the rank and size of the MPI world
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int N;
    // N: number of vertices in the graph
    vector<vector<int>> graph(N, vector<int>(N, 0));
    // graph[i][j] = weight of the edge from vertex i to vertex j
    // graph[i][j] = -1 if there is no edge from vertex i to vertex j

    if (rank == MASTER)
    {
        // Read the input
        cin >> N;
        for (int i = 0; i < N; ++i)
        {
            for (int j = 0; j < N; j++)
            {
                cin >> graph[i][j];
                if (graph[i][j] == -1)
                    graph[i][j] = INF;
            }
        }

        if (size == 1)
        {
            // If there is only one process, then the master process will compute the shortest paths
            for (int k = 0; k < N; ++k)
            {
                for (int i = 0; i < N; ++i)
                {
                    for (int j = 0; j < N; ++j)
                    {
                        if (graph[i][k] != INF && graph[k][j] != INF && graph[i][k] + graph[k][j] < graph[i][j])
                        {
                            graph[i][j] = graph[i][k] + graph[k][j];
                        }
                    }
                }
            }

            // Print the graph
            for (int i = 0; i < N; ++i)
            {
                for (int j = 0; j < N; ++j)
                {
                    cout << graph[i][j] << " ";
                }
                cout << endl;
            }
        }
        else
        {
            // Send the data to all other processes
            for (int i = 1; i < size; ++i)
            {
                // Send the number of vertices to process i
                MPI_Send(&N, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
            }
            // Send the graph to all other processes
            for (int i = 1; i < size; ++i)
            {
                for (int j = 0; j < N; ++j)
                {
                    MPI_Send(&graph[j][0], N, MPI_INT, i, 0, MPI_COMM_WORLD);
                }
            }

            // Receive the data_Recv from all other processes: {src, dest, updatedWeight}
            int deadCount = 0;
            while (deadCount < size - 1)
            {
                int data_Recv[3];
                MPI_Recv(&data_Recv, 3, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                int src = data_Recv[0], dest = data_Recv[1], updatedWeight = data_Recv[2];
                if (src == INV && dest == INV && updatedWeight == INV)
                {
                    ++deadCount;
                }
                else if (graph[src][dest] > updatedWeight)
                {
                    graph[src][dest] = updatedWeight;
                }
            }

            // Print the graph
            for (int i = 0; i < N; ++i)
            {
                for (int j = 0; j < N; ++j)
                {
                    cout << graph[i][j] << " ";
                }
                cout << endl;
            }
        }
    }
    else
    {
        // Receive the number of vertices from the master process
        MPI_Recv(&N, 1, MPI_INT, MASTER, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        // Receive the graph from the master process
        for (int i = 0; i < N; ++i)
        {
            MPI_Recv(&graph[i][0], N, MPI_INT, MASTER, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }

        int rowsPerProcess = N / (size - 1);
        int startRow = (rank - 1) * rowsPerProcess;
        int endRow = (rank == size - 1) ? N : startRow + rowsPerProcess;

        // Compute the shortest paths
        int data_Send[3];
        for (int k = 0; k < N; ++k)
        {
            for (int i = 0; i < N; ++i)
            {
                for (int j = startRow; j < endRow; ++j)
                {
                    if (graph[i][k] != INF && graph[k][j] != INF && graph[i][k] + graph[k][j] < graph[i][j])
                    {
                        graph[i][j] = graph[i][k] + graph[k][j];

                        // Send the updated weight to the master process
                        data_Send[0] = i;
                        data_Send[1] = j;
                        data_Send[2] = graph[i][j];
                        MPI_Send(&data_Send, 3, MPI_INT, MASTER, 0, MPI_COMM_WORLD);
                    }
                }
            }
        }

        // Send a message to the master process that this process has finished computing
        data_Send[0] = INV;
        data_Send[1] = INV;
        data_Send[2] = INV;
        MPI_Send(&data_Send, 3, MPI_INT, MASTER, 0, MPI_COMM_WORLD);
    }

    // Finalize MPI
    MPI_Finalize();
    return 0;
}
