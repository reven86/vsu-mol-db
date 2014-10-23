#include "pch.h"
#include "settings.h"



Settings::Settings()
    : _sidebarWidth(0.0f)
{
    _molecule.reset(Molecule::create(std::vector<Molecule::Atom>(), std::vector<Molecule::Link>()));
}

Settings::~Settings()
{
}

void Settings::setSidebarWidth(float width)
{
    _sidebarWidth = width;
    sidebarWidthChangedSignal(width);
}