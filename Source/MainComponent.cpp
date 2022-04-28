#include "MainComponent.h"

#include "delaunator.h"

#define M_PI 3.14159265358979323846

template<typename FunctionType>
inline void visitEach(std::initializer_list<juce::Component*> comps, FunctionType&& fn)
{
    std::for_each(std::begin(comps), std::end(comps), fn);
}

//==============================================================================
MainComponent::MainComponent()
{
    visitEach({ &btnRandBox, &btnSep, &btnCenter,
        &btnSelect, &btnTri, &btnMst, &btnAddBack,
        &btnLine, &btnPipeline, &btnFinal, &btnZoomIn, &btnZoomOut },
        [this](Component* c) { this->addAndMakeVisible(c); });

    setSize(1000, 700);

    btnRandBox.onClick = [this]() { this->randBox(); };
	btnSep.onClick = [this]() { this->separateBox(); };
    btnCenter.onClick = [this]() { this->centerAndCropBox(); };

    btnSelect.onClick = [this]() { this->randSelect(); };
    btnTri.onClick = [this]() { this->triangulate(); };
    btnMst.onClick = [this]() { this->mst(); };
    btnAddBack.onClick = [this]() { this->addBack(); };
    btnLine.onClick = [this]() { this->lineConnect(); };

    btnPipeline.onClick = [this]() {
        this->randBox();
        this->separateBox();
        this->centerAndCropBox();
        this->randSelect();
        this->triangulate();
        this->mst();
        this->addBack();
        this->lineConnect();
    };
    btnFinal.onClick = [this]() { this->tiling(); };

    btnZoomIn.onClick = [this]() { this->scale += 2; this->repaint(); };
    btnZoomOut.onClick = [this]() { this->scale -= 2; this->repaint(); };
}

MainComponent::~MainComponent()
{
}

void MainComponent::randBox()
{
    int numBox = 100;
    int radius = 1;

    std::default_random_engine generator(std::random_device{}());
    std::uniform_real_distribution<float> pos_dist(0.0f, 1.0f);
    std::uniform_real_distribution<float> choice_dist(0.0f, 1.0f);
    std::uniform_int_distribution<int> short_edge_dist(1, 4);
    std::uniform_int_distribution<int> long_edge_dist(7, 10);
	
    boxes.clear();
    rooms.clear();
    corridors.clear();
    edges.clear();
    mstEdges.clear();
    lines.clear();

    tiles.clear();
    generated = false;
	
    std::set<RoomBox, RoomBoxComp> boxSet;
    int i = 0;
    while (i < numBox)
    {
        double t = 2 * M_PI * pos_dist(generator);
        double u = pos_dist(generator) + pos_dist(generator);
        double r = 0.f;
        if (u > 1.0)
            r = 2.0f - u;
        else
            r = u;
		
        int w = 0, h = 0;
        if (choice_dist(generator) < 0.9)
        {
            w = short_edge_dist(generator);
            h = short_edge_dist(generator);
        }
        else
        {
            w = long_edge_dist(generator);
			h = long_edge_dist(generator);
            if ((float)w / h > 3.0f || (float)h / w > 3.0f)
                continue;
            r /= 1.5;
        }
        RoomBox box(radius * r * std::cos(t), radius * r * std::sin(t), w, h);
        auto result = boxSet.insert(box);
        //boxes.push_back(box);
		if (result.second)
            i++;
    }
	for (auto& box : boxSet)
		boxes.push_back(box);
    std::sort(boxes.begin(), boxes.end());
    boxes[0].snapToGrid();

    repaint();
}

