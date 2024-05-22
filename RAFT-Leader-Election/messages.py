import logging
from utils import parse_config

logger = logging.getLogger("raft_logger")
config = parse_config()


class AppendEntryArgs:
    def __init__(
        self,
        term,
        leader_id,
        entries,
        prev_log_index,
        prev_log_term,
        leader_commit,
        sent_length,
        acked_len,
    ):
        self.term = term
        self.entries = entries
        self.leader_id = leader_id
        self.acked_length = acked_len
        self.sent_length = sent_length
        self.prev_log_term = prev_log_term
        self.leader_commit = leader_commit
        self.prev_log_index = prev_log_index


class AppendEntryReply:
    def __init__(self, follower_id, term, acked_len, success):
        self.term = term
        self.success = success
        self.acked_len = acked_len
        self.follower_id = follower_id


class RequestVoteArgs:
    def __init__(self, term, candidate_id, last_log_index, last_log_term):
        self.term = term
        self.candidate_id = candidate_id
        self.last_log_term = last_log_term
        self.last_log_index = last_log_index


class RequestVoteReply:
    def __init__(self, voter_id, term, vote_granted):
        self.term = term
        self.voter_id = voter_id
        self.vote_granted = vote_granted
