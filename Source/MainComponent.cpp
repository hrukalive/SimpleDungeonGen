#include "MainComponent.h"

//==============================================================================
template<typename FunctionType>
inline void visitEach(std::initializer_list<juce::Component*> comps, FunctionType&& fn)
{
    std::for_each(std::begin(comps), std::end(comps), fn);
}

MainComponent::CanvasOverlayComponent::CanvasOverlayComponent(MainComponent* parent, juce::ValueTree& state)
{
	setInterceptsMouseClicks(true, false);
	setOpaque(false);
	setBounds(parent->getLocalBounds());
    this->parent = parent;
    this->generalState = state.getChildWithName(juce::Identifier("General"));
    this->randGenState = state.getChildWithName(juce::Identifier("Random Box Generation"));
}

void MainComponent::CanvasOverlayComponent::paint(juce::Graphics& g)
{
    static const float dashes[2] = { 2.0f, 4.0f };
    static const float dashes2[2] = { 5.0f, 7.0f };
    int mapWidth = generalState.getProperty(juce::Identifier("mapWidth"), 64);
    int mapHeight = generalState.getProperty(juce::Identifier("mapHeight"), 64);
    bool useRectRegion = randGenState.getProperty(juce::Identifier("useRectRegion"), false);
    float radiusX = randGenState.getProperty(juce::Identifier("radiusX"), 8.0f);
    float radiusY = randGenState.getProperty(juce::Identifier("radiusY"), 8.0f);
	
    auto b2 = getLocalBounds();
    if (!generated)
    {
        g.setColour(juce::Colours::grey);
        for (int i = -mapWidth / 2; i < mapWidth / 2 + 1; i++)
        {
            g.drawLine(juce::Line<float>(
                b2.getCentreX() + i * scale + viewDx,
                -mapHeight / 2 * scale + b2.getCentreY() + viewDy,
                b2.getCentreX() + i * scale + viewDx,
                mapHeight / 2 * scale + b2.getCentreY() + viewDy));
        }
        for (int i = -mapHeight / 2; i < mapHeight / 2 + 1; i++)
        {
            g.drawLine(juce::Line<float>(
                -mapWidth / 2 * scale + b2.getCentreX() + viewDx,
                b2.getCentreY() + i * scale + viewDy,
                mapWidth / 2 * scale + b2.getCentreX() + viewDx,
                b2.getCentreY() + i * scale + viewDy));
        }
        g.setColour(juce::Colours::black);
        g.drawLine(juce::Line<float>(
            b2.getCentreX() + viewDx,
            -mapHeight / 2 * scale + b2.getCentreY() + viewDy,
            b2.getCentreX() + viewDx,
            mapHeight / 2 * scale + b2.getCentreY() + viewDy), 2.f);
        g.drawLine(juce::Line<float>(
            -mapWidth / 2 * scale + b2.getCentreX() + viewDx,
            b2.getCentreY() + viewDy,
            mapWidth / 2 * scale + b2.getCentreX() + viewDx,
            b2.getCentreY() + viewDy), 2.f);
		
        g.setColour(juce::Colours::white);
        auto genRegion = juce::Rectangle<float>(
            -radiusX * scale + b2.getCentreX() + viewDx,
            -radiusY * scale + b2.getCentreY() + viewDy,
            radiusX * 2 * scale,
            radiusY * 2 * scale
            );
        juce::Path p;
        if (useRectRegion)
            p.addRectangle(genRegion);
        else
            p.addEllipse(genRegion);
        juce::PathStrokeType pStroke(1.0);
        pStroke.createDashedStroke(p, p, dashes, 2);
        g.strokePath(p, pStroke);
    }

    int idx = 0;
    for (const auto& box : boxes)
    {
        auto rect = juce::Rectangle<float>(
            box.x * scale + b2.getCentreX() + viewDx,
            box.y * scale + b2.getCentreY() + viewDy,
            box.w * scale, box.h * scale);

        g.setColour(juce::Colours::lightgreen.withAlpha(0.5f));
        g.fillRect(rect);
        g.setColour(juce::Colours::orange.withAlpha(0.5f));
        g.drawRect(rect, 3.f);

        if (scale > 4)
        {
            g.setColour(juce::Colours::red);
            g.fillEllipse(juce::Rectangle<float>(
                box.cx * scale + b2.getCentreX() - 4 + viewDx,
                box.cy * scale + b2.getCentreY() - 4 + viewDy, 8, 8));
        }

        juce::AttributedString label;
        label.setJustification(juce::Justification::centred);
        label.append(juce::String(idx), juce::Font(18), juce::Colours::black);
        label.append("  w,h=" + juce::String(box.w) + ", " + juce::String(box.h), juce::Font(9), juce::Colours::black);
        label.append("  cx,cy=" + juce::String(box.cx) + ", " + juce::String(box.cy), juce::Font(9), juce::Colours::black);
        juce::TextLayout layout;
        layout.createLayout(label, rect.getWidth(), rect.getHeight());
        layout.draw(g, rect);
        idx++;
    }
    idx = 0;
    for (const auto& box : rooms)
    {
        auto rect = juce::Rectangle<float>(
            box.x * scale + b2.getCentreX() + viewDx,
            box.y * scale + b2.getCentreY() + viewDy,
            box.w * scale, box.h * scale);

        g.setColour(juce::Colours::skyblue.withAlpha(0.8f));
        g.fillRect(rect);
        g.setColour(juce::Colours::orange.withAlpha(0.5f));
        g.drawRect(rect, 3.f);

        if (scale > 4)
        {
            g.setColour(juce::Colours::red);
            g.fillEllipse(juce::Rectangle<float>(
                box.cx * scale + b2.getCentreX() - 4 + viewDx,
                box.cy * scale + b2.getCentreY() - 4 + viewDy, 8, 8));
        }

        juce::AttributedString label;
        label.setJustification(juce::Justification::centred);
        label.append(juce::String(idx), juce::Font(18), juce::Colours::black);
        label.append("  w,h=" + juce::String(box.w) + ", " + juce::String(box.h), juce::Font(9), juce::Colours::black);
        label.append("  cx,cy=" + juce::String(box.cx) + ", " + juce::String(box.cy), juce::Font(9), juce::Colours::black);
        juce::TextLayout layout;
        layout.createLayout(label, rect.getWidth(), rect.getHeight());
        layout.draw(g, rect);
        idx++;
    }
    idx = 0;
    for (const auto& box : corridors)
    {
        auto rect = juce::Rectangle<float>(
            box.x * scale + b2.getCentreX() + viewDx,
            box.y * scale + b2.getCentreY() + viewDy,
            box.w * scale, box.h * scale);

        g.setColour(juce::Colours::green.withAlpha(0.7f));
        g.fillRect(rect);
        g.setColour(juce::Colours::orange.withAlpha(0.5f));
        g.drawRect(rect, 3.f);

        if (scale > 4)
        {
            g.setColour(juce::Colours::red);
            g.fillEllipse(juce::Rectangle<float>(
                box.cx * scale + b2.getCentreX() - 4 + viewDx,
                box.cy * scale + b2.getCentreY() - 4 + viewDy, 8, 8));
        }

        juce::AttributedString label;
        label.setJustification(juce::Justification::centred);
        label.append(juce::String(idx), juce::Font(18), juce::Colours::black);
        label.append("  w,h=" + juce::String(box.w) + ", " + juce::String(box.h), juce::Font(9), juce::Colours::black);
        label.append("  cx,cy=" + juce::String(box.cx) + ", " + juce::String(box.cy), juce::Font(9), juce::Colours::black);
        juce::TextLayout layout;
        layout.createLayout(label, rect.getWidth(), rect.getHeight());
        layout.draw(g, rect);
        idx++;
    }
    for (const auto& e : edges)
    {
        g.setColour(juce::Colours::blue);
        double x1 = rooms[std::get<0>(e)].cx;
        double y1 = rooms[std::get<0>(e)].cy;
        double x2 = rooms[std::get<1>(e)].cx;
        double y2 = rooms[std::get<1>(e)].cy;
        g.drawDashedLine(juce::Line<float>(
            x1 * scale + b2.getCentreX() + viewDx,
            y1 * scale + b2.getCentreY() + viewDy,
            x2 * scale + b2.getCentreX() + viewDx,
            y2 * scale + b2.getCentreY() + viewDy), dashes, 2, 2.f);
    }
    for (const auto& e : mst_edges)
    {
        g.setColour(juce::Colours::peachpuff);
        double x1 = rooms[e.first].cx;
        double y1 = rooms[e.first].cy;
        double x2 = rooms[e.second].cx;
        double y2 = rooms[e.second].cy;
        g.drawDashedLine(juce::Line<float>(
            x1 * scale + b2.getCentreX() + viewDx,
            y1 * scale + b2.getCentreY() + viewDy,
            x2 * scale + b2.getCentreX() + viewDx,
            y2 * scale + b2.getCentreY() + viewDy), dashes2, 2, 4.f);
    }
    for (const auto& e : lines)
    {
        g.setColour(juce::Colours::purple.brighter());
        double x1, y1, x2, y2;
        std::tie(x1, y1, x2, y2) = e;
        g.drawLine(
            x1 * scale + b2.getCentreX() + viewDx,
            y1 * scale + b2.getCentreY() + viewDy,
            x2 * scale + b2.getCentreX() + viewDx,
            y2 * scale + b2.getCentreY() + viewDy, 3.f);
    }

    if (generated)
    {
        g.setColour(juce::Colours::black.withAlpha(0.6f));
        if (scale > 4)
            g.setColour(juce::Colours::white.withAlpha(0.6f));
        g.fillRect(
            -mapWidth / 2 * scale + b2.getCentreX() + viewDx,
            -mapHeight / 2 * scale + b2.getCentreY() + viewDy,
            mapWidth * scale, mapHeight * scale);
        for (int i = -mapHeight / 2; i < mapHeight / 2; i++)
        {
            for (int j = -mapWidth / 2; j < mapWidth / 2; j++)
            {
                auto type = tiles[(i + mapHeight / 2) * mapWidth + j + mapWidth / 2];
                g.setColour(juce::Colours::black);
                juce::Rectangle<float> cell(j * scale + b2.getCentreX() + viewDx, i * scale + b2.getCentreY() + viewDy, scale, scale);
                if (type > 0)
                {
                    if (!tileColor || type == 1)
                        g.setColour(juce::Colours::lightcyan);
                    else if (type == 2)
                        g.setColour(juce::Colours::lightgreen);
                    else if (type == 3)
                        g.setColour(juce::Colours::skyblue);
                    g.fillRect(cell);
                }
                if (scale > 4)
                {
                    g.drawRect(cell, 0.5f);
                    g.setColour(juce::Colours::black);
                    g.setFont(scale);
                    g.drawText(juce::String(tiles[(i + mapHeight / 2) * mapWidth + j + mapWidth / 2]), cell, juce::Justification::centred);
                }
            }
        }
    }
}

