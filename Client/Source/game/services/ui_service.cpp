#include "pch.h"
#include "ui_service.h"
#include "services/service_manager.h"
#include "services/tracker_service.h"
#include "game/generators/tube_generator.h"
#include "main.h"



UIService::UIService(const ServiceManager * manager)
    : Service(manager)
    , _updateTime(0.0f)
    , _isTablet(false)
{
}

UIService::~UIService()
{
}

bool UIService::onInit()
{
    _isTablet = gameplay::Game::getInstance()->getHeight() >= 768;

    _form.reset(gameplay::Form::create("ui/main-tablet.form"));// _isTablet ? "ui/main-tablet.form" : "ui/main.form"));
    _updateTime = static_cast< float >(gameplay::Game::getInstance()->getGameTime());

    RenderService * rs = _manager->findService< RenderService >();

    _uiRenderStep.reset(rs->createRenderStep("UIRenderStep"));

    _uiRenderClick.reset(new RenderClick("UTRenderClick", std::bind(&UIService::renderUI, this)));
    _uiRenderStep->addRenderClick(_uiRenderClick.get());

    _form->getControl("sidebarToggleButton")->addListener(this, gameplay::Control::Listener::CLICK);
    _form->getControl("chiralityA1Slider")->addListener(this, gameplay::Control::Listener::VALUE_CHANGED);
    _form->getControl("chiralityA2Slider")->addListener(this, gameplay::Control::Listener::VALUE_CHANGED);
    _form->getControl("tubeHeightSlider")->addListener(this, gameplay::Control::Listener::VALUE_CHANGED);
    _form->getControl("tubeTransitionSlider")->addListener(this, gameplay::Control::Listener::VALUE_CHANGED);

    float scaleFactor = getUIScaleFactor();
    scaleUIControls(_form.get(), scaleFactor, scaleFactor);

    _form->update(0.0f);
    _form->update(0.0f);
    _form->update(0.0f);

    _trackerService = ServiceManager::getInstance()->findService< TrackerService >();

    getSettings()->setSidebarWidth(_form->getControl("sidebarControls")->getWidth());
    generateTube();

    return true;
}

bool UIService::onTick()
{
    float newTime = static_cast< float >(gameplay::Game::getInstance()->getGameTime());
    _form->update(newTime - _updateTime);
    _updateTime = newTime;

    return false;
}

void UIService::renderUI() const
{
    const_cast< gameplay::Form * >(_form.get())->draw();
}

void UIService::controlEvent(gameplay::Control* control, gameplay::Control::Listener::EventType evt)
{
    if (!strcmp(control->getId(), "sidebarToggleButton"))
    {
        if (_form->getControl("sidebarControls")->isVisible())
        {
            _form->getControl("sidebarControls")->setVisible(false);
            getSettings()->setSidebarWidth(0.0f);
            static_cast<gameplay::Button *>(control)->setText(L">>");
            control->setX(0.0f);
        }
        else
        {
            _form->getControl("sidebarControls")->setVisible(true);
            getSettings()->setSidebarWidth(_form->getControl("sidebarControls")->getWidth());
            static_cast<gameplay::Button *>(control)->setText(L"<<");
            control->setX(_form->getControl("sidebarControls")->getWidth());
        }
    }
    else if (!strcmp(control->getId(), "tubeHeightSlider")
        || !strcmp(control->getId(), "chiralityA1Slider")
        || !strcmp(control->getId(), "chiralityA2Slider")
        || !strcmp(control->getId(), "tubeTransitionSlider")
        )
    {
        generateTube();
    }
}

void UIService::hideControl(gameplay::Control * control)
{
    if (!control->isVisible() || control->getOpacity() == 0.0f)
        return;

    float from = control->getOpacity();
    float to = 0.0f;
    control->setEnabled(false);
    control->createAnimationFromTo("", gameplay::Control::ANIMATE_OPACITY, &from, &to, gameplay::Curve::LINEAR, 500)->play();
}

void UIService::unhideControl(gameplay::Control * control)
{
    if (control->isVisible() && control->getOpacity() == 1.0f)
        return;

    float from = control->getOpacity();
    float to = 1.0f;
    control->setVisible(true);
    control->setEnabled(true);
    control->createAnimationFromTo("", gameplay::Control::ANIMATE_OPACITY, &from, &to, gameplay::Curve::LINEAR, 500)->play();
}

