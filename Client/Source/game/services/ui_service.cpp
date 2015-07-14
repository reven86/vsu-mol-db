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
    _form->getControl("bondLengthSlider")->addListener(this, gameplay::Control::Listener::VALUE_CHANGED);
    _form->getControl("saveButton")->addListener(this, gameplay::Control::Listener::CLICK);
    _form->getControl("saveCubeButton")->addListener(this, gameplay::Control::Listener::CLICK);
    _form->getControl("bboxExtent")->addListener(this, gameplay::Control::Listener::TEXT_CHANGED);

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
        || !strcmp(control->getId(), "bondLengthSlider")
        )
    {
        generateTube();
    }
    else if (!strcmp(control->getId(), "saveButton"))
    {
        onSaveButton();
    }
    else if (!strcmp(control->getId(), "bboxExtent"))
    {
        updateBBoxLabels();
    }
    else if (!strcmp(control->getId(), "saveCubeButton"))
    {
        onSaveCubeButton();
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
    float bondLength = static_cast<gameplay::Slider *>(_form->getControl("bondLengthSlider"))->getValue();

    TubeGenerator gen;
    gen.generateCarbonTube(a1, a2, h, transition, bondLength, atoms, links);

    getSettings()->getMolecule()->setup(atoms, links);
    updateBBoxLabels();
    updateTubePropertiesLabels();
}

void UIService::updateBBoxLabels()
{
    float extent = static_cast<float>(_wtof(static_cast<gameplay::TextBox *>(_form->getControl("bboxExtent"))->getText()));

    gameplay::BoundingBox bbox = getSettings()->getMolecule()->getBBox();
    bbox.min -= gameplay::Vector3(extent, extent, extent);
    bbox.max += gameplay::Vector3(extent, extent, extent);
    static_cast<gameplay::Label *>(_form->getControl("bboxMinLabel"))->setText(Utils::UTF8ToWCS(Utils::format("%.2f %.2f %.2f", bbox.min.x, bbox.min.y, bbox.min.z)));
    static_cast<gameplay::Label *>(_form->getControl("bboxMaxLabel"))->setText(Utils::UTF8ToWCS(Utils::format("%.2f %.2f %.2f", bbox.max.x, bbox.max.y, bbox.max.z)));
}

void UIService::updateTubePropertiesLabels()
{
    int n1 = static_cast<int>(static_cast<gameplay::Slider *>(_form->getControl("chiralityA1Slider"))->getValue());
    int n2 = static_cast<int>(static_cast<gameplay::Slider *>(_form->getControl("chiralityA2Slider"))->getValue());
    int k = static_cast<int>(static_cast<gameplay::Slider *>(_form->getControl("tubeHeightSlider"))->getValue());
    float transition = 0.01f * static_cast<gameplay::Slider *>(_form->getControl("tubeTransitionSlider"))->getValue();
    float bondLength = static_cast<gameplay::Slider *>(_form->getControl("bondLengthSlider"))->getValue();
    if (transition == 0.0f)
        transition = 1.0f;

    gameplay::Vector2 a1(1.7320508f, 0.0f);
    gameplay::Vector2 a2(-0.8660254f, 1.5f);

    gameplay::Vector2 R = (a1 * static_cast<float>(n1)+a2 * static_cast<float>(n2))* bondLength;
    gameplay::Vector2 L(-R.y, R.x);
    L.normalize();
    L.scale(k * bondLength);

    float radius = R.length() / MATH_PIX2 / transition;
    gameplay::BoundingBox bbox = getSettings()->getMolecule()->getBBox();
    float height = bbox.max.y - bbox.min.y;

    static_cast<gameplay::Label *>(_form->getControl("tubePropertiesLabel"))->setText(Utils::UTF8ToWCS(Utils::format(gameDictionary.lookupUTF8("UI_tube_properties").c_str(), radius, height)));
}

void UIService::onSaveButton()
{
    Molecule * mol = getSettings()->getMolecule();
    GP_ASSERT(mol);

    std::string fileName = gameplay::FileSystem::displayFileDialog(gameplay::FileSystem::SAVE, "Save file", "TXT files", "txt", NULL);
    if (fileName.empty())
        return;

    FILE * file = fopen(fileName.c_str(), "wt");
    if (!file)
        return;

    for (auto atom : mol->getAtoms())
    {
        GP_ASSERT(atom.index == 1);
        fprintf(file, "C %f %f %f\n", atom.pos.x, atom.pos.y, atom.pos.z);
    }

    fclose(file);
}

void UIService::onSaveCubeButton()
{
    Molecule * mol = getSettings()->getMolecule();
    GP_ASSERT(mol);

    std::string fileName = gameplay::FileSystem::displayFileDialog(gameplay::FileSystem::SAVE, "Save file", "TXT files", "txt", NULL);
    if (fileName.empty())
        return;

    FILE * file = fopen(fileName.c_str(), "wt");
    if (!file)
        return;

    float extent = static_cast<float>(_wtof(static_cast<gameplay::TextBox *>(_form->getControl("bboxExtent"))->getText()));

    gameplay::BoundingBox bbox = getSettings()->getMolecule()->getBBox();
    bbox.min -= gameplay::Vector3(extent, extent, extent);
    bbox.max += gameplay::Vector3(extent, extent, extent);

    int xcount = _wtoi(static_cast<gameplay::TextBox *>(_form->getControl("bboxXCount"))->getText());
    int ycount = _wtoi(static_cast<gameplay::TextBox *>(_form->getControl("bboxYCount"))->getText());
    int zcount = _wtoi(static_cast<gameplay::TextBox *>(_form->getControl("bboxZCount"))->getText());

    if (xcount <= 0)
        xcount = 100;
    if (ycount <= 0)
        ycount = 100;
    if (zcount <= 0)
        zcount = 100;

    for (auto atom : mol->getAtoms())
    {
        GP_ASSERT(atom.index == 1);
        fprintf(file, "C %f %f %f\n", atom.pos.x, atom.pos.y, atom.pos.z);
    }

    gameplay::Vector3 bboxSize = bbox.max - bbox.min;
    bboxSize.x /= xcount;
    bboxSize.y /= ycount;
    bboxSize.z /= zcount;

    fprintf(file, "\n");
    fprintf(file, "%s.cub\n", fileName.c_str());
    fprintf(file, "-1 %f %f %f\n", bbox.min.x, bbox.min.y, bbox.min.z);
    fprintf(file, "%d %f %f %f\n", xcount, bboxSize.x, 0.0f, 0.0f);
    fprintf(file, "%d %f %f %f\n", ycount, 0.0f, bboxSize.y, 0.0f);
    fprintf(file, "%d %f %f %f\n", zcount, 0.0f, 0.0f, bboxSize.z);

    fclose(file);
}