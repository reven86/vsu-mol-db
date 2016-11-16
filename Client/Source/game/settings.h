#pragma once

#ifndef __SETTINGS_H__
#define __SETTINGS_H__

#include "molecule.h"



class AppSettings
{
    RefPtr< Molecule > _molecule;
    float _sidebarWidth;

public:
    sigc::signal<void, float> sidebarWidthChangedSignal;

	AppSettings();
    virtual ~AppSettings();

    Molecule * getMolecule() { return _molecule.get(); };
    float getSidebarWidth() const { return _sidebarWidth; };

    void setSidebarWidth(float width);
};



#endif // __SETTINGS_H__


