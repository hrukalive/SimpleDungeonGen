/*
  ==============================================================================

    DungeonGenerationEngine.cpp
    Created: 3 May 2022 7:38:36pm
    Author:  bowen

  ==============================================================================
*/

#include "DungeonGenerationEngine.h"
#include "delaunator.h"

#define M_PI 3.14159265358979323846

DungeonGenerationEngine::RoomBox::RoomBox(double cx, double cy, double w, double h)
    : cx(cx), cy(cy), w(w), h(h)
{
    x = cx - w / 2.0;
    y = cy - h / 2.0;
}

void DungeonGenerationEngine::RoomBox::snapToGrid()
{
    x = (x < 0) ? floor(x) : ceil(x);
    y = (y < 0) ? floor(y) : ceil(y);
    cx = x + w / 2.0;
    cy = y + h / 2.0;
}

void DungeonGenerationEngine::RoomBox::moveDelta(double dx, double dy)
{
    x += dx;
    y += dy;
    cx = x + w / 2.0;
    cy = y + h / 2.0;
}

bool DungeonGenerationEngine::RoomBox::operator<(const RoomBox& other) const
{
    return getDistance() < other.getDistance();
}

double DungeonGenerationEngine::RoomBox::getDistance() const
{
    return sqrt(cx * cx + cy * cy);
}

double DungeonGenerationEngine::RoomBox::getHamiltonDist(const RoomBox& other) const
{
    return abs(cx - other.cx) + abs(cy - other.cy);
}

double DungeonGenerationEngine::RoomBox::getSize() const
{
    return w * h;
}

bool DungeonGenerationEngine::RoomBox::isOverlap(const RoomBox& other) const
{
    return abs(cx - other.cx) < w / 2 + other.w / 2 && abs(cy - other.cy) < h / 2 + other.h / 2;
}

bool DungeonGenerationEngine::RoomBox::isTouching(const RoomBox& other) const
{
    return abs(cx - other.cx) <= w / 2 + other.w / 2 && abs(cy - other.cy) <= h / 2 + other.h / 2;
}

bool DungeonGenerationEngine::RoomBox::isTouchingLine(std::tuple<double, double, double, double> line)
{
    double x1, y1, x2, y2;
    std::tie(x1, y1, x2, y2) = line;

    return
        (y1 == y2 && abs(y1 - cy) <= h / 2.0 && ((x1 <= cx && x2 >= cx || x1 >= cx && x2 <= cx) || abs(x1 - cx) < w / 2.0 || abs(x2 - cx) < w / 2.0)) ||
        (x1 == x2 && abs(x1 - cx) <= w / 2.0 && ((y1 <= cy && y2 >= cy || y1 >= cy && y2 <= cy) || abs(y1 - cy) < h / 2.0 || abs(y2 - cy) < h / 2.0));
}

std::pair<double, double> DungeonGenerationEngine::RoomBox::getDirection(const RoomBox& fixed) const
{
    // Normalized direction
    double dx = cx - fixed.cx;
    double dy = cy - fixed.cy;
    if (dx == 0.0 && dy == 0.0)
    {
        dx = 2 * static_cast<float>(rand()) / static_cast<float>(RAND_MAX) - 1;
        dy = 2 * static_cast<float>(rand()) / static_cast<float>(RAND_MAX) - 1;
    }
    double d = sqrt(dx * dx + dy * dy);
    return { dx / d, dy / d };
}

void DungeonGenerationEngine::RoomBox::moveAwayFrom(const RoomBox& fixed, double dirx, double diry)
{
    double targetX = 0;
    double targetY = 0;

    if (dirx == 0 || diry == 0)
    {
        if (dirx > 0 && diry == 0)
        {
            targetX = fixed.x + fixed.w;
            targetY = y;
        }
        else if (dirx < 0 && diry == 0)
        {
            targetX = fixed.x - w;
            targetY = y;
        }
        else if (dirx == 0 && diry > 0)
        {
            targetX = x;
            targetY = fixed.y + fixed.h;
        }
        else if (dirx == 0 && diry < 0)
        {
            targetX = x;
            targetY = fixed.y - h;
        }
    }
    else
    {
        if (dirx > 0 && diry > 0) {
            targetX = fixed.x + fixed.w;
            targetY = fixed.y + fixed.h;
        }
        else if (dirx > 0 && diry < 0) {
            targetX = fixed.x + fixed.w;
            targetY = fixed.y - h;
        }
        else if (dirx < 0 && diry > 0) {
            targetX = fixed.x - w;
            targetY = fixed.y + fixed.h;
        }
        else if (dirx < 0 && diry < 0) {
            targetX = fixed.x - w;
            targetY = fixed.y - h;
        }
        else
        {
            throw std::runtime_error("Invalid direction");
        }
        double dx = targetX - x;
        double dy = targetY - y;

        double dx2 = (dy / diry) * dirx;
        double dy2 = (dx / dirx) * diry;

        if (abs(dx) + abs(dy2) < abs(dx2) + abs(dy))
        {
            targetX = x + dx;
            targetY = y + dy2;
        }
        else
        {
            targetX = x + dx2;
            targetY = y + dy;
        }
        targetX = (targetX < 0) ? floor(targetX) : ceil(targetX);
        targetY = (targetY < 0) ? floor(targetY) : ceil(targetY);
    }
    x = targetX;
    y = targetY;
    cx = x + w / 2.0;
    cy = y + h / 2.0;
}

