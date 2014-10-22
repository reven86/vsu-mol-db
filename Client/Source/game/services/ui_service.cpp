#include "pch.h"
#include "ui_service.h"
#include "services/service_manager.h"
#include "services/tracker_service.h"
#include "render/mem_fn_render_click.h"
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

bool UIService::OnInit()
{
    _isTablet = gameplay::Game::getInstance()->getHeight() >= 768;

    _form.reset(gameplay::Form::create("ui/main-tablet.form"));// _isTablet ? "ui/main-tablet.form" : "ui/main.form"));
    _updateTime = static_cast< float >(gameplay::Game::getInstance()->getGameTime());

    RenderService * rs = _manager->FindService< RenderService >();

    _uiRenderStep.reset(rs->CreateRenderStep("UIRenderStep"));

    RefPtr< RenderClick > renderClick;
    renderClick.reset(new MemFnRenderClick< UIService >(this, &UIService::renderUI, "UIRenderClick"));
    _uiRenderStep->AddRenderClick(renderClick);

    float scaleFactor = getUIScaleFactor();
    scaleUIControls(_form.get(), scaleFactor, scaleFactor);

    _form->update(0.0f);
    _form->update(0.0f);
    _form->update(0.0f);

    _trackerService = ServiceManager::Instance().FindService< TrackerService >();

    //getViewSettings()->setSidebarWidth(_form->getControl("sidebarControls")->getWidth());

    return true;
}

bool UIService::OnTick()
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

    if (!strcmp(control->getType(), "slider"))
        static_cast<gameplay::Slider *>(control)->setScaleFactor(0.5f * ky);

    if (!strcmp(control->getType(), "label")
        || !strcmp(control->getType(), "button")
        || !strcmp(control->getType(), "textbox")
        || !strcmp(control->getType(), "checkBox")
        || !strcmp(control->getType(), "slider")
        || !strcmp(control->getType(), "radioButton")
        )
    {
        gameplay::Label * label = static_cast< gameplay::Label * >(control);
        const wchar_t * text = label->getText();

        std::string key;
        std::copy(text, text + wcslen(text), std::back_inserter(key));

        if (gameDictionary.HasEntry(key.c_str()))
            label->setText(gameDictionary.Lookup(key.c_str()).c_str());
    }

    if (!strcmp(control->getType(), "image"))
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
        const char * type = controls[j]->getType();
        if (!strcmp(type, "container"))
            scaleUIControls(static_cast< gameplay::Container * >(controls[j]), kx, ky);
        else
            scaleUIControl(controls[j], kx, ky);
    }
}

float UIService::getUIScaleFactor() const
{
    return static_cast<float>(gameplay::Game::getInstance()->getHeight()) / 768.0f;// (_isTablet ? 768.0f : 320.0f);
}