void MainComponent::separateBox()
{
    bool overlapped = true;
    for (int round = 0; round < 10 && overlapped; round++)
    {
        for (int current = 1; current < boxes.size(); current++)
        {
            double dirx = boxes[current].cx, diry = boxes[current].cy;
            //bool overlapped = true;
            //for (int fidx = 0; fidx < current; fidx++)
            //{
            //    if (boxes[current].isOverlap(boxes[fidx]))
            //    {
            //        overlapped = true;
            //        auto dir = boxes[current].getDirection(boxes[fidx]);
            //        DBG("Overlap " << fidx << " dir:" << dir.first << "," << dir.second);
            //        if (dirx + dir.first == 0.0 && diry + dir.second == 0.0)
            //            continue;
            //        else
            //        {
            //            dirx += dir.first;
            //            diry += dir.second;
            //        }
            //        DBG("Now dir: " << dirx << "," << diry);
            //    }
            //}
            double norm = std::sqrt(dirx * dirx + diry * diry);
            dirx /= norm;
            diry /= norm;
            for (int fidx = 0; fidx < current; fidx++)
            {
                if (boxes[current].isOverlap(boxes[fidx]))
                {
                    overlapped = true;
                    boxes[current].moveAwayFrom(boxes[fidx], dirx, diry);
                }
            }
        }
    }
    repaint();
}

void MainComponent::centerAndCropBox()
{
    double centerX = 0.0, centerY = 0.0;
	for (auto& box : boxes)
	{
        centerX += box.cx;
		centerY += box.cy;
	}
	centerX /= boxes.size();
	centerY /= boxes.size();
	centerX = std::round(centerX);
	centerY = std::round(centerY);

    std::vector<RoomBox> filtered;
	for (auto& box : boxes)
	{
        box.moveDelta(-centerX, -centerY);
        if (box.x < -(mapWidth / 2) || box.y < -(mapHeight / 2) || 
            box.x + box.w > mapWidth / 2 || box.y + box.h > mapHeight / 2)
            continue;
        filtered.push_back(box);
	}
    boxes = filtered;
	
	repaint();
}

void MainComponent::randSelect()
{
    int numRooms = 12;
    std::sort(boxes.begin(), boxes.end(), [](const RoomBox& a, const RoomBox& b) { return a.getSize() > b.getSize(); });

    auto it = boxes.begin();
    while (it != boxes.end() && numRooms > 0)
    {
        bool touching = false;
        for (const auto& room : rooms)
            touching |= room.isTouching(*it);
        if (!touching)
        {
            rooms.push_back(*it);
            it = boxes.erase(it);
            numRooms--;
        }
        else
            it++;
    }
    repaint();
}

void MainComponent::triangulate()
{
    edges.clear();

    std::vector<double> coords;
	for (const auto& room : rooms)
	{
        coords.push_back(room.cx);
        coords.push_back(room.cy);
	}
    delaunator::Delaunator d(coords);
    for (std::size_t i = 0; i < d.triangles.size(); i += 3)
    {
        edges.insert({ d.triangles[i], d.triangles[i + 1], rooms[d.triangles[i]].getHamiltonDist(rooms[d.triangles[i + 1]]) });
        edges.insert({ d.triangles[i + 1], d.triangles[i + 2], rooms[d.triangles[i + 1]].getHamiltonDist(rooms[d.triangles[i + 2]]) });
        edges.insert({ d.triangles[i + 2], d.triangles[i], rooms[d.triangles[i + 2]].getHamiltonDist(rooms[d.triangles[i]]) });

        edges.insert({ d.triangles[i + 1], d.triangles[i], rooms[d.triangles[i]].getHamiltonDist(rooms[d.triangles[i + 1]]) });
        edges.insert({ d.triangles[i + 2], d.triangles[i + 1], rooms[d.triangles[i + 1]].getHamiltonDist(rooms[d.triangles[i + 2]]) });
        edges.insert({ d.triangles[i], d.triangles[i + 2], rooms[d.triangles[i + 2]].getHamiltonDist(rooms[d.triangles[i]]) });
    }
    repaint();
}

