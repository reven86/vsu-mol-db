#pragma once

#ifndef __SETTINGS_H__
#define __SETTINGS_H__

#include "molecule.h"



class Settings
{
    RefPtr< Molecule > _molecule;
    float _sidebarWidth;

public:
    sigc::signal<void, float> sidebarWidthChangedSignal;

    Settings();
    virtual ~Settings();

    Molecule * getMolecule() { return _molecule.get(); };
    float getSidebarWidth() const { return _sidebarWidth; };

    void setSidebarWidth(float width);
};



#endif // __SETTINGS_H__


