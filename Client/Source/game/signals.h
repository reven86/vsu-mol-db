#pragma once

#ifndef __SIGNALS_H__
#define __SIGNALS_H__




struct GameSignals : public Singleton< GameSignals >
{
    friend class Singleton< GameSignals >;

protected:
    GameSignals() {};
    ~GameSignals() {};
};



#endif