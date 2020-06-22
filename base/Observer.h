/***************************************************************************
                          Observer.h  -  Observer pattern class
                             -------------------
    begin                : Mon Jun 22 2020
    copyright            : (C) 2020 by c30zD
    email                : c30zD.x@gmail.com
***************************************************************************/

#ifndef OBSERVER_INTERFACE_H
#define OBSERVER_INTERFACE_H
class Observer {
  public:
    virtual void ~Observer();
    virtual void update() = 0;
}
#endif
