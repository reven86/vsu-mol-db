#include "pch.h"
#include "settings.h"



AppSettings::AppSettings()
    : _sidebarWidth(0.0f)
{
    _molecule.reset(Molecule::create(std::vector<Molecule::Atom>(), std::vector<Molecule::Link>()));
}

AppSettings::~AppSettings()
{
}

void AppSettings::setSidebarWidth(float width)
{
    _sidebarWidth = width;
    sidebarWidthChangedSignal(width);
}