void MainComponent::mst()
{
    mstEdges.clear();

    using iPair = std::pair<int, double>;

    std::vector<std::vector<iPair>> adj;
    adj.resize(rooms.size());
    for (const auto& e : edges)
    {
        int u, v;
        double w;
        std::tie(u, v, w) = e;

        adj[u].push_back({ v, w });
    }

    std::priority_queue<iPair, std::vector<iPair>, std::greater<iPair>> pq;
    int src = 0;
    std::vector<double> key(rooms.size(), std::numeric_limits<double>::max());
    std::vector<int> parent(rooms.size(), -1);
    std::vector<bool> inMst(rooms.size(), false);

    pq.push({ 0.0, src });
    key[src] = 0.0;
    while (!pq.empty())
    {
        int u = pq.top().second;
        pq.pop();
        if (inMst[u] == true) {
            continue;
        }
        inMst[u] = true;

        for (const auto& neigh : adj[u])
        {
            int v = neigh.first;
            int weight = neigh.second;

            if (inMst[v] == false && key[v] > weight)
            {
                key[v] = weight;
                pq.push({ key[v], v });
                parent[v] = u;
            }
        }
    }
    for (int i = 1; i < rooms.size(); i++)
    {
        mstEdges.insert({ i, parent[i] });
        mstEdges.insert({ parent[i], i });
    }

    repaint();
}

void MainComponent::addBack()
{
    using Edge = std::tuple<int, int, double>;

    std::default_random_engine generator(std::random_device{}());
    std::uniform_real_distribution<float> choice_dist(0.0f, 1.0f);

    for (const auto& e : edges)
    {
        int na, nb;
        double w;
        std::tie(na, nb, w) = e;
        if (mstEdges.find({ na, nb }) == mstEdges.end() && choice_dist(generator) < 0.1)
        {
            mstEdges.insert({ na, nb });
        }
    }
    repaint();
}

void MainComponent::lineConnect()
{
    lines.clear();

    std::default_random_engine generator(std::random_device{}());
    std::uniform_real_distribution<float> choice_dist(0.0f, 1.0f);

    for (const auto& e : mstEdges)
    {
        const RoomBox& a = rooms[e.first];
        const RoomBox& b = rooms[e.second];
        if (abs(a.cx - b.cx) <= a.w / 2.0 + b.w / 2.0 - 3)
        {
            auto centerx = (a.cx + b.cx) / 2.0;
            lines.insert({ centerx, a.cy, centerx, b.cy });
        }
        else if (abs(a.cy - b.cy) <= a.h / 2.0 + b.h / 2.0 - 3)
        {
            auto centery = (a.cy + b.cy) / 2.0;
            lines.insert({ a.cx, centery, b.cx, centery });
        }
        else
        {
            if (choice_dist(generator) < 0.5)
            {
                lines.insert({ a.cx, a.cy, b.cx, a.cy });
                lines.insert({ b.cx, a.cy, b.cx, b.cy });
            }
            else
            {
                lines.insert({ a.cx, a.cy, a.cx, b.cy });
                lines.insert({ a.cx, b.cy, b.cx, b.cy });
            }
        }
    }

    auto it = boxes.begin();
    while (it != boxes.end())
    {
        if (it->getSize() > 12)
        {
            it++;
            continue;
        }
        bool touching = false;
        for (const auto& line : lines)
        {
            if (it->isTouchingLine(line))
            {
                touching = true;
                break;
            }
        }
        if (touching)
        {
            corridors.push_back(*it);
            it = boxes.erase(it);
        }
        else
            it++;
    }
    repaint();
}

