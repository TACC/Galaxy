// ========================================================================== //
// Copyright (c) 2014-2019 The University of Texas at Austin.                 //
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

#include <QtCore/QObject>
#include <QtCore/QVector>
#include <QtCore/QUuid>

#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QDialog>
#include <QtWidgets/QMessageBox>

#include <nodes/NodeDataModel>

using QtNodes::NodeData;
using QtNodes::NodeDataModel;
using QtNodes::NodeDataType;
using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeValidationState;

#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include <iostream>

#include "GxyConnectionMgr.hpp"
#include "GxyModel.hpp"
#include "GxyData.hpp"
#include <iostream>

class MyQListWidgetItem : public QListWidgetItem
{
public:
  MyQListWidgetItem(GxyDataInfo& di)  : QListWidgetItem(di.name.c_str()), dataInfo(di)
  {
    dlg = new QDialog;
    dlg->setFixedWidth(300);
    dlg->setModal(true);
    QGridLayout *gl = new QGridLayout;
    gl->setContentsMargins(0, 0, 0, 0);
    gl->setSpacing(0);
    dlg->setLayout(gl);

    int row = 0;
    gl->addWidget(new QLabel("name"), row, 0);
    gl->addWidget(new QLabel(di.name.c_str()), row++, 1);

    gl->addWidget(new QLabel("key"), row, 0);
    gl->addWidget(new QLabel(QString::number(di.key)), row++, 1);

    gl->addWidget(new QLabel("type"), row, 0);
    gl->addWidget(new QLabel(di.type == 0 ? "Volume" : di.type == 1 ? "Mesh" : "Particles"), row++, 1);

    gl->addWidget(new QLabel("vector?"), row, 0);
    gl->addWidget(new QLabel(di.type != 0 ? "n/a" : di.isVector ? "yes" : "no"), row++, 1);
    
    gl->addWidget(new QLabel("data range"), row, 0);
    gl->addWidget(new QLabel(QString::number(di.data_max, 'g', 4)), row, 1);
    gl->addWidget(new QLabel(QString::number(di.data_min, 'g', 4)), row++, 2);

    gl->addWidget(new QLabel("X span"), row, 0);
    gl->addWidget(new QLabel(QString::number(di.box[0], 'g', 4)), row, 1);
    gl->addWidget(new QLabel(QString::number(di.box[1], 'g', 4)), row++, 2);

    gl->addWidget(new QLabel("Y span"), row, 0);
    gl->addWidget(new QLabel(QString::number(di.box[2], 'g', 4)), row, 1);
    gl->addWidget(new QLabel(QString::number(di.box[3], 'g', 4)), row++, 2);

    gl->addWidget(new QLabel("Z span"), row, 0);
    gl->addWidget(new QLabel(QString::number(di.box[4], 'g', 4)), row, 1);
    gl->addWidget(new QLabel(QString::number(di.box[5], 'g', 4)), row++, 2);

    QPushButton *done = new QPushButton("done");
    dlg->connect(done, SIGNAL(released()), dlg, SLOT(accept()));
    gl->addWidget(done, row++, 0);
  }

  ~MyQListWidgetItem()
  {
    delete dlg;
  }

  void showDialog() { dlg->show(); }
  void getDataInfo(GxyDataInfo& di) { di = dataInfo; }
  GxyDataInfo getDataInfo() { return dataInfo; }

private:
  QDialog *dlg = NULL;
  GxyDataInfo dataInfo;
};

class MyQListWidget : public QListWidget
{
  Q_OBJECT

public: 
  MyQListWidget() : QListWidget()
  {
  }

public Q_SLOTS:

  void showDialog() 
  {
    if (selectedItems().length() > 0)
    {
      ((MyQListWidgetItem *)currentItem())->showDialog();
    }
    else
    {
      QMessageBox m;
      m.setText("no item selected!\n");
      m.exec();
    }
  }
};

