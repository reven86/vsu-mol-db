#include "pch.h"
#include "scene_service.h"
#include "services/render_service.h"
#include "services/service_manager.h"



SceneService::SceneService(const ServiceManager * manager)
    : Service(manager)
    , _cameraPhi(0.0f)
    , _cameraTheta(0.0f)
    , _touchStart(0.0f, 0.0f)
    , _cameraDistance(15.0f)
    , _dragDetected(false)
    , _lastPinchScale(1.0f)
{
}

SceneService::~SceneService()
{
}

bool SceneService::OnInit()
{
    _scene.reset(gameplay::Scene::create());
    _scene->setAmbientColor(0.1f, 0.1f, 0.1f);

    _camera.reset(gameplay::Camera::createPerspective(30.0f, gameplay::Game::getInstance()->getAspectRatio(), 5.0f, 30.0f));
    _scene->setActiveCamera(_camera.get());

    _light.reset(gameplay::Light::createDirectional(0.9f, 0.9f, 0.9f));

    gameplay::Node * cameraNode = _scene->addNode("Camera");
    cameraNode->setCamera(_camera.get());

    gameplay::Node * lightNode = _scene->addNode("Light");
    lightNode->setLight(_light);
    lightNode->setRotation(gameplay::Vector3(1.0f, 0.0f, 0.0f), -MATH_PIOVER4);

    ServiceManager::Instance().signals.inputTouchEvent.connect(10, sigc::mem_fun(this, &SceneService::onTouchEvent));
    ServiceManager::Instance().signals.inputMouseEvent.connect(10, sigc::mem_fun(this, &SceneService::onMouseEvent));
    ServiceManager::Instance().signals.inputGesturePinchEvent.connect(10, sigc::mem_fun(this, &SceneService::onGesturePinchEvent));

    _renderService = _manager->FindService<RenderService>();

    gameplay::Game::getInstance()->registerGesture(gameplay::Gesture::GESTURE_PINCH);

    updateCamera();
    _lastTime = static_cast< float >(gameplay::Game::getInstance()->getGameTime() * 0.001f);

    float vpwidth = static_cast<float>(gameplay::Game::getInstance()->getWidth());
    float vpheight = static_cast<float>(gameplay::Game::getInstance()->getHeight());
    float sidebarWidth = 0.0f;// getViewSettings()->getSidebarWidth();
    _camera->setAspectRatio((vpwidth - sidebarWidth) / vpheight);

    return true;
}

bool SceneService::OnTick()
{
    float time = static_cast< float >(gameplay::Game::getInstance()->getGameTime() * 0.001f);
    float dt = time - _lastTime;
    _lastTime = time;

    _scene->update(dt * 1000.0f);

    return false;
}

void SceneService::render()
{
    float vpwidth = static_cast<float>(gameplay::Game::getInstance()->getWidth());
    float vpheight = static_cast<float>(gameplay::Game::getInstance()->getHeight());
    float sidebarWidth = 0.0f;// getViewSettings()->getSidebarWidth();

    gameplay::Rectangle prevViewport = gameplay::Game::getInstance()->getViewport();
    gameplay::Game::getInstance()->setViewport(gameplay::Rectangle(sidebarWidth, 0.0f, vpwidth - sidebarWidth, vpheight));

    gameplay::Game::getInstance()->setViewport(prevViewport);
}

bool SceneService::onTouchEvent(const gameplay::Touch::TouchEvent& ev, int x, int y, unsigned contactIndex)
{
    if (contactIndex != 0)
        return false;

    if (ev == gameplay::Touch::TOUCH_PRESS)
    {
        _touchStart.set(static_cast<float>(x), static_cast<float>(y));
        _dragDetected = true;
        return true;
    }

    if (ev == gameplay::Touch::TOUCH_RELEASE && !_dragDetected)
    {
        float vpwidth = static_cast<float>(gameplay::Game::getInstance()->getWidth());
        float vpheight = static_cast<float>(gameplay::Game::getInstance()->getHeight());
        float sidebarWidth = 0.0;// getViewSettings()->getSidebarWidth();

        gameplay::Ray ray;
        _camera->pickRay(gameplay::Rectangle(sidebarWidth, 0.0f, vpwidth - sidebarWidth, vpheight), _touchStart.x, _touchStart.y, &ray);
        return true;
    }

    if (ev == gameplay::Touch::TOUCH_MOVE)
    {
        gameplay::Vector2 touchPos(static_cast<float>(x), static_cast<float>(y));
        gameplay::Vector2 delta(touchPos - _touchStart);

        if (_dragDetected)
        {
            _touchStart = touchPos;

            _cameraTheta = std::min(MATH_PIOVER2 * 0.99f, std::max(-MATH_PIOVER2 * 0.99f, _cameraTheta + MATH_PI * delta.y / gameplay::Game::getInstance()->getViewport().height));
            _cameraPhi = fmodf(_cameraPhi + MATH_PI * delta.x / gameplay::Game::getInstance()->getViewport().width, MATH_PIX2);
            updateCamera();
        }
        else if (delta.length() > gameplay::Game::getInstance()->getViewport().height * 0.01f)
        {
            _dragDetected = true;
        }

        return true;
    }

    return false;
}

bool SceneService::onMouseEvent(const gameplay::Mouse::MouseEvent& ev, int x, int y, int wheelDelta)
{
    if (wheelDelta != 0)
    {
        //_cameraDistance = std::min(15.0f, std::max(3.0f, _cameraDistance * pow(0.9f, wheelDelta)));
        //updateCamera();
        float fov = std::min(60.0f, std::max(10.0f, _camera->getFieldOfView() * powf(0.9f, static_cast< float >(wheelDelta))));
        _camera->setFieldOfView(fov);
        return true;
    }

    return false;
}

void SceneService::updateCamera()
{
    gameplay::Vector3 targetPoint(0.0f, 0.0f, 0.0f);

    //if (_vehicleCollision)
    //    targetPoint = _vehicleCollision->getBBox().getCenter();

    gameplay::Vector3 translate(sinf(_cameraPhi) * cosf(_cameraTheta), sinf(_cameraTheta), cosf(_cameraPhi) * cosf(_cameraTheta));
    translate *= _cameraDistance;
    translate += targetPoint;

    gameplay::Matrix transform;
    gameplay::Matrix::createLookAt(translate, targetPoint, gameplay::Vector3::unitY(), &transform);

    gameplay::Quaternion rotation;
    transform.decompose(NULL, &rotation, NULL);
    _camera->getNode()->set(gameplay::Vector3(-1.0f, 1.0f, 1.0f), rotation, translate);
}

bool SceneService::onGesturePinchEvent(int x, int y, float scale, int numberOfTouches)
{
    if (numberOfTouches > 0)
    {
        float fov = std::min(60.0f, std::max(10.0f, _camera->getFieldOfView() * _lastPinchScale / scale));
        _camera->setFieldOfView(fov);
    }

    _lastPinchScale = scale;

    return true;
}