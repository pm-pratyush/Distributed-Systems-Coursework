import os
import zmq
import time
import random
import pickle
import logging
import requests
import threading

from utils import parse_config
from flask import Flask, request
from multiprocessing import Queue

from messages import (
    AppendEntryArgs,
    AppendEntryReply,
    RequestVoteArgs,
    RequestVoteReply,
)

logger = logging.getLogger("raft_logger")

def save(object):
    logfolder = "state"
    if not os.path.exists(logfolder):
        os.makedirs(logfolder)
    filepath = os.path.join(logfolder, f"{object.node_id}.state")
    class_dict = {}
    saved_states = [
        "term",
        "log",
        "commit_index",
        "last_log_index",
        "last_log_term",
    ]
    for k, v in object.__dict__.items():
        if k in saved_states:
            class_dict[k] = v
    with open(filepath, "wb") as f:
        pickle.dump(class_dict, f)

def restore(object):
    logfolder = "state"
    filepath = os.path.join(logfolder, f"{object.node_id}.state")
    with open(filepath, "rb") as f:
        state = pickle.load(f)
        object.__dict__.update(state)
    logger.info(f"Server {object.node_id} rebooted.Current Log:{object.log}")