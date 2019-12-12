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


#include "GxyConnectionMgr.hpp"
#include "GxyModel.hpp"

class GxyFilter : public GxyModel
{
  Q_OBJECT

public:
  ~GxyFilter()
  {
    if (getTheGxyConnectionMgr()->IsConnected())
    {
      QJsonObject json;
      json["cmd"] = "gui::remove_filter";
      json["id"] = model_identifier.c_str();

      QJsonDocument doc(json);
      QByteArray bytes = doc.toJson(QJsonDocument::Compact);
      QString s = QLatin1String(bytes);

      std::string msg = s.toStdString();
      std::cerr << "request: " << msg << "\n";
      getTheGxyConnectionMgr()->CSendRecv(msg);
      std::cerr << "reply: " << msg << "\n";
    }
  }
};
