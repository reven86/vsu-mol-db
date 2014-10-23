#pragma once

#ifndef __SCENE_SERVICE_H__
#define __SCENE_SERVICE_H__

#include "services/service.h"
#include "render/particle_system.h"




class SceneService : public Service
{
    RefPtr< gameplay::Scene > _scene;
    RefPtr< gameplay::Camera > _camera;
    RefPtr< gameplay::Light > _light;

    float _cameraPhi;
    float _cameraTheta;
    float _cameraDistance;
    gameplay::Vector2 _touchStart;
    bool _dragDetected;
    float _lastPinchScale;

    class RenderService * _renderService;
    float _lastTime;

    RefPtr< class MoleculeModel > _moleculeModel;

public:
    SceneService(const ServiceManager * manager);
    virtual ~SceneService();

    static const char * GetName() { return "SceneService"; }

    virtual bool OnInit();
    virtual bool OnTick();

    void render();

    void insertNode(gameplay::Node * node);

private:
    bool onTouchEvent(const gameplay::Touch::TouchEvent& ev, int x, int y, unsigned contactIndex);
    bool onMouseEvent(const gameplay::Mouse::MouseEvent& ev, int x, int y, int wheelDelta);
    bool onGesturePinchEvent(int x, int y, float scale, int numberOfTouches);
    void onSidebarWidthChanged(float width);

    void updateCamera();

    bool bindUniforms(gameplay::Node * node);
    bool drawNode(gameplay::Node * node);
};



#endif // __SCENE_H__