void MainComponent::CanvasOverlayComponent::resized()
{
    repaint();
}


void MainComponent::CanvasOverlayComponent::mouseDown(const juce::MouseEvent& e)
{
    prevMouseX = 0;
    prevMouseY = 0;
}

void MainComponent::CanvasOverlayComponent::mouseDrag(const juce::MouseEvent& e)
{
    auto pos = e.getOffsetFromDragStart();
    viewDx += pos.getX() - prevMouseX;
    viewDy += pos.getY() - prevMouseY;
	prevMouseX = pos.getX();
	prevMouseY = pos.getY();
    repaint();
}
void MainComponent::CanvasOverlayComponent::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
{
    fScale += wheel.deltaY * 5;
	if (fScale < 1)
		fScale = 1;
	if (fScale > 32)
		fScale = 32;
    if ((int)round(fScale) != scale)
    {
        viewDx += (e.getMouseDownX() - getWidth() / 2 - viewDx) * (1.0 - round(fScale) / (double)scale);
        viewDy += (e.getMouseDownY() - getHeight() / 2 - viewDy) * (1.0 - round(fScale) / (double)scale);
    }
    scale = round(fScale);
	repaint();
}

MainComponent::CustomSliderPropertyComponent::CustomSliderPropertyComponent(const juce::Value& valueToControl,
            const juce::String& propertyName,
            double rangeMin,
            double rangeMax,
            double interval,
            double skewFactor,
            bool symmetricSkew)
    : juce::SliderPropertyComponent(valueToControl, propertyName, rangeMin, rangeMax, interval, skewFactor, symmetricSkew)
{
    slider.setSliderStyle(juce::Slider::LinearHorizontal);
}