class LoadDataDialog : public QDialog
{
  Q_OBJECT

public:
  LoadDataDialog()
  {
    QFrame *frame = new QFrame();
    QVBoxLayout *layout = new QVBoxLayout();
    frame->setLayout(layout);

    QWidget *chooser = new QWidget();
    QHBoxLayout *clayout = new QHBoxLayout();
    chooser->setLayout(clayout);

    clayout->addWidget(new QLabel("File:"));

    fileName = new QLineEdit();
    connect(fileName, SIGNAL(editingFinished()), this, SLOT(validateOpen()));
    clayout->addWidget(fileName);

    QPushButton *openBrowser = new QPushButton("...");
    connect(openBrowser, SIGNAL(released()), this, SLOT(onFileSelectorOpen()));
    openBrowser->setAutoDefault(false);
    clayout->addWidget(openBrowser);

    layout->addWidget(chooser);

    QWidget *pbox = new QWidget();
    QHBoxLayout *playout = new QHBoxLayout();
    pbox->setLayout(playout);

    lbl = new QLabel("Name:");
    playout->addWidget(lbl);

    dataName = new QLineEdit();
    connect(dataName, SIGNAL(editingFinished()), this, SLOT(validateOpen()));
    playout->addWidget(dataName);

    layout->addWidget(pbox);

    QFrame *typeFrame = new QFrame();
    QVBoxLayout *typeLayout = new QVBoxLayout();
    typeFrame->setLayout(typeLayout);

    typeButtonGroup = new QButtonGroup();

    QRadioButton *button = new QRadioButton("Volume");
    typeLayout->addWidget(button);
    typeButtonGroup->addButton(button, 0);

    button = new QRadioButton("TriangleMesh");
    typeLayout->addWidget(button);
    typeButtonGroup->addButton(button, 1);

    button = new QRadioButton("Points");
    typeLayout->addWidget(button);
    typeButtonGroup->addButton(button, 2);

    button = new QRadioButton("PathLines");
    typeLayout->addWidget(button);
    typeButtonGroup->addButton(button, 3);

    connect(typeButtonGroup, SIGNAL(buttonClicked(int)), this, SLOT(dataTypeSet(int)));

    layout->addWidget(typeFrame);

    auto bb = new QWidget();
    auto bbl = new QHBoxLayout();
    bb->setLayout(bbl);

    auto cancel = new QPushButton("Cancel");
    cancel->setAutoDefault(false);
    connect(cancel, SIGNAL(released()), this, SLOT(reject()));
    bbl->addWidget(cancel);
    
    openButton = new QPushButton("Open");
    openButton->setEnabled(false);
    openButton->setAutoDefault(false);
    connect(openButton, SIGNAL(released()), this, SLOT(accept()));
    bbl->addWidget(openButton);
    
    layout->addWidget(bb);

    setLayout(layout);
  } 

  int getDataType() { return typeButtonGroup->checkedId(); }
  std::string getFileName() { return fileName->text().toStdString(); }
  std::string getDataName() { return dataName->text().toStdString(); }

private Q_SLOTS:

  void validateOpen()
  {
    openButton->setEnabled(((getFileName().length() > 0) && (getDataName().length() > 0)) && (getDataType() != -1));
  }
    
  void dataTypeSet(int i)
  {
    lbl->setEnabled(i != 0);
    dataName->setEnabled(i != 0);
    validateOpen();
  }

  void enableDataName(int i)
  {
    lbl->setEnabled(i != 0);
    dataName->setEnabled(i != 0);
    validateOpen();
  }

  void onFileSelectorOpen()
  {
    QFileDialog *fileDialog = new QFileDialog();
    if (fileDialog->exec())
    {
      fileName->insert(fileDialog->selectedFiles().at(0));
    }
    delete fileDialog;
    validateOpen();
  }

private:
  QButtonGroup *typeButtonGroup;
  QLineEdit *fileName;
  QLineEdit *dataName;
  QPushButton *openButton;
  QLabel *lbl;
};

class DataSourceModel : public GxyModel
{
  Q_OBJECT

public:
  static void init()
  {
    static bool first = true;
    if (first)
    {
      first = false;
      // getTheGxyConnectionMgr()->addModule("libgxy_module_data.so");
    }
  }

  DataSourceModel();

  virtual
  ~DataSourceModel() {}

  unsigned int nPorts(PortType portType) const override;

  NodeDataType dataType(PortType portType, PortIndex portIndex) const override;

  std::shared_ptr<NodeData> outData(PortIndex port) override;

  void setInData(std::shared_ptr<NodeData> data, PortIndex portIndex) override;

  NodeValidationState validationState() const override;

  QString validationMessage() const override;

  QString caption() const override { return QStringLiteral("DataSource"); }

  QString name() const override { return QStringLiteral("DataSource"); }

  bool isValid() override { return current_selection != NULL; }

private Q_SLOTS:

  void selection(QListWidgetItem *item)
  {
    std::cerr << "Data... selection!\n";
    current_selection = (MyQListWidgetItem *)item;
    info->setEnabled(true);
    enableIfValid();
  }

