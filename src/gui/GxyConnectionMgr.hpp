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

class GxyConnectionMgr : public QObject
{
  Q_OBJECT

public:
  GxyConnectionMgr() {}

public Q_SLOTS:

  void makeConnection()
  {
    if (! dlg)
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
      server_le->setText(QString::fromStdString(server));
      connect(server_le, SIGNAL(editingFinished()), this, SLOT(server_entered()));
      grid_layout->addWidget(server_le, 0, 1);
      
      grid_layout->addWidget(new QLabel("port"), 1, 0);
      
      port_le = new QLineEdit();
      port_le->setText(QString::fromStdString(port));
      port_le->setValidator(new QIntValidator());
      connect(port_le, SIGNAL(editingFinished()), this, SLOT(port_entered()));
      grid_layout->addWidget(port_le, 1, 1);

      layout->addWidget(grid);

      QFrame *hbox = new QFrame();
      QHBoxLayout *hbox_layout = new QHBoxLayout();
      hbox->setLayout(hbox_layout);

      connect_button = new QPushButton("connect");
      connect_button->setEnabled(false);
      connect(connect_button, SIGNAL(released()), this, SLOT(connectToServer()));
      hbox_layout->addWidget(connect_button);

      QPushButton *done = new QPushButton("cancel");
      connect(done, SIGNAL(released()), dlg, SLOT(close()));
      hbox_layout->addWidget(done);

      layout->addWidget(hbox);
    }
      
    dlg->show();
    dlg->exec();
  }

private Q_SLOTS:
  
  void server_entered()
  {
    sset = true;
    server = server_le->text().toStdString();
    if (pset) 
    {
      just_enabled = true;
      connect_button->setEnabled(true);
    }
  }

  void port_entered()
  {
    pset = true;
    port = port_le->text().toStdString();
    if (sset) 
    {
      just_enabled = true;
      connect_button->setEnabled(true);
    }
  }

  void connectToServer()
  {
    if (just_enabled)
      just_enabled = false;
    else
    {
      std::stringstream ss(port);
      int pnum;
      ss >> pnum;
      std::cerr << "connect to server: " << server << " " << pnum << "\n";
      if (! gxyConnection.Connect(server.c_str(), pnum))
      {
        QMessageBox msgBox;
        msgBox.setText("Unable to make connection");
        msgBox.exec();
      }
      else
      {
        std::string sofile = "libgxy_module_data.so";
        if (! gxyConnection.CSendRecv(sofile))
        {
          std::cerr << "Sending sofile failed\n";
          exit(1);
        }
        dlg->hide();
      }
    }
  }

private:
  
  QPushButton *connect_button;

  gxy::SocketHandler gxyConnection;

  QDialog *dlg = NULL;
  bool pset = false, sset = false, just_enabled = false;
  QLineEdit *port_le, *server_le;
  std::string server = "localhost"; 
  std::string port = "5001";
};

