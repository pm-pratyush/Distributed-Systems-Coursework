#include <bits/stdc++.h>
#include <mpi.h>

using namespace std;

#define MASTER 0
#define WORKER 1

#define ALIVE 1
#define DEAD 0

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

    if (size == 1)
    {
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
        for (int i = 0; i < N; ++i)
        {
            cout << graph[i][0];
            for (int j = 1; j < N; ++j)
            {
                cout << " " << graph[i][j];
            }
            cout << endl;
        }
    }
    else
    {
        if (rank == MASTER)
        {
            int deadProcesses = 0;
            while (deadProcesses < size - 1)
            {
                int message[3];
                MPI_Status status;
                MPI_Recv(&message, 3, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

                int src = message[0], dest = message[1], weight = message[2];
                if (status.MPI_TAG == DEAD)
                {
                    deadProcesses++;
                }
                else if (graph[src][dest] > weight)
                {
                    graph[src][dest] = weight;
                }
            }

            for (int i = 0; i < N; ++i)
            {
                cout << graph[i][0];
                for (int j = 1; j < N; ++j)
                {
                    cout << " " << graph[i][j];
                }
                cout << endl;
            }
        }
        else
        {
            int rowsPerProcess = N / (size - 1);
            int startRow = (rank - 1) * rowsPerProcess;
            int endRow = (rank == size - 1) ? N : startRow + rowsPerProcess;

            int message[3];
            for (int k = 0; k < N; ++k)
            {
                for (int i = 0; i < N; ++i)
                {
                    for (int j = startRow; j < endRow; ++j)
                    {
                        if (graph[i][k] != INF && graph[k][j] != INF && graph[i][k] + graph[k][j] < graph[i][j])
                        {
                            graph[i][j] = graph[i][k] + graph[k][j];

                            message[0] = i;
                            message[1] = j;
                            message[2] = graph[i][j];
                            MPI_Send(&message, 3, MPI_INT, MASTER, ALIVE, MPI_COMM_WORLD);
                        }
                    }
                }
            }
            MPI_Send(&message, 3, MPI_INT, MASTER, DEAD, MPI_COMM_WORLD);
        }
    }
    
    // Finalize MPI
    MPI_Finalize();
    return 0;
}

// 5 
// 0 4 -1 5 -1
// -1 0 1 -1 6
// 2 -1 0 3 -1
// -1 -1 1 0 2
// 1 -1 -1 4 0