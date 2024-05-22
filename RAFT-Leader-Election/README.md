
# Raft Leader Election Simulation

This project implements a simulation of the RAFT consensus algorithm focusing on the leader election process using Python and ZeroMQ for message passing. The Raft algorithm ensures fault-tolerant and consistent replication of state across a cluster of nodes.

## Team Members:

Karthik Prasanna N | Pratyush Mohanty

## Overview

The Raft algorithm is a consensus protocol designed to manage a replicated log and ensure that a single node (leader) is responsible for managing log entries and maintaining consistency across all nodes in the cluster. Key features of this implementation include:

* **Node Initialization** : Each node is initialized as a Follower node with unique identifiers, network addresses, and other necessary parameters.
* **Message Passing** : Nodes communicate using ZeroMQ sockets (`REQ/REP`) for sending and receiving messages such as `RequestVote` and `AppendEntry`.
* **Election Process** : Nodes transition between states (`Follower`, `Candidate`, `Leader`) based on timeouts and received messages to facilitate leader election.
* **Log Replication** : The leader manages log replication by sending `AppendEntry` messages to followers, ensuring consistency across the cluster.
* **State Persistence** : Node state including current term, log entries, and voting status is periodically saved to disk for fault tolerance.

## Project Structure

The project is structured as follows:

* **`node.py`** : Contains the main implementation of the `RaftNode` class responsible for node functionality.
* **`utils.py`** : Utility functions for parsing configuration and handling state persistence.
* **`handle_state.py`** : Functions for saving and restoring node state to/from disk.
* **`messages.py`** : Defines message classes (`RequestVoteArgs`, `RequestVoteReply`, `AppendEntryArgs`, `AppendEntryReply`) used for inter-node communication.

## Dependencies

* Python 3.x
* ZeroMQ (`pyzmq`)
* Flask (`flask`)
* Requests (`requests`)

## Usage

### Step 1: Clone the Repository

```
git clone <repository>
cd <repository>
```

### Step 2: Run Nodes

Open four different terminal windows or tabs and run the following commands to start individual nodes:

Step 1:

```
python3 server.py --node_id 0
```

Step 2:

```
python3 server.py --node_id 1
```

Step 3:

```
python3 server.py --node_id 2
```

Step 4: Interact with Client

Once all nodes are running, interact with the client terminal to submit commands to the leader node. The client communicates with the leader via a Flask server running on the specified port.

```
python3 client.py
```

### Step 3: Observing Logs

Monitor the terminal outputs of each node to observe the leader election process, log replication, and state transitions. Nodes log their current state, message receptions, and election 

**Log State Files** :

* Each server node maintains a log file in the `/logs` folder named `<node_id>.txt`, containing the committed log entries.
* You can monitor the content of these log files to observe the replicated log state across nodes.
* Log entries are written in the format: `<index>, <term>, <command>`, representing the index, term, and command of each log entry.

### Step 4: Experiment and Explore

Explore different scenarios by stopping and restarting nodes, submitting different commands from the client, and observing how the RAFT consensus algorithm maintains leadership and log consistency.

## Configuration

* Adjust configurations such as timeouts, ports, and node IDs in `config.json` as needed for testing and experimentation.

## Notes

* Ensure all nodes have unique `node_id` values and are configured correctly to communicate with each other.
* This project provides a basic simulation of the RAFT leader election process and can be extended for more complex scenarios or additional features using 3 servers and a single client.

By following these steps, you can simulate and explore the RAFT leader election process using the provided Python implementation.
