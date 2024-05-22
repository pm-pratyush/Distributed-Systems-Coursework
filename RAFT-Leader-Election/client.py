import time
from utils import parse_config
import requests
from flask import Flask, request
import threading

acked_upto = 0
commands_list = []

config = parse_config()
ports = config["client_ports"]
ports = [int(i) for i in ports]

app = Flask(__name__)


def update_commands(data):
    global acked_upto
    acked_upto += 1


@app.route("/", methods=["POST"])
def handle_post_request():
    data = request.json
    print(data)
    update_commands(data["commit_len"])
    return f'client ack updated successfully{data["commit_len"]}'


def run_server():
    print("starting flask server")
    app.run(debug=False, port=config["client_ack_port"])


def send_commands(command):
    data = {"data": command}

    for port in ports:
        url = f"http://localhost:{port}/submit"
        try:
            response = requests.post(url, data=data)
            print(response.text)
            if response.status_code == 200:
                print("command sent successfully")
                break
        except:
            pass



thread = threading.Thread(target=run_server)
thread.start()

while True:
    command = input("Enter data: ")
    commands_list.append(command)
    send_commands(command)


