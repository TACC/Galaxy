#pragma once

class Observer
{
public:
  enum ObserverEvent
  {
    Deleted, Updated
  };

  Observer() {}

  ~Observer()
  {
    NotifyObservers(Deleted, NULL);
    for (auto o : observed)
      o->UnregisterObserver(this);
  }

  void Observe(Observer *o)
  {
    for (auto i : observed)
      if (i == o)
      {
        std::cerr << "Observer is already observing this!\n";
        return;
      }

    observed.push_back(o);
    o->RegisterObserver(this);
  }

  virtual void Notify(Observer* o, ObserverEvent id, void *cargo)
  {
    switch(id)
    {
      case Deleted:
      {
        for (auto i = observed.begin(); i != observed.end(); i++)
          if (*i == o)
          {
            observed.erase(i);
            return;
          }
        std::cerr << "Observer received a Delete from an Observable it wasn't observing!\n";
        return;
      }

      default:
      {
        std::cerr << "Base-class Observer received a Notification that should have been handed by subclass\n";
        return;
      }
    }
  }

  void RegisterObserver(Observer* o)
  {
    for (auto i = observers.begin(); i != observers.end(); i++)
      if (*i == o)
      {
        std::cerr << "Adding observer to observable that already has it!\n";
        return;
      }

    observers.push_back(o);
  }

  void UnregisterObserver(Observer *o)
  {
    for (auto i = observers.begin(); i != observers.end(); i++)
      if (*i == o)
      {
        observers.erase(i);
        return;
      }

    std::cerr << "Removing observer from observable that doesn't have it!\n";
    return;
  }

  void NotifyObservers(ObserverEvent id, void *cargo)
  {
    for (auto o : observers)
      o->Notify(this, id, cargo);
  }

private:
  std::vector<Observer*> observed;
  std::vector<Observer*> observers;
};
