/**********************************************************************
  RpcConnection - Class for sending GUI updates to Avogadro2 through RPC

  Copyright (C) 2017 by Patrick S. Avery

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

 ***********************************************************************/

#include "rpcconnection.h"

#include <QJsonDocument>
#include <QJsonObject>

RpcConnection::RpcConnection(QObject* parent, const QString& serverName)
  : QObject(parent),
    m_isConnected(false),
    m_idCounter(0),
    m_socket(),
    m_dataStream(),
    m_serverName(serverName)
{
  m_dataStream.setDevice(&m_socket);
  m_dataStream.setVersion(QDataStream::Qt_4_8);
  // Cache whether we are connected or not when we receive the signal
  connect(&m_socket, &QLocalSocket::connected,
          [this](){ this->setIsConnected(true); });
  connect(&m_socket, &QLocalSocket::disconnected,
          [this](){ this->setIsConnected(false); });

  // If the local socket produces any errors, let's print them.
  connect(&m_socket,
          static_cast<void(QLocalSocket::*)(QLocalSocket::LocalSocketError)>
            (&QLocalSocket::error),
          [this](QLocalSocket::LocalSocketError socketError)
          {
            // We can use this in the future if we feel that we need to
            // print out errors that are emitted. Usually they just happen
            // because the user doesn't have Avogadro2 open, though...
            //qDebug() << "RpcConnection received a socket error: "
            //         << this->m_socket.errorString();
          });

  // We can read data back from the server if we want to
  connect(&m_socket, &QLocalSocket::readyRead,
          this, &RpcConnection::readData);
}

bool RpcConnection::updateDisplayedMolecule(const QString& format,
                                            const QString& data)
{
  if (!reconnectIfNeeded())
    return false;

  QJsonObject params;
  params["format"] = format;
  params["content"] = data;

  QJsonObject message;
  message["jsonrpc"] = QString("2.0");
  message["id"] = QString::number(++m_idCounter);
  message["method"] = QString("loadMolecule");
  message["params"] = params;

  return sendMessage(message);
}

bool RpcConnection::isConnected() const
{
  return m_socket.isOpen() && m_isConnected;
}

bool RpcConnection::reconnectIfNeeded()
{
  if (isConnected())
    return true;
  else
    return connectToServer();
}

bool RpcConnection::connectToServer()
{
  if (m_serverName.isEmpty())
    return false;

  if (m_socket.isOpen())
    m_socket.close();

  m_socket.connectToServer(m_serverName);
  return isConnected();
}

bool RpcConnection::sendMessage(const QJsonObject& message)
{
  if (!isConnected())
    return false;

  QJsonDocument document(message);
  QByteArray data = document.toJson();

#ifdef _WIN32
  // Windows can't do QDataStream << QByteArray correctly if the device is a
  // QLocalSocket because it will send two packets. We need to send the data
  // manually to prevent issues.
  QByteArray byteArray;
  QDataStream stream(&byteArray, QIODevice::WriteOnly);
  stream << data;
  m_dataStream.writeRawData(byteArray, byteArray.size());
#else
  // Easy way of sending the data.
  m_dataStream << data;
#endif

  return true;
}

void RpcConnection::readData()
{
  // Read it as a document
  QByteArray data;

  // If we have several packets in the datastream (because a lot of messages
  // were sent to it at once), only read the last one.
  // If we ever need to read every packet, we may change this in the future.
  while (!m_dataStream.atEnd())
    m_dataStream >> data;

  QJsonDocument doc = QJsonDocument::fromJson(data);

  if (doc.isNull()) {
    qDebug() << "In RpcConnection: JsonData received from Avogadro2 is null!";
    qDebug() << "JsonData is as follows:" << data;
    m_dataStream.resetStatus();
    return;
  }

  if (!doc.isObject()) {
    qDebug() << "In RpcConnection: JsonData received from Avogadro2 is not"
             << "an object!";
    qDebug() << "JsonData is as follows:" << data;
    return;
  }

  // Convert to QJson object
  QJsonObject obj = doc.object();

  // Check to see if there is an error
  if (obj.contains("error")) {
    // Should be structured like this, but we can change it in the future
    // if the response message changes:
    /**
     * "error": {
     *   "code": <int>,
     *   "message": <string>
     * }
     */
    QJsonObject errorObj = obj["error"].toObject();
    qDebug() << "Error received from RPC to Avogadro2";
    qDebug() << "Error code: " << errorObj["code"].toInt();
    qDebug() << "Error message: " << errorObj["message"].toString();
  }
}
