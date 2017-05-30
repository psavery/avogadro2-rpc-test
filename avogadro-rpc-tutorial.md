Communicating with Avogadro2 via RPC
====================================
By Patrick Avery
----------------

Table of Contents
=================

-   [1: Prerequesites and Important Notes](#1-prerequesites-and-important-notes)
-   [2: Basic Message in C++](#2-basic-message-in-c)
-   [3: Basic Message in Python](#3-basic-message-in-python)
-   [4: Receiving Messages Back (C++)](#4-receiving-messages-back-c)

1: Prerequesites and Important Notes
------------------------------------
The Avogadro2 RPC interface is a powerful tool as it allows you to load a
molecule into the Avogadro2 GUI from external programs. This means that you
can use Avogadro2 to render molecules for your program without having to link
with it. In fact, you can set up your program so that a user can simply open
Avogadro2 if they want selected molecules to be rendered, and if they do not
want them rendered, they just doesn't open Avogadro2.

Avogadro2 uses [QLocalServer](http://doc.qt.io/qt-5/qlocalserver.html) from
the Qt library. If you wish to communicate with it, it is easiest to use the
Qt API within your own program as well. It is possible to communicate with
the Avogadro2 server without Qt, but I do not discuss that in this tutorial.

Avogadro2 also uses a
[Json RPC specification](http://www.jsonrpc.org/specification) for the
inter-process communication. So you may want to familiarize yourself with
that.

In this tutorial, I will demonstrate how to send a basic molecule update to
Avogadro2 using C++ and Python. I will also demonstrate how to write a class
in C++ that keeps the connection open and listens for return messages
(which is very helpful to have if this is part of a larger program).

2: Basic Message in C++
-----------------------
A few Qt classes are needed in sending an RPC message to Avogadro2
in C++: QApplication, QByteArray, QDataStream, QJsonDocument, QJsonObject,
and QLocalSocket.

Basically, what we must do is connect to the server with a QLocalSocket,
generate a QByteArray that contains the json data, connect a QDataStream
to the local socket, and then send the json data through the data stream
to the server. We need the QApplication because the local socket
communication is scheduled as an event, and we need to process the event
in order for the message to be sent (this step will not be necessary if
this function is part of the event loop in a larger program).

First, we connect to the server with a QLocalSocket:

```
QLocalSocket socket;
QString serverName = "avogadro";
socket.connectToServer(serverName);

// Did we successfully connect?
if (!socket.isOpen()) {
  qDebug() << "Failed to connect to server: " << serverName;
  return 1;
}
```

If the Avogadro2 server does not exist, socket.isOpen() will return
false. In a larger program, if the socket fails to connect,
you can probably just assume that the user does not want to render their
molecules and the program can just go about its business.

Next, we need to generate the json rpc data and store it in a QByteArray.
An example of this is given for an XYZ file for methane:

```
QJsonObject params;
params["format"] = "xyz";
// Methane
params["content"] = "5\n"
                    "XYZ file\n"
                    "C      0.00000    0.00000    0.00000\n"
                    "H      0.00000    0.00000    1.08900\n"
                    "H      1.02672    0.00000   -0.36300\n"
                    "H     -0.51336   -0.88916   -0.36300\n"
                    "H     -0.51336    0.88916   -0.36300";

QJsonObject message;
message["jsonrpc"] = QString("2.0");
message["id"] = QString::number(1);
message["method"] = QString("loadMolecule");
message["params"] = params;

QJsonDocument document(message);
QByteArray data = document.toJson();
```

You can increment the "id" every time you call it. For our case,
we can just leave it at "1". In addition, there is another method
other than loadMolecule: "openFile". This just instructs the program
to open a particular file (given by a param: "fileName"). I'm just going
to use "loadMolecule" as the example.

Finally, we need to connect a QDataStream to the local socket and send the
data to Avogadro2.

```
QDataStream dataStream(&socket);
dataStream.setVersion(QDataStream::Qt_4_8);
dataStream << data;
```

Avogadro2 uses a QDataStream version of Qt 4.8. I haven't found this to be
important as of yet, but we should match their version just in case.

Unfortunately, there also seems to be a bug with Windows that involves it
not sending all the data in a single packet to the QLocalServer. If you are
using windows, replace the last code block with the following section:

```
QDataStream dataStream(&socket);
dataStream.setVersion(QDataStream::Qt_4_8);
QByteArray byteArray;
QDataStream stream(&byteArray, QIODevice::WriteOnly);
stream << data;
dataStream.writeRawData(byteArray, byteArray.size());
```

This requires a few more steps, but it ensures that all of the data is sent
in a single packet at once.

If your program is not in an event loop, you will also need to process the
send (and waiting a short time before doing so is helpful so that the event
has enough time to prepare):

```
QThread::msleep(10);
app.processEvents()
```
And that's it! If all went fine, and you have Avogadro2 open, a methane
molecule should appear in the window. Here is a complete file that you
should be able to compile and test (you can also find this file with a
suitable CMakeLists.txt file
[here](https://github.com/psavery/avogadro2-rpc-test/tree/master/basicCpp)).

```
#include <QApplication>
#include <QByteArray>
#include <QDataStream>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocalSocket>
#include <QThread>

int main(int argc, char* argv[])
{
  QApplication app(argc, argv);
  // First, let's try to connect to the local server
  QLocalSocket socket;
  QString serverName = "avogadro";
  socket.connectToServer(serverName);

  // Did we successfully connect?
  if (!socket.isOpen()) {
    qDebug() << "Failed to connect to server: " << serverName;
    return 1;
  }

  QJsonObject params;
  params["format"] = "xyz";
  // Methane
  params["content"] = "5\n"
                      "XYZ file\n"
                      "C      0.00000    0.00000    0.00000\n"
                      "H      0.00000    0.00000    1.08900\n"
                      "H      1.02672    0.00000   -0.36300\n"
                      "H     -0.51336   -0.88916   -0.36300\n"
                      "H     -0.51336    0.88916   -0.36300";

  QJsonObject message;
  message["jsonrpc"] = QString("2.0");
  message["id"] = QString::number(1);
  message["method"] = QString("loadMolecule");
  message["params"] = params;

  QJsonDocument document(message);
  QByteArray data = document.toJson();

  // Now let's set up a datastream to write to it
  QDataStream dataStream(&socket);
  dataStream.setVersion(QDataStream::Qt_4_8);

#ifdef _WIN32
  // Windows can't do QDataStream << QByteArray correctly if the device is a
  // QLocalSocket because it will send two packets. We need to send the data
  // manually to prevent issues.
  QByteArray byteArray;
  QDataStream stream(&byteArray, QIODevice::WriteOnly);
  stream << data;
  dataStream.writeRawData(byteArray, byteArray.size());
#else
  // Easy way of sending the data.
  dataStream << data;
#endif
  // We need to wait a short amount of time before processing events
  QThread::msleep(10);
  // We have to process events to get the message sent to Avogadro2
  app.processEvents();
  socket.disconnectFromServer();

  return 0;
}
```

3: Basic Message in Python
--------------------------
Similar to the C++ method given above, you can use PyQt to send and receive
messages with Avogadro2. A lot of the logic in the code is explained in the
C++ section, so if you want to read about some of the logic, I would advise
you to look in the previous section. Here, I will just show a section of
code that does the same kind of thing (except in Python).

One of the few differences is that I used the standard Python json library
instead of the QJson classes. This is only because I found it simpler with
the Python interface. Please also note that you may run into the same packet
problem on the Windows operating system and you may need to write the raw data
directly into the stream for Windows (as explained in the prior section).

You can also find this file
[here](https://github.com/psavery/avogadro2-rpc-test/tree/master/basicPython)

```
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
```

4: Receiving Messages Back (C++)
--------------------------------
If you wish to receive messages back from Avogadro2, you will need to keep
the socket open and make a connection for the event handler. Such a connection
will look something like this:

```
connect(&m_socket, &QLocalSocket::readyRead,
        this, &RpcConnection::readData);
```

This works because QLocalSocket emits the signal, readyRead(), when it
receives a packet. You then can read from QLocalSocket as follows:

```
QByteArray data;
QDataStream stream(&m_socket);
stream >> data;

QJsonDocument doc = QJsonDocument::fromJson(data);
QJsonObject obj = doc.object();
if (obj.contains("error")) {
  // Read the error message
}
```

Of course, you can add several checks in there to make sure that the data
can be converted to a QJsonDocument, the QJsonDocument can be converted to
a QJsonObject, etc. But that is the gist of it.

In order to keep the program alert as to when the socket connects and
disconnects, you can also create a few other connections like so:

```
connect(&m_socket, &QLocalSocket::connected,
        [this](){ this->setIsConnected(true); });
connect(&m_socket, &QLocalSocket::disconnected,
        [this](){ this->setIsConnected(false); });
```

And you can receive error messages from the socket itself like so:

```
connect(&m_socket,
        static_cast<void(QLocalSocket::*)(QLocalSocket::LocalSocketError)>
          (&QLocalSocket::error),
        [this](QLocalSocket::LocalSocketError socketError)
        {
          qDebug() << "RpcConnection received a socket error: "
                   << this->m_socket.errorString();
        });
```

Anyways, if you want to see all of that put together in a single class, I
have an example you can try out
[here](https://github.com/psavery/avogadro2-rpc-test/tree/master/dialogConnectionCpp).
If you integrate a class like that into your program, you can allow for
seamless rendering of your molecules in an Avogadro2 instance.

Happy coding!