  void onApply() override
  {
    if (current_selection)
    {
      std::cerr << "Data.... onApply... current_selection IS SET\n";
      output->dataInfo = current_selection->getDataInfo();
      output->setValid(true);
      Q_EMIT dataUpdated(0);
    }
    else
      std::cerr << "Data.... onApply... current_selection IS NOT SET\n";
  }

  void onAdd()
  {
    GxyConnectionMgr *gxyMgr = getTheGxyConnectionMgr();

    if (! gxyMgr->IsConnected())
    {
      QMessageBox msgBox;
      msgBox.setText("Not connected");
      msgBox.exec();
      return;
    }

    LoadDataDialog *dlg = new LoadDataDialog();
    auto r = dlg->exec();

    if (r)
    {
      rapidjson::Document doc;
      doc.SetObject();

      rapidjson::Value dsets(rapidjson::kObjectType);
      std::string cmd("gui::import");
      dsets.AddMember("cmd", rapidjson::Value().SetString(cmd.c_str(), cmd.length()+1), doc.GetAllocator());

      std::string dataName = dlg->getDataName().c_str();
      std::string fileName = dlg->getFileName().c_str();

      dsets.AddMember("name", rapidjson::Value().SetString(dataName.c_str(), dataName.length()+1), doc.GetAllocator());
      dsets.AddMember("filename", rapidjson::Value().SetString(fileName.c_str(), fileName.length()+1), doc.GetAllocator());

      switch (dlg->getDataType())
      {
        case 0: dsets.AddMember("type", "Volume", doc.GetAllocator()); break;
        case 1: dsets.AddMember("type", "Triangles", doc.GetAllocator()); break;
        case 2: dsets.AddMember("type", "Particles", doc.GetAllocator()); break;
        case 3: dsets.AddMember("type", "PathLines", doc.GetAllocator()); break;
        default: dsets.AddMember("type", "??????????", doc.GetAllocator()); break;
      }

      doc.AddMember("Datasets", dsets, doc.GetAllocator());

      rapidjson::StringBuffer strbuf;
      rapidjson::Writer<rapidjson::StringBuffer> writer(strbuf);
      doc.Accept(writer);

      std::string line = std::string("import ") + strbuf.GetString();
      std::cerr << line << "\n";

      gxyMgr->CSendRecv(line);

      std::stringstream ss(line);

      std::string status;
      ss >> status;

      if (status != "ok")
      {
        QMessageBox msgBox;
        msgBox.setText(line.c_str());
        msgBox.exec();
      }
      else
        onRefresh();
    }
  }

  void onRefresh()
  {
    info->setEnabled(false);
    objectList->clear();

    GxyConnectionMgr *gxyMgr = getTheGxyConnectionMgr();
    if (! gxyMgr->IsConnected())
    {
      QMessageBox msgBox;
      msgBox.setText("Not connected");
      msgBox.exec();
    }
    else
    {
      rapidjson::Document doc;
      doc.SetObject();

      std::string cmd("gui::list");
      doc.AddMember("cmd", rapidjson::Value().SetString(cmd.c_str(), cmd.length()+1), doc.GetAllocator());
      rapidjson::StringBuffer strbuf;
      rapidjson::Writer<rapidjson::StringBuffer> writer(strbuf);
      doc.Accept(writer);

      std::string line = strbuf.GetString();
      gxyMgr->CSendRecv(line);

      std::stringstream ss(line);

      std::string s;
      ss >> s;

      if (s != "ok")
      {
        QMessageBox msgBox;
        msgBox.setText((std::string("Error: ") + line).c_str());
        msgBox.exec();
      }
      else
      {
        rapidjson::Document rply;
        std::getline(ss, line);

        rply.Parse(line.c_str());

        rapidjson::Value& array = rply["Datasets"];

        for (auto i = 0; i < array.Size(); i++)
        {
          rapidjson::Value& dset = array[i];
          GxyDataInfo datainfo;
          datainfo.name = dset["name"].GetString();
          datainfo.key = dset["key"].GetInt();
          datainfo.type = dset["type"].GetInt();
          datainfo.isVector = dset["isVector"].GetBool();
          datainfo.data_min = dset["min"].GetDouble();
          datainfo.data_max = dset["max"].GetDouble();
          for (auto i = 0; i < 6; i++)
            datainfo.box[i] = dset["box"][i].GetDouble();

          MyQListWidgetItem *mlwi = new MyQListWidgetItem(datainfo);
          objectList->addItem(mlwi);
        }
      }
    }
  }

private:
  MyQListWidget *objectList;
  MyQListWidgetItem *current_selection = NULL;
  QPushButton *info;
  std::shared_ptr<GxyData> output;
};
