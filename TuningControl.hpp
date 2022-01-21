//
//  Microtonal.hpp
//  Electrobass
//
//  Created by Davis Polito on 1/20/22.
//  Copyright © 2022 Snyderphonics. All rights reserved.
//

#ifndef TuningControl_hpp
#define TuningControl_hpp

#include "MTS-ESP/Client/libMTSClient.h"
#include "Utilities.h"

#include <stdio.h>
/*
 * This class uses a member pointer. To avoid querying which mtof function to use within the process loop
 *  https://web.archive.org/web/20120723071248/http://www.dulcineatech.com/tips/code/c++/member-pointers.html
 */
class TuningControl
{
public:
    TuningControl()
    {
        setIsMTS(false);
    }
    ~TuningControl()
    {
        if(MTS_HasMaster(client))
            MTS_DeregisterClient(client);
    }
    void loadScala(std::string fname, float* arr);
    void setIsMTS(bool f)
    {
        isMTS = f;
        MTSOnOff();
        mtofptr = setMtoFFunction(isMTS);
    };
    float mtof (float mn);
    auto const getIsMTS() {return isMTS;};
private:
    typedef float (TuningControl::*MidiToFreq)(float);
    static MidiToFreq setMtoFFunction(bool);
    MidiToFreq mtofptr;
    float _MTS_mtof(float mn);
    float _mtof(float mn);
    MTSClient *client;
    bool isMTS;
    void MTSOnOff();
    

};
#endif /* TuningControl_hpp */
