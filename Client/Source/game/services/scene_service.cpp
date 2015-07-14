#include "pch.h"
#include "scene_service.h"
#include "services/render_service.h"
#include "services/service_manager.h"
#include "game/primitives.h"
#include "game/molecule.h"
#include "game/molecule_model.h"



SceneService::SceneService(const ServiceManager * manager)
    : Service(manager)
    , _cameraPhi(0.0f)
    , _cameraTheta(0.0f)
    , _touchStart(0.0f, 0.0f)
    , _cameraDistance(20.0f)
    , _dragDetected(false)
    , _lastPinchScale(1.0f)
{
}

SceneService::~SceneService()
{
}

bool SceneService::onInit()
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

    ServiceManager::getInstance()->signals.inputTouchEvent.connect(10, sigc::mem_fun(this, &SceneService::onTouchEvent));
    ServiceManager::getInstance()->signals.inputMouseEvent.connect(10, sigc::mem_fun(this, &SceneService::onMouseEvent));
    ServiceManager::getInstance()->signals.inputGesturePinchEvent.connect(10, sigc::mem_fun(this, &SceneService::onGesturePinchEvent));

    getSettings()->sidebarWidthChangedSignal.connect(sigc::mem_fun(this, &SceneService::onSidebarWidthChanged));

    _renderService = _manager->findService<RenderService>();

    gameplay::Game::getInstance()->registerGesture(gameplay::Gesture::GESTURE_PINCH);

    PrimitivesPool::getInstance()->generatePrimitives();

    _moleculeModel.reset(MoleculeModel::create(getSettings()->getMolecule()));
    insertNode(_moleculeModel->getRootNode());
    _moleculeModel->modelChangedSignal.connect(sigc::mem_fun(this, &SceneService::moleculeChanged));

    updateCamera();
    _lastTime = static_cast< float >(gameplay::Game::getInstance()->getGameTime() * 0.001f);

    float vpwidth = static_cast<float>(gameplay::Game::getInstance()->getWidth());
    float vpheight = static_cast<float>(gameplay::Game::getInstance()->getHeight());
    float sidebarWidth = getSettings()->getSidebarWidth();
    _camera->setAspectRatio((vpwidth - sidebarWidth) / vpheight);

    gameplay::Vector3 axisX[] = { gameplay::Vector3::zero(), gameplay::Vector3::unitX() };
    gameplay::Vector3 axisY[] = { gameplay::Vector3::zero(), gameplay::Vector3::unitY() };
    gameplay::Vector3 axisZ[] = { gameplay::Vector3::zero(), gameplay::Vector3::unitZ() };
    gameplay::Mesh * axisXMesh = gameplay::Mesh::createLines(axisX, 2);
    gameplay::Mesh * axisYMesh = gameplay::Mesh::createLines(axisY, 2);
    gameplay::Mesh * axisZMesh = gameplay::Mesh::createLines(axisZ, 2);
    _axisXModel.reset(gameplay::Model::create(axisXMesh));
    _axisYModel.reset(gameplay::Model::create(axisYMesh));
    _axisZModel.reset(gameplay::Model::create(axisZMesh));
    _axisXModel->setMaterial("materials/models/line.material");
    _axisYModel->setMaterial("materials/models/line.material");
    _axisZModel->setMaterial("materials/models/line.material");
    _axisXModel->getMaterial()->getParameter("u_modulateColor")->setVector4(gameplay::Vector4(1.0f, 0.0f, 0.0f, 1.0f));
    _axisYModel->getMaterial()->getParameter("u_modulateColor")->setVector4(gameplay::Vector4(0.0f, 1.0f, 0.0f, 1.0f));
    _axisZModel->getMaterial()->getParameter("u_modulateColor")->setVector4(gameplay::Vector4(0.0f, 0.0f, 1.0f, 1.0f));
    SAFE_RELEASE(axisXMesh);
    SAFE_RELEASE(axisYMesh);
    SAFE_RELEASE(axisZMesh);

    gameplay::Node * axisXNode = _scene->addNode("AxisX");
    gameplay::Node * axisYNode = _scene->addNode("AxisY");
    gameplay::Node * axisZNode = _scene->addNode("AxisZ");
    axisXNode->setDrawable(_axisXModel.get());
    axisYNode->setDrawable(_axisYModel.get());
    axisZNode->setDrawable(_axisZModel.get());

    return true;
}

