#pragma once

#include <JuceHeader.h>
#include <algorithm>
#include <random>
#include <numeric>
#include <set>
#include <queue>
#include <limits>

struct RoomBox
{
	RoomBox(double cx, double cy, double w, double h)
		: cx(cx), cy(cy), w(w), h(h)
	{
        x = cx - w / 2.0;
        y = cy - h / 2.0;
	}

    void snapToGrid()
    {
        x = (x < 0) ? floor(x) : ceil(x);
        y = (y < 0) ? floor(y) : ceil(y);
        cx = x + w / 2.0;
        cy = y + h / 2.0;
    }

    void moveDelta(double dx, double dy)
    {
        x += dx;
        y += dy;
        cx = x + w / 2.0;
        cy = y + h / 2.0;
    }

    bool operator<(const RoomBox& other) const
	{
		return getDistance() < other.getDistance();
	}

    double getDistance() const
    {
		return sqrt(cx * cx + cy * cy);
    }

    double getHamiltonDist(const RoomBox& other) const
    {
        return abs(cx - other.cx) + abs(cy - other.cy);
    }
	
    double getSize() const
    {
        return w * h;
    }
	
    bool isOverlap(const RoomBox& other) const
    {
		return abs(cx - other.cx) < w / 2 + other.w / 2 && abs(cy - other.cy) < h / 2 + other.h / 2;
    }

    bool isTouching(const RoomBox& other) const
    {
        return abs(cx - other.cx) <= w / 2 + other.w / 2 && abs(cy - other.cy) <= h / 2 + other.h / 2;
    }

    bool isTouchingLine(std::tuple<double, double, double, double> line)
    {
        double x1, y1, x2, y2;
        std::tie(x1, y1, x2, y2) = line;

        return
            (y1 == y2 && abs(y1 - cy) <= h / 2.0 && ((x1 <= cx && x2 >= cx || x1 >= cx && x2 <= cx) || abs(x1 - cx) < w / 2.0 || abs(x2 - cx) < w / 2.0)) ||
            (x1 == x2 && abs(x1 - cx) <= w / 2.0 && ((y1 <= cy && y2 >= cy || y1 >= cy && y2 <= cy) || abs(y1 - cy) < h / 2.0 || abs(y2 - cy) < h / 2.0));
    }
	
    std::pair<double, double> getDirection(const RoomBox& fixed) const
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

    void moveAwayFrom(const RoomBox& fixed, double dirx, double diry)
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
	
    double x, cx;
    double y, cy;
    double w;
    double h;

    int selectionStatus{ 0 };
};

struct RoomBoxComp
{
    constexpr bool operator()(const RoomBox& lhs, const RoomBox& rhs) const
    {
        return std::tie(lhs.cx, lhs.cy) < std::tie(rhs.cx, rhs.cy);
    }
};
struct CustomTupleComp
{
    constexpr bool operator()(const std::tuple<int, int, double>& lhs, const std::tuple<int, int, double>& rhs) const
    {
        return std::tie(std::get<0>(lhs), std::get<1>(lhs)) < std::tie(std::get<0>(rhs), std::get<1>(rhs));
    }
};

//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class MainComponent  : public juce::Component
{
public:
    //==============================================================================
    MainComponent();
    ~MainComponent() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

    //==============================================================================
    void randBox();
    void separateBox();
    void centerAndCropBox();
    void randSelect();
    void triangulate();
    void mst();
    void addBack();
    void lineConnect();
    void tiling();

private:
    //==============================================================================
    // Your private member variables go here...
    juce::TextButton btnRandBox{ "RandBoxes" }, btnSep{ "Separate" }, btnCenter{ "Center&Crop" };
    juce::TextButton btnSelect{ "RandSelect" }, btnTri{ "Triangulate" }, btnMst{ "MST" }, btnAddBack{ "AddBack" };
    juce::TextButton btnLine{ "LineConnect" }, btnPipeline{ "RunUntilHere" }, btnFinal{ "TileIt" };
    juce::TextButton btnZoomIn{ "ZoomIn" }, btnZoomOut{ "ZoomOut" };

    std::vector<RoomBox> rooms;
    std::vector<RoomBox> corridors;
    std::vector<RoomBox> boxes;
    std::set<std::tuple<int, int, double>, CustomTupleComp> edges;
    std::set<std::pair<int, int>> mstEdges;

    std::set<std::tuple<double, double, double, double>> lines;

    std::vector<int> tiles;
    bool generated{ false };

    int scale{ 10 };
    int mapWidth{ 64 };
    int mapHeight{ 64 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
