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
#include <nodes/NodeDataModel>

#include <QtCore/QtCore>

using QtNodes::NodeDataType;
using QtNodes::NodeData;

class Json : public NodeData
{
public:
  
  Json() {}

  Json(QJsonDocument doc)
  {
    _content = doc.toJson();
  }
  
  Json(const QByteArray &bytes)
  {
    _content = bytes;
  }
  
  NodeDataType type() const override
  { 
    return NodeDataType {"json", "JSON"};
  }
  
  QJsonDocument getDocument()
  {
    QJsonParseError *error = new QJsonParseError();
    QJsonDocument result = QJsonDocument::fromJson(_content, error);
    if (error->error != QJsonParseError::NoError)
    {
      std::cerr << "json parse error: " << error->errorString().toStdString() << "\n";
      exit(0);
    }
    delete error;
    return result;
  }

private:

  QByteArray _content;
};

class JsonVis : public Json
{
public:
  
  JsonVis() {}

  NodeDataType type() const override
  { 
    return NodeDataType {"jsonVis", "JSONVIS"};
  }
};