void UIService::scaleUIControl(gameplay::Control * control, float kx, float ky)
{
    if (!control->isXPercentage())
        control->setX(control->getX() * kx);
    if (!control->isYPercentage())
        control->setY(control->getY() * ky);
    if (!control->isWidthPercentage() && (control->getAutoSize() & gameplay::Control::AUTO_SIZE_WIDTH) == 0)
        control->setWidth(control->getWidth() * kx);
    if (!control->isHeightPercentage() && (control->getAutoSize() & gameplay::Control::AUTO_SIZE_HEIGHT) == 0)
        control->setHeight(control->getHeight() * ky);

    const gameplay::Theme::Border& border = control->getBorder();
    control->setBorder(border.top * ky, border.bottom * ky, border.left * kx, border.right * kx);

    const gameplay::Theme::Margin& margin = control->getMargin();
    control->setMargin(margin.top * ky, margin.bottom * ky, margin.left * kx, margin.right * kx);

    const gameplay::Theme::Padding& padding = control->getPadding();
    control->setPadding(padding.top * ky, padding.bottom * ky, padding.left * kx, padding.right * kx);

    control->setFontSize(ky * control->getFontSize());

    //if( !strcmp( control->getType( ), "checkBox" ) )
    //{
    //    gameplay::CheckBox * checkbox = static_cast< gameplay::CheckBox * >( control );
    //    checkbox->setImageSize( checkbox->getImageSize( ).x * kx, checkbox->getImageSize( ).y * ky );
    //}

    if (!strcmp(control->getTypeName(), "Slider"))
        static_cast<gameplay::Slider *>(control)->setScaleFactor(0.5f * ky);

    if (!strcmp(control->getTypeName(), "Label")
        || !strcmp(control->getTypeName(), "Button")
        || !strcmp(control->getTypeName(), "TextBox")
        || !strcmp(control->getTypeName(), "CheckBox")
        || !strcmp(control->getTypeName(), "Slider")
        || !strcmp(control->getTypeName(), "RadioButton")
        )
    {
        gameplay::Label * label = static_cast< gameplay::Label * >(control);
        const wchar_t * text = label->getText();

        std::string key;
        std::copy(text, text + wcslen(text), std::back_inserter(key));

        if (gameDictionary.hasEntry(key.c_str()))
            label->setText(gameDictionary.lookup(key.c_str()).c_str());
    }

    if (!strcmp(control->getTypeName(), "ImageControl"))
    {
        gameplay::ImageControl * image = static_cast< gameplay::ImageControl * >(control);
        const gameplay::Rectangle& dstRegion = image->getRegionDst();
        image->setRegionDst(dstRegion.x * kx, dstRegion.y * ky, dstRegion.width * kx, dstRegion.height * ky);
    }
}

void UIService::scaleUIControls(gameplay::Container * container, float kx, float ky)
{
    if (!container)
        return;

    scaleUIControl(container, kx, ky);

    const std::vector< gameplay::Control * >& controls = container->getControls();
    for (unsigned j = 0; j < controls.size(); j++)
    {
        const char * type = controls[j]->getTypeName();
        if (!strcmp(type, "Container"))
            scaleUIControls(static_cast< gameplay::Container * >(controls[j]), kx, ky);
        else
            scaleUIControl(controls[j], kx, ky);
    }
}

float UIService::getUIScaleFactor() const
{
    return static_cast<float>(gameplay::Game::getInstance()->getHeight()) / 768.0f;// (_isTablet ? 768.0f : 320.0f);
}

void UIService::generateTube()
{
    std::vector<Molecule::Atom> atoms;
    std::vector<Molecule::Link> links;

    int a1 = static_cast<int>(static_cast<gameplay::Slider *>(_form->getControl("chiralityA1Slider"))->getValue());
    int a2 = static_cast<int>(static_cast<gameplay::Slider *>(_form->getControl("chiralityA2Slider"))->getValue());
    int h = static_cast<int>(static_cast<gameplay::Slider *>(_form->getControl("tubeHeightSlider"))->getValue());
    float transition = 0.01f * static_cast<gameplay::Slider *>(_form->getControl("tubeTransitionSlider"))->getValue();

    TubeGenerator gen;
    gen.generateCarbonTube(a1, a2, h, transition, atoms, links);

    getSettings()->getMolecule()->setup(atoms, links);
}