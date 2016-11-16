#include "pch.h"
#include "game/game.h"




template<> Cache< GameplayRefAsset< gameplay::Font > > * GameplayRefAsset< gameplay::Font >::_cache = nullptr;

AppSettings * getSettings()
{
    return static_cast<VSUDatabale *>(gameplay::Game::getInstance())->getSettings();
}