// ========================================================================== //
// Copyright (c) 2014-2018 The University of Texas at Austin.                 //
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

/*! \file vtkerror.h 
 * \brief a wrapper class for VTK warnings and errors within Galaxy
 * \ingroup data
 */

#include <iostream>
#include <vtkAlgorithm.h>
#include <vtkCommand.h>
#include <vtkExecutive.h>

//! a wrapper class for VTK warnings and errors within Galaxy
/*! \ingroup data 
 */
class VTKError : public vtkCommand
{
public:
  VTKError(): Error(false), Warning(false) {} //!< default constructor
  static VTKError *New() { return new VTKError; } //!< create a new VTKError object
  bool GetError() const { return this->Error; } //!< does this represent an error?
  bool GetWarning() const { return this->Warning; } //!< does this represent a warning?
  
  //! add warning and error event observers to a given vtkAlgorithm
  void watch(vtkAlgorithm *a)
  { 
    a->AddObserver(vtkCommand::ErrorEvent, this);
    a->AddObserver(vtkCommand::WarningEvent, this);
    a->GetExecutive()->AddObserver(vtkCommand::ErrorEvent, this);
    a->GetExecutive()->AddObserver(vtkCommand::WarningEvent, this);
  }
  
  //! executed when a warning or error event is triggered by the watched vtkAlgorithm
  /* \sa watch */
  virtual void Execute(vtkObject *vtkNotUsed(caller), unsigned long event, void *calldata)
  { 
    switch(event)
    { 
      case vtkCommand::ErrorEvent: this->Error = true; break;
      case vtkCommand::WarningEvent: this->Warning = true; break;
    }
  }

private:      
  bool        Error;
  bool        Warning;
};


