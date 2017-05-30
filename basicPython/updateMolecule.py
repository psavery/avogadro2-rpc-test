#!/usr/bin/env python3

from PyQt5.QtCore import QByteArray
from PyQt5.QtCore import QDataStream
from PyQt5.QtCore import QIODevice
from PyQt5.QtNetwork import QLocalSocket
from PyQt5.QtWidgets import QApplication

import json

import sys

# We need to process events later
app = QApplication(sys.argv)

# Connect to the server
serverName = "avogadro"
socket = QLocalSocket()
socket.connectToServer(serverName)

# Are we connected?
if not socket.isOpen():
  sys.exit("Failed to connect to server: " + serverName)

# Create the json input
params = {}
params["format"] = "xyz"
params["content"] =  "5\n" \
                     "XYZ file\n" \
                     "C      0.00000    0.00000    0.00000\n" \
                     "H      0.00000    0.00000    1.08900\n" \
                     "H      1.02672    0.00000   -0.36300\n" \
                     "H     -0.51336   -0.88916   -0.36300\n" \
                     "H     -0.51336    0.88916   -0.36300"

message = {}
message["jsonrpc"] = "2.0";
message["id"] = "1";
message["method"] = "loadMolecule";
message["params"] = params;

# Convert the json input into a QByteArray
json_data = json.dumps(message)
jsonMessage = QByteArray()
jsonMessage.append(json_data)

# Send the message via a datastream
dataStream = QDataStream(socket)
dataStream.setVersion(QDataStream.Qt_4_8)
dataStream << jsonMessage

# We have to process events so that the socket is written to
app.processEvents()