MainComponent::MainComponent()
{
    juce::ValueTree basicTree(juce::Identifier("General"));
    basicTree.setProperty(juce::Identifier("seed"), 0, nullptr);
    basicTree.setProperty(juce::Identifier("maxIteration"), 1000 * 100, nullptr);
    basicTree.setProperty(juce::Identifier("mapWidth"), 64, nullptr);
    basicTree.setProperty(juce::Identifier("mapHeight"), 64, nullptr);
    state.appendChild(basicTree, nullptr);

    juce::ValueTree randBoxTree(juce::Identifier("Random Box Generation"));
    randBoxTree.setProperty(juce::Identifier("useRectRegion"), false, nullptr);
    randBoxTree.setProperty(juce::Identifier("radiusX"), 8.0f, nullptr);
    randBoxTree.setProperty(juce::Identifier("radiusY"), 8.0f, nullptr);
    randBoxTree.setProperty(juce::Identifier("numBox"), 100, nullptr);
    randBoxTree.setProperty(juce::Identifier("smallBoxProb"), 0.9f, nullptr);
    randBoxTree.setProperty(juce::Identifier("smallBoxRatioLimit"), 4.f, nullptr);
    randBoxTree.setProperty(juce::Identifier("largeBoxRatioLimit"), 3.f, nullptr);
    randBoxTree.setProperty(juce::Identifier("largeBoxRadiusMul"), 0.65f, nullptr);
    randBoxTree.setProperty(juce::Identifier("smallBoxUseNormalDist"), false, nullptr);
    randBoxTree.setProperty(juce::Identifier("smallBoxDistUnifA"), 1.f, nullptr);
    randBoxTree.setProperty(juce::Identifier("smallBoxDistUnifB"), 4.f, nullptr);
    randBoxTree.setProperty(juce::Identifier("smallBoxDistMu"), 2.f, nullptr);
    randBoxTree.setProperty(juce::Identifier("smallBoxDistSigma"), 2.f, nullptr);
    randBoxTree.setProperty(juce::Identifier("largeBoxUseNormalDist"), false, nullptr);
    randBoxTree.setProperty(juce::Identifier("largeBoxDistUnifA"), 8.f, nullptr);
    randBoxTree.setProperty(juce::Identifier("largeBoxDistUnifB"), 12.f, nullptr);
    randBoxTree.setProperty(juce::Identifier("largeBoxDistMu"), 8.f, nullptr);
    randBoxTree.setProperty(juce::Identifier("largeBoxDistSigma"), 2.f, nullptr);
    state.appendChild(randBoxTree, nullptr);

    juce::ValueTree randBoxSelect(juce::Identifier("Random Box Selection"));
    randBoxSelect.setProperty(juce::Identifier("numRooms"), 12, nullptr);
    randBoxSelect.setProperty(juce::Identifier("allowTouching"), false , nullptr);
    state.appendChild(randBoxSelect, nullptr);

    juce::ValueTree otherTree(juce::Identifier("Line Connection"));
    otherTree.setProperty(juce::Identifier("addBackProb"), 0.1f, nullptr);
    otherTree.setProperty(juce::Identifier("overlapPadding"), 3, nullptr);
    otherTree.setProperty(juce::Identifier("addBothDirection"), false, nullptr);
    otherTree.setProperty(juce::Identifier("firstHorizontalProb"), 0.5f, nullptr);
    otherTree.setProperty(juce::Identifier("maxRoomSize"), 12, nullptr);
    state.appendChild(otherTree, nullptr);

    state.addListener(this);
	
    canvasComp = std::make_unique<CanvasOverlayComponent>(this, state);
	
    std::for_each(stepBtns.begin(), stepBtns.end(), [this](juce::Component* c) { addAndMakeVisible(c); });

    addAndMakeVisible(propertyPanel);
    addAndMakeVisible(canvasComp.get());
    addAndMakeVisible(layoutResizer);
	
    addAndMakeVisible(btnResetView, 999);
    addAndMakeVisible(btnPipeline, 999);
    addAndMakeVisible(btnTileColor, 999);

    layout.setItemLayout(0, -0.1, -0.5, 400);
    layout.setItemLayout(1, 5, 5, 5);
    layout.setItemLayout(2, -0.5, -0.9, -0.8);

    setSize(1200, 800);

    for (auto child : state)
    {
        juce::Array<juce::PropertyComponent*> pc;
        for (int i = 0; i < child.getNumProperties(); ++i)
        {
            const juce::Identifier name = child.getPropertyName(i);
            juce::Value v = child.getPropertyAsValue(name, nullptr);
            juce::PropertyComponent* tpc;

            if (v.getValue().isBool())
                tpc = new juce::BooleanPropertyComponent(v, name.toString(), "");
            else if (name.toString().endsWith("Prob"))
                tpc = new CustomSliderPropertyComponent(v, name.toString(), 0.0, 1.0, 0.01);
            else
                tpc = new juce::TextPropertyComponent(v, name.toString(), 20, false, true);
            pc.add(tpc);
        }
        propertyPanel.addSection(child.getType().toString(), pc, true, -1, 5);
    }

    for (int i = 0; i < stepBtns.size(); i++)
        stepBtns[i]->onClick = [this, i]() { runPipeline(i); };

    btnResetView.onClick = [this]() {
		canvasComp->viewDx = 0;
        canvasComp->viewDy = 0;
        canvasComp->scale = 10;
        canvasComp->fScale = 10;
		repaint();
	};
    btnPipeline.onClick = [this]() {
        auto& generalParams = state.getChildWithName(juce::Identifier("General"));
        generalParams.setProperty("seed", (int)std::random_device{}(), nullptr);
        if (lastStep == -1)
            lastStep = 9;
        runPipeline(lastStep);
    };
    btnTileColor.setToggleState(true, false);
    btnTileColor.onClick = [this]() {
        canvasComp->tileColor = !canvasComp->tileColor;
        repaint();
    };

}