void MainComponent::tiling()
{
    tiles.clear();
    tiles.resize(mapWidth * mapHeight);

    int halfW = mapWidth / 2;
    int halfH = mapHeight / 2;
    for (const auto& line : lines)
    {
        double x1, y1, x2, y2;
        std::tie(x1, y1, x2, y2) = line;
        x1 += halfW;
        x2 += halfW;
        y1 += halfH;
        y2 += halfH;
        if (y1 == y2)
        {
            if (x2 < x1)
                std::swap(x1, x2);
            for (int x = std::max(0.0, floor(x1 - 0.5)); x < std::min((double)mapWidth, floor(x2 + 0.5)); x++)
            {
                tiles[(int)std::max(0.0, floor(y1 - 0.5)) * mapWidth + x] = 1;
                tiles[(int)std::min((double)mapHeight, floor(y1 + 0.5)) * mapWidth + x] = 1;
            }
        }
        else if (x1 == x2)
        {
            if (y2 < y1)
                std::swap(y1, y2);
            for (int y = std::max(0.0, floor(y1 - 0.5)); y < std::min((double)mapHeight, floor(y2 + 0.5)); y++)
            {
                tiles[y * mapWidth + (int)std::max(0.0, floor(x1 - 0.5))] = 1;
                tiles[y * mapWidth + (int)std::min((double)mapWidth, floor(x1 + 0.5))] = 1;
            }
        }
    }
    for (const auto& box : corridors)
    {
        for (int y = box.y + halfH; y < box.y + box.h + halfH; y++)
        {
            for (int x = box.x + halfW; x < box.x + box.w + halfW; x++)
            {
                tiles[y * mapWidth + x] = 2;
            }
        }
    }
    for (const auto& box : rooms)
    {
        for (int y = box.y + halfH; y < box.y + box.h + halfH; y++)
        {
            for (int x = box.x + halfW; x < box.x + box.w + halfW; x++)
            {
                tiles[y * mapWidth + x] = 3;
            }
        }
    }

    generated = true;

    repaint();
}

