#include "pch.h"
#include "game/game.h"




Dictionary gameDictionary;

Settings * getSettings()
{
    return static_cast<VSUDatabale *>(gameplay::Game::getInstance())->getSettings();
}