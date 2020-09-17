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

#include <dirent.h>
#include <iostream>
#include <vector>
#include <string>
#include <unistd.h>

#include <QtWidgets/QGridLayout>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QFrame>
#include <QtWidgets/QLabel>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QScrollArea>
#include <QDialog>
#include <QPixmap>
#include <QMessageBox>

class CMapDialog : public QDialog
{
  Q_OBJECT  

public:
  CMapDialog(std::string o)
  {
    original = o;

    QWidget *f = new QWidget;
    QGridLayout *gl = new QGridLayout;
    f->setLayout(gl);

    row = 0;

#if __APPLE__
    char buf[1024];
    getcwd(buf, 1024);
    QString dirname((std::string(buf) + "/../colormaps").c_str());
#else
    QString dirname((std::string(getenv("GALAXY_ROOT")) + "/colormaps").c_str());
#endif
  
    load_from_dir(dirname.toStdString(), gl);

    std::string home(getenv("HOME"));

    load_from_dir(home + "/galaxy_colormaps", gl);

    QScrollArea *sa = new QScrollArea;
    sa->setWidgetResizable(true);
    sa->setGeometry(1, 1, 400, 400);
    sa->setWidget(f);

    connect(&selectionGroup, SIGNAL(buttonClicked(int)), this, SLOT(selection(int)));

    QWidget *bb = new QWidget;
    QHBoxLayout *bbl = new QHBoxLayout();
    bb->setLayout(bbl);

    QPushButton *reset = new QPushButton("Reset");
    bbl->addWidget(reset);
    connect(reset, SIGNAL(released()), this, SLOT(reset()));

    QPushButton *close = new QPushButton("Accept");
    bbl->addWidget(close);
    connect(close, SIGNAL(released()), this, SLOT(close()));

    QVBoxLayout *vl = new QVBoxLayout;
    vl->addWidget(sa);
    vl->addWidget(bb);
    setLayout(vl);
  }

signals:

    void cmap_selected(std::string s);

private Q_SLOTS:

    void selection(int i) 
    {
      emit cmap_selected(names[i]);
    }

    void reset()
    {
      emit cmap_selected(original);
      done(Rejected);
    }

    void close()
    {
      done(Accepted);
    }

private:

    void load_from_dir(std::string dirname, QGridLayout *gl)
    {
      DIR *dir = opendir(dirname.c_str());
      if (! dir) 
        return;
    
      struct dirent *ff;
      while (( ff = readdir(dir)) != NULL)
        if (strstr(ff->d_name, ".png"))
        {
          char base[1000];
          int n = strrchr(ff->d_name, '.') - ff->d_name;
          strncpy(base, ff->d_name, n);
          base[n] = '\0';

          QLabel *name = new QLabel(base);
          gl->addWidget(name, row, 0);
          
          sprintf(base, "%s/%s", dirname.c_str(), ff->d_name);
          names.push_back(base);
          QPixmap *pm = new QPixmap(base);
          QLabel *img = new QLabel;
          img->setFixedWidth(200);
          img->setPixmap(pm->scaled(img->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
          img->setScaledContents(true);
          gl->addWidget(img, row, 1);

          QRadioButton *b = new QRadioButton();
          selectionGroup.addButton(b, row);
          gl->addWidget(b, row, 2);

          row ++;
        }
    }

    int row;
    std::vector<std::string> names;
    QButtonGroup selectionGroup;
    std::string original;
};

