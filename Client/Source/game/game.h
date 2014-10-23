#ifndef __WOT_TANK_EXPLORER_H__
#define __WOT_TANK_EXPLORER_H__

#include "main.h"




/*!	\brief Main game class.
 */

class VSUDatabale : public DfgGame
{
    class SceneService * _sceneService;

    std::string _clientUUID;
    Settings _settings;

public:
    VSUDatabale();
    virtual ~VSUDatabale();

    Settings * getSettings() { return &_settings; };

protected:
    void initialize();
    void finalize();
    void render(float elapsedTime);

    void saveSettings();
    void loadSettings();
};




#endif // __WOT_TANK_EXPLORER_H__