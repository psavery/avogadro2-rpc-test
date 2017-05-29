/**********************************************************************
  Avogadro2 Rpc Test

  Copyright (C) 2017 by Patrick S. Avery

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

 ***********************************************************************/

#include <QApplication>
#include <QByteArray>
#include <QDataStream>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocalSocket>

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

  // Now let's set up a datastream to write to it
  QDataStream dataStream(&socket);
  dataStream.setVersion(QDataStream::Qt_4_8);

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
  // We have to process events to get the message sent to Avogadro2
  app.processEvents();
  socket.disconnectFromServer();

  return 0;
}
