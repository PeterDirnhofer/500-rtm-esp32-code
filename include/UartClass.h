// Header DECLARATION
// https://marketplace.visualstudio.com/items?itemName=FleeXo.cpp-class-creator
#ifndef UARTCLASS_H
#define UARTCLASS_H

#pragma once


class UartClass
{
  public:                              // öffentlich
    UartClass();                      // der Default-Konstruktor
    ~UartClass(); 
   

  private:                             // privat
    int m_eineVariable;
};

#endif