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

#include "SocketHandler.h"

class GxyConnectionMgr; 
extern GxyConnectionMgr *getTheGxyConnectionMgr();

class GxyConnectionMgr : public QObject, public gxy::SocketHandler
{
  Q_OBJECT

public:
  GxyConnectionMgr()
  {
    server = QString("localhost");
    port = QString("5001");
    dlg = NULL;
  }

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
    connect(server_le, SIGNAL(editingFinished()), this, SLOT(server_entered()));
    grid_layout->addWidget(server_le, 0, 1);
    
    grid_layout->addWidget(new QLabel("port"), 1, 0);
    
    port_le = new QLineEdit();
    port_le->setText(port);
    port_le->setValidator(new QIntValidator());
    connect(port_le, SIGNAL(editingFinished()), this, SLOT(port_entered()));
    grid_layout->addWidget(port_le, 1, 1);

    layout->addWidget(grid);

    QFrame *hbox = new QFrame();
    QHBoxLayout *hbox_layout = new QHBoxLayout();
    hbox->setLayout(hbox_layout);

    connect_button = new QPushButton("connect");
    connect_button->setEnabled(false);
    connect(connect_button, SIGNAL(released()), this, SLOT(connectToServerDialogConnect()));
    hbox_layout->addWidget(connect_button);

    QPushButton *done = new QPushButton("cancel");
    connect(done, SIGNAL(released()), dlg, SLOT(close()));
    hbox_layout->addWidget(done);

    layout->addWidget(hbox);
  }
      
  bool isConnected() { return _isConnected; }

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

  bool connectToServer()
  {
    // int   port = atoi(port_le->text().toStdString().c_str());
    // char *host = (char *)server_le->text().toStdString().c_str();

    int   p = atoi(port.toStdString().c_str());
    char *s = (char *)server.toStdString().c_str();

    std::cerr << "XXX" << s << "XXX" << p << "XXX\n";

    _isConnected = Connect(s, p);
    if (_isConnected)
    {
      std::string sofile = "load libgxy_module_data.so";
      if (! CSendRecv(sofile))
      {
        std::cerr << "Sending sofile failed\n";
        exit(1);
      }
      if (dlg) dlg->hide();
      emit connectionStateChanged(true);
      return true;
    }
    else
    {
      QMessageBox msgBox;
      msgBox.setText("Unable to make connection");
      msgBox.exec();
      return false;
    }
  }

signals:

  void connectionStateChanged(bool b);

public Q_SLOTS:

  void openConnectToServerDialog()
  {
    if (! dlg)
      createDialog();
    dlg->show();
    dlg->exec();
  }

  void server_entered()
  {
    sset = true;
    server = server_le->text();
    if (pset) 
    {
      just_enabled = true;
      connect_button->setEnabled(true);
    }
  }

  void port_entered()
  {
    pset = true;
    port = port_le->text();
    if (sset) 
    {
      just_enabled = true;
      connect_button->setEnabled(true);
    }
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
      emit connectionStateChanged(true);
    }
  }

private:
  
  bool _isConnected = false;
  QPushButton *connect_button;

  QDialog *dlg = NULL;
  bool pset = false, sset = false, just_enabled = false;
  QLineEdit *port_le, *server_le;
  QString server;
  QString port;
};

