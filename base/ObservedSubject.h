/***************************************************************************
                  ObservedSubject.h  -  Observer pattern class (Subject)
                             -------------------
    begin                : Mon Jun 22 2020
    copyright            : (C) 2020 by c30zD
    email                : c30zD.x@gmail.com
***************************************************************************/

#ifndef OBSERVED_SUBJECT_H
#define OBSERVED_SUBJECT_H
#include <vector>

class Observer;

class ObservedSubject {
  private:
    std::vector<Observer*> observers;
  public:
    virtual ~ObservedSubject();
    virtual void attach(Observer *o) { observers.push_back(o); }
    virtual void detach(Observer *o);
    virtual void notify();
}

void ObservedSubject::detach(Observer *o) {
  std::vector<Observer*>::iterator it = observers.begin();
  while (it != observers.end && *it != o) {
    ++it;
  }
  if (it != observers.end)
    observers.erase(it);
}

void ObservedSubject::notify() {
  std::vector<Observer*>::iterator it = observers.begin(); 
  for (; it != observers.end(); ++it) {
    (*it)->update();
  }
}
#endif // OBSERVED_SUBJECT_H