constexpr bool DungeonGenerationEngine::RoomBoxComp::operator()(const RoomBox& lhs, const RoomBox& rhs) const
{
    return std::tie(lhs.cx, lhs.cy) < std::tie(rhs.cx, rhs.cy);
}
constexpr bool DungeonGenerationEngine::CustomTupleComp::operator()(const std::tuple<int, int, double>& lhs, const std::tuple<int, int, double>& rhs) const
{
    return std::tie(std::get<0>(lhs), std::get<1>(lhs)) < std::tie(std::get<0>(rhs), std::get<1>(rhs));
}


DungeonGenerationEngine::RoomBoxVec DungeonGenerationEngine::randBox(
    unsigned int seed, bool useRectRegion, float radiusX, float radiusY,
    unsigned int numBox, unsigned int maxIteration, float smallBoxProb,
    bool smallBoxUseNormalDist, float smallBoxDistParamA, float smallBoxDistParamB, float smallBoxRatioLimit,
    bool largeBoxUseNormalDist, float largeBoxDistParamA, float largeBoxDistParamB, float largeBoxRatioLimit,
    float largeBoxRadiusMultiplier)
{
	smallBoxDistParamA = std::max(0.0f, smallBoxDistParamA);
	smallBoxDistParamB = std::max(0.0f, smallBoxDistParamB);
	largeBoxDistParamA = std::max(0.0f, largeBoxDistParamA);
	largeBoxDistParamB = std::max(0.0f, largeBoxDistParamB);
	
    if (!smallBoxUseNormalDist && smallBoxDistParamB < smallBoxDistParamA)
        return {};
    if (!largeBoxUseNormalDist && largeBoxDistParamB < largeBoxDistParamA)
        return {};

    std::default_random_engine generator(seed);
    std::uniform_real_distribution<float> pos_dist(0.0f, 1.0f);
    std::uniform_real_distribution<float> choice_dist(0.0f, 1.0f);
    std::uniform_int_distribution<int> unif_short_edge_dist((int)smallBoxDistParamA, (int)smallBoxDistParamB);
    std::uniform_int_distribution<int> unif_long_edge_dist((int)largeBoxDistParamA, (int)largeBoxDistParamB);
    std::normal_distribution<float> norm_short_edge_dist(smallBoxDistParamA, smallBoxDistParamB);
    std::normal_distribution<float> norm_long_edge_dist(largeBoxDistParamA, largeBoxDistParamB);

    RoomBoxVec boxes;
    std::set<RoomBox, RoomBoxComp> boxSet;

    radiusX = std::max(1.0f, radiusX);
    radiusY = std::max(1.0f, radiusY);
	largeBoxRadiusMultiplier = std::max(0.01f, largeBoxRadiusMultiplier);

    int i = 0;
    while (i < numBox && i < maxIteration)
    {
        bool largeBox = false;
        float w = 0, h = 0;
        if (choice_dist(generator) < smallBoxProb)
        {
            if (smallBoxUseNormalDist)
            {
                w = norm_short_edge_dist(generator);
                h = norm_short_edge_dist(generator);
            }
            else
            {
                w = unif_short_edge_dist(generator);
                h = unif_short_edge_dist(generator);
            }
            w = std::max(1.f, roundf(w));
            h = std::max(1.f, roundf(h));
            if (w / h > smallBoxRatioLimit || h / w > smallBoxRatioLimit)
                continue;
        }
        else
        {
            if (largeBoxUseNormalDist)
            {
                w = norm_long_edge_dist(generator);
                h = norm_long_edge_dist(generator);
            }
            else
            {
                w = unif_long_edge_dist(generator);
                h = unif_long_edge_dist(generator);
            }
            w = std::max(1.f, roundf(w));
            h = std::max(1.f, roundf(h));
            if ((float)w / h > largeBoxRatioLimit || (float)h / w > largeBoxRatioLimit)
                continue;
            largeBox = true;
        }
        double cx = 0, cy = 0;
        if (useRectRegion)
        {
			cx = (pos_dist(generator) - 0.5) * 2.0 * radiusX;
			cy = (pos_dist(generator) - 0.5) * 2.0 * radiusY;
        }
        else
        {
            double t = 2 * M_PI * pos_dist(generator);
            double u = std::sqrt(pos_dist(generator));
			if (largeBox)
                u *= largeBoxRadiusMultiplier;
            cx = radiusX * u * std::cos(t);
            cy = radiusY * u * std::sin(t);
        }
        RoomBox box(cx, cy, (int)w, (int)h);
        auto result = boxSet.insert(box);
        if (result.second)
            i++;
    }
    for (auto& box : boxSet)
        boxes.push_back(box);
    std::sort(boxes.begin(), boxes.end());
    boxes[0].snapToGrid();

    return boxes;
}

