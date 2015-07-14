#pragma once

#ifndef __UI_SERVICE_H__
#define __UI_SERVICE_H__

#include "services/service.h"
#include "services/render_service.h"




class UIService : public Service, public gameplay::Control::Listener
{
    RefPtr< gameplay::Form > _form;
    float _updateTime;

    bool _isTablet;

    std::unique_ptr< RenderStep > _uiRenderStep;
    std::unique_ptr< RenderClick > _uiRenderClick;
    class TrackerService * _trackerService;

public:
    UIService(const ServiceManager * manager);
    virtual ~UIService();

    static const char * getTypeName() { return "UIService"; }

    virtual bool onInit();
    virtual bool onTick();

private:
    void renderUI() const;
    void hideControl(gameplay::Control * control);
    void unhideControl(gameplay::Control * control);
    void scaleUIControl(gameplay::Control * control, float kx, float ky);
    void scaleUIControls(gameplay::Container * container, float kx, float ky);

    virtual void controlEvent(gameplay::Control* control, gameplay::Control::Listener::EventType evt);

    float getUIScaleFactor() const;

    void generateTube();

    void onSaveButton();
    void onSaveCubeButton();
    void updateBBoxLabels();
    void updateTubePropertiesLabels();
};



#endif