bool SceneService::onTick()
{
    return false;
}

void SceneService::render()
{
    if (getState() != SceneService::RUNNING)
        return;

    float vpwidth = static_cast<float>(gameplay::Game::getInstance()->getWidth());
    float vpheight = static_cast<float>(gameplay::Game::getInstance()->getHeight());
    float sidebarWidth = getSettings()->getSidebarWidth();

    gameplay::Rectangle prevViewport = gameplay::Game::getInstance()->getViewport();
    gameplay::Game::getInstance()->setViewport(gameplay::Rectangle(sidebarWidth, 0.0f, vpwidth - sidebarWidth, vpheight));

    _scene->visit(this, &SceneService::drawNode);

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
        float sidebarWidth = getSettings()->getSidebarWidth();

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
            _cameraPhi = fmodf(_cameraPhi - MATH_PI * delta.x / gameplay::Game::getInstance()->getViewport().width, MATH_PIX2);
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
        float fov = std::min(90.0f, std::max(10.0f, _camera->getFieldOfView() * powf(0.9f, static_cast< float >(wheelDelta))));
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
    _camera->getNode()->set(gameplay::Vector3::one(), rotation, translate);
}

bool SceneService::onGesturePinchEvent(int x, int y, float scale, int numberOfTouches)
{
    if (numberOfTouches > 0)
    {
        float fov = std::min(90.0f, std::max(10.0f, _camera->getFieldOfView() * _lastPinchScale / scale));
        _camera->setFieldOfView(fov);
    }

    _lastPinchScale = scale;

    return true;
}

void SceneService::insertNode(gameplay::Node * node)
{
    if (node->getScene() != _scene)
        _scene->addNode(node);
    _scene->visit(this, &SceneService::bindUniforms);
}

bool SceneService::bindUniforms(gameplay::Node * node)
{
    if (node->getDrawable())
    {
        gameplay::Effect * effect = static_cast<gameplay::Model *>(node->getDrawable())->getMaterial()->getTechnique()->getPassByIndex(0)->getEffect();

        if (effect->getUniform("u_directionalLightColor[0]"))
            static_cast<gameplay::Model *>(node->getDrawable())->getMaterial()->getParameter("u_directionalLightColor[0]")->setValue(_light->getColor());
        if (effect->getUniform("u_directionalLightDirection[0]"))
            static_cast<gameplay::Model *>(node->getDrawable())->getMaterial()->getParameter("u_directionalLightDirection[0]")->bindValue(_camera->getNode(), &gameplay::Node::getForwardVectorView);
    }

    return true;
}

bool SceneService::drawNode(gameplay::Node * node)
{
    if (node->getDrawable())
        node->getDrawable()->draw();

    return true;
}

void SceneService::onSidebarWidthChanged(float width)
{
    float vpwidth = static_cast<float>(gameplay::Game::getInstance()->getWidth());
    float vpheight = static_cast<float>(gameplay::Game::getInstance()->getHeight());

    _camera->setAspectRatio((vpwidth - width) / vpheight);
}

void SceneService::moleculeChanged()
{
    insertNode(_moleculeModel->getRootNode());
    const gameplay::Vector3& translation = _moleculeModel->getRootNode()->getTranslationWorld();
    _axisXModel->getNode()->setTranslation(translation);
    _axisYModel->getNode()->setTranslation(translation);
    _axisZModel->getNode()->setTranslation(translation);
}