DungeonGenerationEngine::RoomBoxVec DungeonGenerationEngine::separateBox(RoomBoxVec boxes)
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
    return boxes;
}

DungeonGenerationEngine::RoomBoxVec DungeonGenerationEngine::centerAndCropBox(RoomBoxVec boxes, unsigned int mapWidth, unsigned int mapHeight)
{
    if (boxes.size() == 0)
        return boxes;
	
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

    RoomBoxVec filtered;
    for (auto& box : boxes)
    {
        box.moveDelta(-centerX, -centerY);
        if (box.x < -((int)mapWidth / 2) || box.y < -((int)mapHeight / 2) ||
            box.x + box.w >(int)mapWidth / 2 || box.y + box.h >(int)mapHeight / 2)
            continue;
        filtered.push_back(box);
    }
    return filtered;
}

std::pair<DungeonGenerationEngine::RoomBoxVec, DungeonGenerationEngine::RoomBoxVec> DungeonGenerationEngine::randSelect(RoomBoxVec boxes, unsigned int numRooms, bool allowTouching)
{
    std::sort(boxes.begin(), boxes.end(), [](const RoomBox& a, const RoomBox& b) { return a.getSize() > b.getSize(); });

    RoomBoxVec rooms;
    auto it = boxes.begin();
    while (it != boxes.end() && numRooms > 0)
    {
        bool touching = false;
        if (!allowTouching)
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
    return std::make_pair(boxes, rooms);
}

DungeonGenerationEngine::WeightedEdgeSet DungeonGenerationEngine::triangulate(const RoomBoxVec& rooms)
{
    WeightedEdgeSet edges;
    std::vector<double> coords;
    for (const auto& room : rooms)
    {
        coords.push_back(room.cx);
        coords.push_back(room.cy);
    }

    if (rooms.size() == 2)
    {
        edges.insert({ 0, 1, rooms[0].getHamiltonDist(rooms[1]) });
        edges.insert({ 1, 0, rooms[0].getHamiltonDist(rooms[1]) });
    }
    else if (rooms.size() > 2)
    {
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
    }
    return edges;
}

DungeonGenerationEngine::EdgeSet DungeonGenerationEngine::mst(const WeightedEdgeSet& edges)
{
    using iPair = std::pair<int, double>;

    if (edges.size() == 0)
        return {};

    std::set<int> nodes;
    for (const auto& e : edges)
    {
        nodes.insert(std::get<0>(e));
        nodes.insert(std::get<1>(e));
    }

    unsigned int numRooms = nodes.size();
    std::set<std::pair<int, int>> mst_edges;
    std::vector<std::vector<iPair>> adj;
    adj.resize(numRooms);
    for (const auto& e : edges)
    {
        int u, v;
        double w;
        std::tie(u, v, w) = e;

        adj[u].push_back({ v, w });
    }

    std::priority_queue<iPair, std::vector<iPair>, std::greater<iPair>> pq;
    int src = 0;
    std::vector<double> key(numRooms, std::numeric_limits<double>::max());
    std::vector<int> parent(numRooms, -1);
    std::vector<bool> inMst(numRooms, false);

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
    for (int i = 1; i < numRooms; i++)
        mst_edges.insert({ i, parent[i] });
    return mst_edges;
}

DungeonGenerationEngine::EdgeSet DungeonGenerationEngine::addSomeEdgesBack(
    unsigned int seed,
    const WeightedEdgeSet& edges,
    EdgeSet mst_edges,
    float addBackProb)
{
    using Edge = std::tuple<int, int, double>;

    std::default_random_engine generator(seed);
    std::uniform_real_distribution<float> choice_dist(0.0f, 1.0f);

    for (const auto& e : edges)
    {
        int na, nb;
        double w;
        std::tie(na, nb, w) = e;
        if (mst_edges.find({ na, nb }) == mst_edges.end()
            && mst_edges.find({ nb, na }) == mst_edges.end()
            && choice_dist(generator) < addBackProb)
        {
            mst_edges.insert({ na, nb });
        }
    }
    return mst_edges;
}

DungeonGenerationEngine::LineSet DungeonGenerationEngine::lineConnect(
    unsigned int seed,
    const RoomBoxVec& rooms, const EdgeSet& mst_edges,
    unsigned int overlapPadding, bool addBothDirection, float firstHorizontalProb)
{
    LineSet lines;

    std::default_random_engine generator(seed);
    std::uniform_real_distribution<float> choice_dist(0.0f, 1.0f);

    for (const auto& e : mst_edges)
    {
        const RoomBox& a = rooms[e.first];
        const RoomBox& b = rooms[e.second];
        if (abs(a.cx - b.cx) <= a.w / 2.0 + b.w / 2.0 - overlapPadding)
        {
            auto centerx = (std::max(a.x, b.x) + std::min(a.x + a.w, b.x + b.w)) / 2;
            lines.insert({ centerx, a.cy, centerx, b.cy });
        }
        else if (abs(a.cy - b.cy) <= a.h / 2.0 + b.h / 2.0 - overlapPadding)
        {
            auto centery = (std::max(a.y, b.y) + std::min(a.y + a.h, b.y + b.h)) / 2;
            lines.insert({ a.cx, centery, b.cx, centery });
        }
        else
        {
            if (addBothDirection)
            {
                lines.insert({ a.cx, a.cy, b.cx, a.cy });
                lines.insert({ b.cx, a.cy, b.cx, b.cy });
                lines.insert({ a.cx, a.cy, a.cx, b.cy });
                lines.insert({ a.cx, b.cy, b.cx, b.cy });
            }
            else
            {
                if (choice_dist(generator) < firstHorizontalProb)
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
    }
    return lines;
}

std::pair<DungeonGenerationEngine::RoomBoxVec, DungeonGenerationEngine::RoomBoxVec> DungeonGenerationEngine::selectCorridors(
    RoomBoxVec boxes, const LineSet& lines, unsigned int maxRoomSize)
{
    RoomBoxVec corridors;
    auto it = boxes.begin();
    while (it != boxes.end())
    {
        if (it->getSize() > maxRoomSize)
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
    return std::make_pair(boxes, corridors);
}

std::vector<int> DungeonGenerationEngine::tiling(
    const RoomBoxVec& rooms, const RoomBoxVec& corridors, const LineSet& lines,
    unsigned int mapWidth, unsigned int mapHeight)
{
    std::vector<int> tiles(mapWidth * mapHeight, 0);
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
            for (int x = std::max(0.0, floor(x1 - 0.5)); x < std::min((double)mapWidth, ceil(x2 + 0.5)); x++)
            {
                tiles[(int)std::max(0.0, floor(y1 - 0.5)) * mapWidth + x] = 1;
                tiles[(int)std::min((double)mapHeight, floor(y1 + 0.5)) * mapWidth + x] = 1;
            }
        }
        else if (x1 == x2)
        {
            if (y2 < y1)
                std::swap(y1, y2);
            for (int y = std::max(0.0, floor(y1 - 0.5)); y < std::min((double)mapHeight, ceil(y2 + 0.5)); y++)
            {
                tiles[y * mapWidth + (int)std::max(0.0, floor(x1 - 0.5))] = 1;
                tiles[y * mapWidth + (int)std::min((double)mapWidth, floor(x1 + 0.5))] = 1;
            }
        }
    }

    for (const auto& box : corridors)
        for (int y = box.y + halfH; y < box.y + box.h + halfH; y++)
            for (int x = box.x + halfW; x < box.x + box.w + halfW; x++)
                tiles[y * mapWidth + x] = 2;
    for (const auto& box : rooms)
        for (int y = box.y + halfH; y < box.y + box.h + halfH; y++)
            for (int x = box.x + halfW; x < box.x + box.w + halfW; x++)
                tiles[y * mapWidth + x] = 3;

    return tiles;
}
