/*
  ==============================================================================

    DungeonGenerationEngine.h
    Created: 3 May 2022 7:38:36pm
    Author:  bowen

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <algorithm>
#include <random>
#include <numeric>
#include <set>
#include <queue>
#include <limits>

struct DungeonGenerationEngine
{
    struct RoomBox
    {
        RoomBox(double cx, double cy, double w, double h);
        bool operator<(const RoomBox& other) const;

        double getDistance() const;
        double getHamiltonDist(const RoomBox& other) const;
        double getSize() const;
        std::pair<double, double> getDirection(const RoomBox& fixed) const;

        bool isOverlap(const RoomBox& other) const;
        bool isTouching(const RoomBox& other) const;
        bool isTouchingLine(std::tuple<double, double, double, double> line);

        void snapToGrid();
        void moveDelta(double dx, double dy);
        void moveAwayFrom(const RoomBox& fixed, double dirx, double diry);

        double x, cx;
        double y, cy;
        double w;
        double h;
    };
    struct RoomBoxComp
    {
        constexpr bool operator()(const RoomBox& lhs, const RoomBox& rhs) const;
    };
    struct CustomTupleComp
    {
        constexpr bool operator()(const std::tuple<int, int, double>& lhs, const std::tuple<int, int, double>& rhs) const;
    };

    //==============================================================================

    using WeightedEdgeSet = std::set<std::tuple<int, int, double>, CustomTupleComp>;
    using EdgeSet = std::set<std::pair<int, int>>;
    using RoomBoxVec = std::vector<RoomBox>;
    using LineSet = std::set<std::tuple<double, double, double, double>>;

    RoomBoxVec randBox(
        unsigned int seed, bool useRectRegion, float radiusX, float radiusY,
        unsigned int numBox, unsigned int maxIteration, float smallBoxProb,
        bool smallBoxUseNormalDist, float smallBoxDistParamA, float smallBoxDistParamB, float smallBoxRatioLimit,
        bool largeBoxUseNormalDist, float largeBoxDistParamA, float largeBoxDistParamB, float largeBoxRatioLimit,
        float largeBoxRadiusMultiplier);
    RoomBoxVec separateBox(RoomBoxVec boxes);
    RoomBoxVec centerAndCropBox(RoomBoxVec boxes, unsigned int mapWidth, unsigned int mapHeight);
    std::pair<RoomBoxVec, RoomBoxVec> randSelect(RoomBoxVec boxes, unsigned int numRooms, bool allowTouching);
    WeightedEdgeSet triangulate(const RoomBoxVec& rooms);
    EdgeSet mst(const WeightedEdgeSet& edges);
    EdgeSet addSomeEdgesBack(
        unsigned int seed,
        const WeightedEdgeSet& edges,
        EdgeSet mst_edges,
        float addBackProb);
    LineSet lineConnect(
        unsigned int seed, const RoomBoxVec& rooms, const EdgeSet& mst_edges,
        unsigned int overlapPadding, bool addBothDirection, float firstHorizontalProb);
    std::pair<RoomBoxVec, RoomBoxVec> selectCorridors(
        RoomBoxVec boxes, const LineSet& lines,
        unsigned int maxRoomSize);
    std::vector<int> tiling(
        const RoomBoxVec& rooms, const RoomBoxVec& corridors, const LineSet& lines,
        unsigned int mapWidth, unsigned int mapHeight);
};
