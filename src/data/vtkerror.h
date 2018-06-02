#include <iostream>
#include <vtkAlgorithm.h>
#include <vtkCommand.h>
#include <vtkExecutive.h>

class VTKError : public vtkCommand
{
public:
  VTKError(): Error(false), Warning(false) {}
  static VTKError *New() { return new VTKError; }
  bool GetError() const { return this->Error; }
  bool GetWarning() const { return this->Warning; }
  
  void watch(vtkAlgorithm *a)
  { 
    a->AddObserver(vtkCommand::ErrorEvent, this);
    a->AddObserver(vtkCommand::WarningEvent, this);
    a->GetExecutive()->AddObserver(vtkCommand::ErrorEvent, this);
    a->GetExecutive()->AddObserver(vtkCommand::WarningEvent, this);
  }
  
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