MainComponent::~MainComponent()
{
    state.removeListener(this);
}

void MainComponent::valueTreePropertyChanged(juce::ValueTree& treeWhosePropertyHasChanged, const juce::Identifier& property)
{
    runPipeline(lastStep);
}

//==============================================================================
void MainComponent::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    auto bounds = getLocalBounds();

    auto gridBounds = bounds.removeFromTop(55).reduced(5);
    juce::Grid grid;
    using Track = juce::Grid::TrackInfo;
    using Fr = juce::Grid::Fr;
    using Px = juce::Grid::Px;
    grid.templateRows = { Track(Fr(1)) };
    grid.templateColumns.resize(10);
    grid.templateColumns.fill(Track(Fr(1)));
    for (auto& btn : stepBtns)
        grid.items.add(juce::GridItem(btn));
    grid.columnGap = Px(5);
    grid.performLayout(gridBounds);

    auto layoutBounds = bounds;
    juce::Component panelDummy, canvasDummy;
    juce::Component* comps[] = { &panelDummy, &layoutResizer, &canvasDummy };
	layout.layOutComponents(comps, 3, layoutBounds.getX(), layoutBounds.getY(), layoutBounds.getWidth(), layoutBounds.getHeight(), false, true);
	
    if (lastStep > -1 && lastStep < stepBtns.size())
    {
        g.setColour(juce::Colours::aqua);
        auto btnBound = stepBtns[lastStep]->getBounds().expanded(2);
        g.drawRoundedRectangle(btnBound.getX(), btnBound.getY(), btnBound.getWidth(), btnBound.getHeight(), 8.0f, 2.0f);
    }

    auto propertyBounds = panelDummy.getBounds().reduced(5);
    btnPipeline.setBounds(propertyBounds.removeFromTop(50));
    propertyBounds.removeFromTop(5);
    propertyPanel.setBounds(propertyBounds.reduced(0, 5));

    auto canvasBounds = canvasDummy.getBounds();
    auto bottomBounds = canvasBounds.removeFromBottom(50).reduced(10);
    canvasComp->setBounds(canvasBounds.reduced(5));
    btnResetView.setBounds(bottomBounds.removeFromRight(100));
    btnTileColor.setBounds(bottomBounds.removeFromLeft(100));
	
    g.setColour(juce::Colours::grey.withAlpha(0.2f));
    g.fillRect(layoutResizer.getBoundsInParent());
}

