import sys
import logging
import argparse

from utils import parse_config
from node import RaftNode
from console import logger

logger = logging.getLogger("raft_logger")
config = parse_config()

parser = argparse.ArgumentParser(description="Argument parser for raft server")
parser.add_argument("--node_id", type=int, help="node_id of the server")

args = parser.parse_args()
node_id = args.node_id
ports = config["client_ports"]
ports = [int(i) for i in ports]

if node_id is None or node_id < 0 or node_id >= len(config["servers"]):
    logger.error("Invalid node_id")
    sys.exit(0)

logger.info("Starting Node {}".format(node_id))
node = RaftNode(node_id, config["servers"], ports)
node.start()
