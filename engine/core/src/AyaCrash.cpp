/*
 *  AyaCrash.cpp
 *  MacClient
 *
 *  Created by elebel on 7/20/10.
 */

#include "Debug.hpp"

void AYACRASH()
{
    Aya::Debugable::doCrash();
}

void AYACRASH(const char* message)
{
    Aya::Debugable::doCrash(message);
}