//==============================================================================
void MainComponent::paint (juce::Graphics& g)
{
    static const float dashes[2] = { 3.0f, 3.0f };
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    auto b = getLocalBounds().reduced(5).removeFromTop(40);

    juce::FlexBox flexBox(juce::FlexBox::Direction::row, juce::FlexBox::Wrap::wrap, juce::FlexBox::AlignContent::stretch, juce::FlexBox::AlignItems::stretch, juce::FlexBox::JustifyContent::center);
    visitEach({ &btnRandBox, &btnSep, &btnCenter,
        &btnSelect, &btnTri, &btnMst, &btnAddBack,
        &btnLine, &btnPipeline, &btnFinal,&btnZoomIn,&btnZoomOut },
        [&flexBox](Component* c) { flexBox.items.add(juce::FlexItem(*c).withMinHeight(10).withMinWidth(50).withFlex(1)); });
    flexBox.performLayout(b);

    auto b2 = getLocalBounds().reduced(5).withTrimmedTop(45);

    if (!generated)
    {
        g.setColour(juce::Colours::grey);
        for (int i = -mapWidth / 2; i < mapWidth / 2; i++)
        {
            g.drawLine(juce::Line<float>(b2.getCentreX() + i * scale, -mapHeight / 2 * scale + b2.getCentreY(), b2.getCentreX() + i * scale, mapHeight / 2 * scale + b2.getCentreY()));
        }
        for (int i = -mapHeight / 2; i < mapHeight / 2; i++)
        {
            g.drawLine(juce::Line<float>(-mapWidth / 2 * scale + b2.getCentreX(), b2.getCentreY() + i * scale, mapWidth / 2 * scale + b2.getCentreX(), b2.getCentreY() + i * scale));
        }
        g.setColour(juce::Colours::black);
        g.drawLine(juce::Line<float>(b2.getCentreX(), -mapHeight / 2 * scale + b2.getCentreY(), b2.getCentreX(), mapHeight / 2 * scale + b2.getCentreY()), 2.f);
        g.drawLine(juce::Line<float>(-mapWidth / 2 * scale + b2.getCentreX(), b2.getCentreY(), mapWidth / 2 * scale + b2.getCentreX(), b2.getCentreY()), 2.f);
    }

    int idx = 0;
	for (const auto& box : boxes)
	{
        auto rect = juce::Rectangle<float>(
            box.x * scale + b2.getCentreX(),
            box.y * scale + b2.getCentreY(),
            box.w * scale, box.h * scale);

        g.setColour(juce::Colours::lightgreen.withAlpha(0.5f));
        g.fillRect(rect);
        g.setColour(juce::Colours::orange.withAlpha(0.5f));
        g.drawRect(rect, 3.f);

        g.setColour(juce::Colours::red);
        g.fillEllipse(juce::Rectangle<float>(box.cx * scale + b2.getCentreX() - 4, box.cy * scale + b2.getCentreY() - 4, 8, 8));

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
    for (const auto& box : rooms)
    {
        auto rect = juce::Rectangle<float>(
            box.x * scale + b2.getCentreX(),
            box.y * scale + b2.getCentreY(),
            box.w * scale, box.h * scale);

        g.setColour(juce::Colours::skyblue.withAlpha(0.8f));
        g.fillRect(rect);
        g.setColour(juce::Colours::orange.withAlpha(0.5f));
        g.drawRect(rect, 3.f);

        g.setColour(juce::Colours::red);
        g.fillEllipse(juce::Rectangle<float>(box.cx * scale + b2.getCentreX() - 4, box.cy * scale + b2.getCentreY() - 4, 8, 8));

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
    for (const auto& box : corridors)
    {
        auto rect = juce::Rectangle<float>(
            box.x * scale + b2.getCentreX(),
            box.y * scale + b2.getCentreY(),
            box.w * scale, box.h * scale);

        g.setColour(juce::Colours::green.withAlpha(0.7f));
        g.fillRect(rect);
        g.setColour(juce::Colours::orange.withAlpha(0.5f));
        g.drawRect(rect, 3.f);

        g.setColour(juce::Colours::red);
        g.fillEllipse(juce::Rectangle<float>(box.cx * scale + b2.getCentreX() - 4, box.cy * scale + b2.getCentreY() - 4, 8, 8));

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
		g.drawDashedLine(juce::Line<float>(x1 * scale + b2.getCentreX(), y1 * scale + b2.getCentreY(), x2 * scale + b2.getCentreX(), y2 * scale + b2.getCentreY()), dashes, 2, 2.f);
	}
    for (const auto& e : mstEdges)
    {
        g.setColour(juce::Colours::green);
        double x1 = rooms[e.first].cx;
        double y1 = rooms[e.first].cy;
        double x2 = rooms[e.second].cx;
        double y2 = rooms[e.second].cy;
        g.drawDashedLine(juce::Line<float>(x1 * scale + b2.getCentreX(), y1 * scale + b2.getCentreY(), x2 * scale + b2.getCentreX(), y2 * scale + b2.getCentreY()), dashes, 2, 3.f);
    }
    for (const auto& e : lines)
    {
        g.setColour(juce::Colours::purple.brighter());
        double x1, y1, x2, y2;
        std::tie(x1, y1, x2, y2) = e;
        g.drawLine(x1 * scale + b2.getCentreX(), y1 * scale + b2.getCentreY(), x2 * scale + b2.getCentreX(), y2 * scale + b2.getCentreY(), 3.f);
    }

    if (generated)
    {
        g.setColour(juce::Colours::white.withAlpha(0.5f));
        g.fillRect(-mapWidth / 2 * scale + b2.getCentreX(), -mapHeight / 2 * scale + b2.getCentreY(), mapWidth * scale, mapHeight * scale);
        for (int i = -mapHeight / 2; i < mapHeight / 2; i++)
        {
            for (int j = -mapWidth / 2; j < mapWidth / 2; j++)
            {
                auto type = tiles[(i + mapHeight / 2) * mapWidth + j + mapWidth / 2];
                g.setColour(juce::Colours::black);
                juce::Rectangle<float> cell(j * scale + b2.getCentreX(), i * scale + b2.getCentreY(), scale, scale);
                g.drawRect(cell, 0.5f);
                if (type > 0)
                {
                    if (type == 1)
                        g.setColour(juce::Colours::lightcyan);
                    else if (type == 2)
                        g.setColour(juce::Colours::lightgreen);
                    else if (type == 3)
                        g.setColour(juce::Colours::skyblue);
                    g.fillRect(cell);
                }
                g.setColour(juce::Colours::black);
                g.setFont(scale);
                g.drawText(juce::String(tiles[(i + mapHeight / 2) * mapWidth + j + mapWidth / 2]), cell, juce::Justification::centred);
            }
        }
    }
}

void MainComponent::resized()
{
    // This is called when the MainComponent is resized.
    // If you add any child components, this is where you should
    // update their positions.
}
