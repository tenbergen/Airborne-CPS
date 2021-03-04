#pragma once

#include "XPLMDefs.h"
#include "XPLMDisplay.h"
#include "XPLMNavigation.h"
#include "XPLMDataAccess.h"
#include "XPLMMenus.h"
#include <cassert>
#include <cstring>
#include <math.h>

// @author Bobby Sgroi
class Autopilot {
public:
    std::string usersMac;
    std::string intruderMac;
    XPLMDataRef theta, psi, phi;
    XPLMDataRef overrideEngine;
    XPLMDataRef jsRoll, jsPitch, engineThrottle, overrideRoll;


    float pitch;
    double deciderV;
    Sense vSense;
    Autopilot(Decider *d) {
        theta = XPLMFindDataRef("sim/flightmodel/position/theta");
        phi = XPLMFindDataRef("sim/flightmodel/position/phi");
        psi = XPLMFindDataRef("sim/flightmodel/position/psi");
        jsRoll = XPLMFindDataRef("sim/joystick/yoke_roll_ratio");
        jsPitch = XPLMFindDataRef("sim/joystick/yoke_pitch_ratio");
        overrideRoll = XPLMFindDataRef("sim/operation/override/override_joystick_roll");
        engineThrottle = XPLMFindDataRef("sim/flightmodel/engine/ENGN_thro");
        overrideEngine = XPLMFindDataRef("sim/operation/override/override_throttles");
        deciderV = d->getVBuff();
    }

    void getPosition() {
        std::string s = "Theta(Pitch): ["+ std::to_string(XPLMGetDataf(theta)) + "] - Phi (Roll): [" + std::to_string(XPLMGetDataf(phi)) + "] Psi(Heading): [" + std::to_string(XPLMGetDataf(psi)) + "] \n";
        XPLMDebugString(s.c_str());
    }



    void neutralizeRoll() {
        float p = XPLMGetDataf(phi);
        float newRatio;
        if (p > 5.0f && p < 15.0f) {
            newRatio = -0.3;
        }
        if (p > 15.0f && p < 30.0f) {
            newRatio = -0.45;
        }
        else if (p > 30.0f && p < 45.0f) {
            newRatio = -0.6;
        }
        else if (p > 45 && p < 60) {
            newRatio = -0.9;
        }
        else if (p > 60.0f) {
            newRatio = -0.6;
        }
        else if (p <-5.0f && p>-15.0f) {
            newRatio = .3;
        }
        else if (p<-15.0f && p >-30.0) {
            newRatio = 0.45;
        }
        else if (p<-30.0f && p> -45.0f) {
            newRatio = 0.6;
        }
        else if (p<-45.0f && p> -60.0f) {
            newRatio = 0.75f;
        }
        else if (p < -60.0f) {
            newRatio = 0.9f;
        }
        else if (p < 5.0f && p>2.5f) {
            newRatio = -.1f;
        }
        else if (p > -5.0f && p < -2.5f) {
            newRatio = .1f;
        }
        else if (p <2.5f && p>-2.5f) {
            newRatio = 0.0f;
        }

        XPLMSetDataf(jsRoll, newRatio);
        float r = XPLMGetDataf(jsRoll);
        std::string jsRollStringPostChange = std::to_string(r)+"\n";
        XPLMDebugString(jsRollStringPostChange.c_str());
   }

    void adjustPitch() {
        float t = XPLMGetDataf(theta);

    }

    void adjustThrottle() {
        float EngineVals[8];
        EngineVals[0] = 1;
        XPLMSetDatavf(engineThrottle, EngineVals, 0, 8);
    }

    //What does decider vBuff return?
    //what does decider vBuff return if there is no RA?
    void apDecider() {
        std::string vBuffString ="vBuff from decider: "+ std::to_string(deciderV) + "\n";
        XPLMDebugString(vBuffString.c_str());
        //if (vbuff != null) {
        //    if (vsense == sense::upward) {

        //    }
        //    if (vsense == sense::downward){

        //    }

        // }
    }
    
    //sim/joystick/has_joystick boolean might be useful for checking joystck use
    //sim/joystick/
    //Might have to use override pitch for joystick /yoke_pitch_ratio and /yoke_roll_ratio

    //used for completely overriding roll joystick axis. Locks user's joystick. Not needed in implementation, helpful for testing.
    bool jsRollOverideSwitch() {
        int i = XPLMGetDatai(overrideRoll);
        (i == 0) ? (i = 1) : (i = 0);
        XPLMSetDatai(overrideRoll, i);
        return i;

    }

    bool engineOverrideSwitch() {
        int i = XPLMGetDatai(overrideEngine);
        (i == 0) ? (i = 1) : (i = 0);
        XPLMSetDatai(overrideEngine, i);
        return i;

    }
   
};