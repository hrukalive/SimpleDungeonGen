#pragma once

#include <JuceHeader.h>
#include "DungeonGenerationEngine.h"

//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class MainComponent  : public juce::Component, public juce::ValueTree::Listener
{
    class CanvasOverlayComponent : public juce::Component
    {
    public:
		CanvasOverlayComponent(MainComponent* parent, juce::ValueTree& state);
		void paint(juce::Graphics& g) override;
		void resized() override;

        void mouseDown(const juce::MouseEvent& e) override;
        void mouseDrag(const juce::MouseEvent& e) override;
        void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;
		
    private:
        using WeightedEdgeSet = std::set<std::tuple<int, int, double>, DungeonGenerationEngine::CustomTupleComp>;
        using EdgeSet = std::set<std::pair<int, int>>;
        using RoomBoxVec = std::vector<DungeonGenerationEngine::RoomBox>;
        using LineSet = std::set<std::tuple<double, double, double, double>>;
		
		MainComponent* parent;
		
    public:
        //unsigned int mapWidth = 64, mapHeight = 64;
        juce::ValueTree generalState;
        juce::ValueTree randGenState;

        RoomBoxVec boxes;
        RoomBoxVec rooms;
        RoomBoxVec corridors;
        WeightedEdgeSet edges;
        EdgeSet mst_edges;
        LineSet lines;
        std::vector<int> tiles;
        bool generated{ false };

        bool tileColor{ true };

        float fScale{ 10.f };
        int scale{ 10 };
        int viewDx = 0, viewDy = 0;

        int prevMouseX, prevMouseY;
    };

    class CustomSliderPropertyComponent : public juce::SliderPropertyComponent
    {
    public:
        CustomSliderPropertyComponent(const juce::Value& valueToControl,
            const juce::String& propertyName,
            double rangeMin,
            double rangeMax,
            double interval,
            double skewFactor = 1.0,
            bool symmetricSkew = false);
    };
	
public:
    //==============================================================================
    MainComponent();
    ~MainComponent() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

    //==============================================================================
    void valueTreePropertyChanged(juce::ValueTree& treeWhosePropertyHasChanged, const juce::Identifier& property) override;

    //==============================================================================
    void runPipeline(int step);

private:
    //==============================================================================
    // Your private member variables go here...
    juce::TextButton btnRandBox{ "RandBoxes" }, btnSep{ "Separate" }, btnCenter{ "Center&Crop" };
    juce::TextButton btnSelect{ "RandSelect" }, btnTri{ "Triangulate" }, btnMst{ "MST" }, btnAddBack{ "AddBack" };
    juce::TextButton btnLine{ "LineConnect" }, btnCorridor{ "Corridor" }, btnTile{ "TileIt" };

    std::vector<juce::TextButton*> stepBtns{ &btnRandBox, &btnSep, &btnCenter, &btnSelect, &btnTri, &btnMst, &btnAddBack, &btnLine, &btnCorridor, &btnTile };
    int lastStep = -1;

    juce::TextButton btnResetView{ "ResetView" }, btnPipeline{"RunPipelineRandomSeed"};

    juce::ToggleButton btnTileColor{ "ColorTypes" };

    DungeonGenerationEngine engine;

    juce::ValueTree state{ "ROOT" };
	
    juce::PropertyPanel propertyPanel{ "Generation Parameters" };
    std::unique_ptr<CanvasOverlayComponent> canvasComp;

    juce::StretchableLayoutManager layout;
    juce::StretchableLayoutResizerBar layoutResizer{ &layout, 1, true };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
