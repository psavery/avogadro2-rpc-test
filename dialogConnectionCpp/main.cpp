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

#include "rpcconnection.h"

#include <QApplication>
#include <QDialog>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QTextEdit>
#include <QVBoxLayout>

int main(int argc, char* argv[])
{
  QApplication app(argc, argv);
  RpcConnection rpcConnection;

  // Create a dialog where the user can enter their own input
  QDialog dialog;
  QVBoxLayout layout;
  dialog.setLayout(&layout);
  dialog.setWindowTitle("Update Avogadro2 Molecule");
  QLineEdit lineEdit("xyz");
  layout.addWidget(&lineEdit);

  // We'll do a default of an xyz file of methane
  QTextEdit textEdit;
  textEdit.setPlainText("5\n"
                        "XYZ file\n"
                        "C      0.00000    0.00000    0.00000\n"
                        "H      0.00000    0.00000    1.08900\n"
                        "H      1.02672    0.00000   -0.36300\n"
                        "H     -0.51336   -0.88916   -0.36300\n"
                        "H     -0.51336    0.88916   -0.36300");
  layout.addWidget(&textEdit);
  QDialogButtonBox buttons;
  buttons.addButton("Send", QDialogButtonBox::AcceptRole);
  buttons.addButton("Close", QDialogButtonBox::RejectRole);
  QObject::connect(&buttons, &QDialogButtonBox::accepted,
    [&rpcConnection, &lineEdit, &textEdit]()
    {
      rpcConnection.updateDisplayedMolecule(lineEdit.text(), textEdit.toPlainText());
    });
  QObject::connect(&buttons, SIGNAL(rejected()), &dialog, SLOT(reject()));
  layout.addWidget(&buttons);
  dialog.resize(500, 500);

  dialog.exec();
  return 0;
}
