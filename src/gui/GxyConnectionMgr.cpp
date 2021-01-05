#include "GxyConnectionMgr.hpp"

static GxyConnectionMgr *_theGxyConnectionMgr = NULL;

GxyConnectionMgr::GxyConnectionMgr()
{
  server = QString("localhost");
  port = QString("5001");
  dlg = NULL;

  _theGxyConnectionMgr = this;
}

void
GxyConnectionMgr::disconnectFromServer()
{
  getTheGxyRenderWindowMgr()->StopReceiving();
  Disconnect();
  Q_EMIT connectionStateChanged(false);
}

bool
GxyConnectionMgr::connectToServer()
{
  if (IsConnected())
  {
    std::cerr << "cannot start a service without disconnecting from prior service first\n";
    return false;
  }
  else
  {
    int   p = atoi(port.toStdString().c_str());

    std::cerr << "trying to connect to " << server.toStdString().c_str() << ":" << p << "\n";

    Connect((char *)server.toStdString().c_str(), p);

    if (IsConnected())
    {
      for (auto m : modules)
      {
        std::string cmd = std::string("load ") + m;
        if (! CSendRecv(cmd))
        {
          QMessageBox msgBox;
          msgBox.setText("Unable to load module");
          msgBox.exec();
          return false;
        }
      }

      if (dlg) dlg->hide();
      getTheGxyRenderWindowMgr()->StartReceiving();
      Q_EMIT connectionStateChanged(true);
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
}

GxyConnectionMgr *getTheGxyConnectionMgr() { return  _theGxyConnectionMgr; }