void MainComponent::resized()
{
    repaint();
}

void MainComponent::runPipeline(int step)
{
    canvasComp->boxes.clear();
    canvasComp->rooms.clear();
    canvasComp->corridors.clear();
    canvasComp->edges.clear();
    canvasComp->mst_edges.clear();
    canvasComp->lines.clear();
    canvasComp->tiles.clear();
    canvasComp->generated = false;

    int s = 0;
    if (s <= step)
    {
        const auto& generalParams = state.getChildWithName(juce::Identifier("General"));
        const auto& boxGenParams = state.getChildWithName(juce::Identifier("Random Box Generation"));
        unsigned int seed = (int)generalParams.getProperty(juce::Identifier("seed"), 42);
        unsigned int maxIteration = (int)generalParams.getProperty(juce::Identifier("maxIteration"), 100000);
        bool useRectRegion = boxGenParams.getProperty(juce::Identifier("useRectRegion"), false);
        unsigned int radiusX = (int)boxGenParams.getProperty(juce::Identifier("radiusX"), 32);
        unsigned int radiusY = (int)boxGenParams.getProperty(juce::Identifier("radiusY"), 32);
        unsigned int numBox = (int)boxGenParams.getProperty(juce::Identifier("numBox"), 100);
        float smallBoxProb = boxGenParams.getProperty(juce::Identifier("smallBoxProb"), 0.9f);
        bool smallBoxUseNormalDist = boxGenParams.getProperty(juce::Identifier("smallBoxUseNormalDist"), false);
        float smallBoxDistUnifA = boxGenParams.getProperty(juce::Identifier("smallBoxDistUnifA"), 1.f);
        float smallBoxDistUnifB = boxGenParams.getProperty(juce::Identifier("smallBoxDistUnifB"), 4.f);
        float smallBoxDistMu = boxGenParams.getProperty(juce::Identifier("smallBoxDistMu"), 2.f);
        float smallBoxDistSigma = boxGenParams.getProperty(juce::Identifier("smallBoxDistSigma"), 2.f);
        float smallBoxRatioLimit = boxGenParams.getProperty(juce::Identifier("smallBoxRatioLimit"), 4.f);
        bool largeBoxUseNormalDist = boxGenParams.getProperty(juce::Identifier("largeBoxUseNormalDist"), false);
        float largeBoxDistUnifA = boxGenParams.getProperty(juce::Identifier("largeBoxDistUnifA"), 8.f);
        float largeBoxDistUnifB = boxGenParams.getProperty(juce::Identifier("largeBoxDistUnifB"), 12.f);
        float largeBoxDistMu = boxGenParams.getProperty(juce::Identifier("largeBoxDistMu"), 10.f);
        float largeBoxDistSigma = boxGenParams.getProperty(juce::Identifier("largeBoxDistSigma"), 2.f);
        float largeBoxRatioLimit = boxGenParams.getProperty(juce::Identifier("largeBoxRatioLimit"), 3.f);
        float largeBoxRadiusMultiplier = boxGenParams.getProperty(juce::Identifier("largeBoxRadiusMultiplier"), 0.65f);
        canvasComp->boxes = engine.randBox(
            seed, useRectRegion, radiusX, radiusY,
            numBox, maxIteration,
            smallBoxProb, smallBoxUseNormalDist,
            smallBoxUseNormalDist ? smallBoxDistMu : smallBoxDistUnifA, smallBoxUseNormalDist ? smallBoxDistSigma : smallBoxDistUnifB,
            smallBoxRatioLimit,
            largeBoxUseNormalDist,
            largeBoxUseNormalDist ? largeBoxDistMu : largeBoxDistUnifA, largeBoxUseNormalDist ? largeBoxDistSigma : largeBoxDistUnifB,
            largeBoxRatioLimit, largeBoxRadiusMultiplier);
        s++;
    }
    if (s <= step)
    {
        canvasComp->boxes = engine.separateBox(canvasComp->boxes);
        s++;
    }
    if (s <= step)
    {
        const auto& generalParams = state.getChildWithName(juce::Identifier("General"));
        unsigned int mapWidth = (int)generalParams.getProperty(juce::Identifier("mapWidth"), 64);
        unsigned int mapHeight = (int)generalParams.getProperty(juce::Identifier("mapHeight"), 64);
        canvasComp->boxes = engine.centerAndCropBox(canvasComp->boxes, mapWidth, mapHeight);
        s++;
    }
    if (s <= step)
    {
        const auto& selectionParams = state.getChildWithName(juce::Identifier("Random Box Selection"));
        unsigned int numRooms = (int)selectionParams.getProperty(juce::Identifier("numRooms"), 64);
        bool allowTouching = selectionParams.getProperty(juce::Identifier("allowTouching"), false);
        auto ret = engine.randSelect(canvasComp->boxes, numRooms, allowTouching);
        canvasComp->boxes = ret.first;
        canvasComp->rooms = ret.second;
        s++;
    }
    if (s <= step)
    {
        canvasComp->edges = engine.triangulate(canvasComp->rooms);
        s++;
    }
    if (s <= step)
    {
        const auto& selectionParams = state.getChildWithName(juce::Identifier("Random Box Selection"));
        unsigned int numRooms = (int)selectionParams.getProperty(juce::Identifier("numRooms"), 64);
        canvasComp->mst_edges = engine.mst(canvasComp->edges);
        s++;
    }
    if (s <= step)
    {
        const auto& generalParams = state.getChildWithName(juce::Identifier("General"));
        const auto& lineParams = state.getChildWithName(juce::Identifier("Line Connection"));
        unsigned int seed = (int)generalParams.getProperty(juce::Identifier("seed"), 42);
        float addBackProb = lineParams.getProperty(juce::Identifier("addBackProb"), 0.1f);
        canvasComp->mst_edges = engine.addSomeEdgesBack(seed, canvasComp->edges, canvasComp->mst_edges, addBackProb);
        s++;
    }
    if (s <= step)
    {
        const auto& generalParams = state.getChildWithName(juce::Identifier("General"));
        const auto& lineParams = state.getChildWithName(juce::Identifier("Line Connection"));
        unsigned int seed = (int)generalParams.getProperty(juce::Identifier("seed"), 42);
        unsigned int overlapPadding = (int)lineParams.getProperty(juce::Identifier("overlapPadding"), 3);
        bool addBothDirection = lineParams.getProperty(juce::Identifier("addBothDirection"), false);
        float firstHorizontalProb = lineParams.getProperty(juce::Identifier("firstHorizontalProb"), 0.5f);
        canvasComp->lines = engine.lineConnect(seed, canvasComp->rooms, canvasComp->mst_edges, overlapPadding, addBothDirection, firstHorizontalProb);
        s++;
    }
    if (s <= step)
    {
        const auto& lineParams = state.getChildWithName(juce::Identifier("Line Connection"));
        unsigned int maxRoomSize = (int)lineParams.getProperty(juce::Identifier("maxRoomSize"), 12);
        auto ret = engine.selectCorridors(canvasComp->boxes, canvasComp->lines, maxRoomSize);
        canvasComp->boxes = ret.first;
        canvasComp->corridors = ret.second;
        s++;
    }
    if (s <= step)
    {
        const auto& generalParams = state.getChildWithName(juce::Identifier("General"));
        unsigned int mapWidth = (int)generalParams.getProperty(juce::Identifier("mapWidth"), 64);
        unsigned int mapHeight = (int)generalParams.getProperty(juce::Identifier("mapHeight"), 64);
        canvasComp->tiles = engine.tiling(canvasComp->rooms, canvasComp->corridors, canvasComp->lines, mapWidth, mapHeight);
        canvasComp->generated = true;
        s++;
    }

    lastStep = step;
    repaint();
}