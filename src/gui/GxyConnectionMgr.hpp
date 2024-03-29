// ========================================================================== //
// Copyright (c) 2014-2020 The University of Texas at Austin.                 //
// All rights reserved.                                                       //
//                                                                            //
// Licensed under the Apache License, Version 2.0 (the "License");            //
// you may not use this file except in compliance with the License.           //
// A copy of the License is included with this software in the file LICENSE.  //
// If your copy does not contain the License, you may obtain a copy of the    //
// License at:                                                                //
//                                                                            //
//     https://www.apache.org/licenses/LICENSE-2.0                            //
//                                                                            //
// Unless required by applicable law or agreed to in writing, software        //
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT  //
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.           //
// See the License for the specific language governing permissions and        //
// limitations under the License.                                             //
//                                                                            //
// ========================================================================== //

#pragma once

#include <iostream>
#include <sstream>
#include <vector>

#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QFrame>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QMessageBox>

#include <QtGui/QIntValidator>
#include <QtGui/QDoubleValidator>

#include "Observer.h"
#include "SocketHandler.h"
#include "GxyRenderWindowMgr.hpp"


class GxyConnectionMgr : public QObject, public gxy::SocketHandler, public Observer
{
  Q_OBJECT

public:
  GxyConnectionMgr();

  void createDialog()
  {
    dlg = new QDialog;

    dlg->setModal(true);
    QVBoxLayout *layout = new QVBoxLayout();
    dlg->setLayout(layout);

    QFrame *grid = new QFrame();
    QGridLayout *grid_layout = new QGridLayout();
    grid->setLayout(grid_layout);

    grid_layout->addWidget(new QLabel("server"), 0, 0);
    
    server_le = new QLineEdit();
    server_le->setText(server);
    grid_layout->addWidget(server_le, 0, 1);
    
    grid_layout->addWidget(new QLabel("port"), 1, 0);
    
    port_le = new QLineEdit();
    port_le->setText(port);
    port_le->setValidator(new QIntValidator());
    grid_layout->addWidget(port_le, 1, 1);

    layout->addWidget(grid);

    QFrame *hbox = new QFrame();
    QHBoxLayout *hbox_layout = new QHBoxLayout();
    hbox->setLayout(hbox_layout);

    connect_button = new QPushButton("connect");
    connect(connect_button, SIGNAL(released()), this, SLOT(connectToServerDialogConnect()));
    hbox_layout->addWidget(connect_button);

    QPushButton *done = new QPushButton("cancel");
    connect(done, SIGNAL(released()), dlg, SLOT(close()));
    hbox_layout->addWidget(done);

    connect(this, SIGNAL(connectionStateChanged(bool)), this, SLOT(onConnectionStateChanged(bool)));

    layout->addWidget(hbox);
  }
      
  void setServer(char *s)
  {
    QString qs = s;
    setServer(qs);
  }

  void setServer(QString s)
  {
    server = s;
    server_le->setText(server);
    sset = true;
  }

  void setPort(char *p)
  {
    QString ps = p;
    setPort(ps);
  }

  void setPort(QString p)
  {
    port = p;
    port_le->setText(port);
    pset = true;
  }

  bool connectToServer();
  void disconnectFromServer();

  void addModule(std::string m)
  {
    modules.push_back(m);
    if (IsConnected())
    {
      std::string cmd = std::string("load ") + m;
      if (! CSendRecv(cmd))
      {
        QMessageBox msgBox;
        msgBox.setText("Unable to load module");
        msgBox.exec();
      }
    }
  }

  void EventHandler()
  {
    char *msg = NULL; int n;
    ERecv(msg, n);

    if (msg != NULL)
    {
      NotifyObservers(Observer::Updated, (void *)msg);
      free(msg);
    }
  }

signals:

  void connectionStateChanged(bool b);

public Q_SLOTS:

  void onConnectionStateChanged(bool b)
  {
    connect_button->setEnabled(IsConnected());
  }

  void openConnectToServerDialog()
  {
    if (! dlg)
      createDialog();
    dlg->show();
    dlg->exec();
  }

  void connectToServerDialogConnect()
  {
    server = server_le->text();
    port   = port_le->text();

    if (just_enabled)
      just_enabled = false;
    else if (connectToServer())
    {
      if (dlg) dlg->hide();
      Q_EMIT connectionStateChanged(true);
    }
  }

private:
  
  QPushButton *connect_button;

  std::vector<std::string> modules;

  // bool is_connected = false;
  QDialog *dlg = NULL;
  bool pset = false, sset = false, just_enabled = false;
  QLineEdit *port_le, *server_le;
  QString server;
  QString port;
};

GxyConnectionMgr *getTheGxyConnectionMgr();
