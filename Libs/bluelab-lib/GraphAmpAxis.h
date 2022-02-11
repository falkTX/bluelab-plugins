//
//  GraphAmpAxis.h
//  BL-InfrasonicViewer
//
//  Created by applematuer on 11/22/19.
//
//

#ifndef __BL_InfrasonicViewer__GraphAmpAxis__
#define __BL_InfrasonicViewer__GraphAmpAxis__

// GraphAmpAxis: from GraphFreqAxis
// For GraphControl12
//
class GraphAxis2;
class GUIHelper12;
class GraphAmpAxis
{
public:
    enum Density
    {
        DENSITY_20DB = 0,
        DENSITY_10DB,
    };
    
    GraphAmpAxis(bool displayLines = true, Density density = DENSITY_20DB);
    
    virtual ~GraphAmpAxis();
    
    void Init(GraphAxis2 *graphAxis,
              GUIHelper12 *guiHelper,
              BL_FLOAT minDB, BL_FLOAT maxDB,
              int graphWidth);
    
    void Reset(BL_FLOAT minDB, BL_FLOAT maxDB);
    
protected:
    void Update();
    
    void UpdateDensity20dB();
    void UpdateDensity10dB();
    
    //
    GraphAxis2 *mGraphAxis;
    
    BL_FLOAT mMinDB;
    BL_FLOAT mMaxDB;
    
    bool mDisplayLines;
    
    Density mDensity;
};

#endif /* defined(__BL_InfrasonicViewer__GraphAmpAxis__) */
