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
    XPLMDataRef vv, plugin_fnrml, plugin_faxil, fnrml_aero, faxil_aero, elv_trim;


    float pitch;
    double deciderV;
    Decider * dec;
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
        vv = XPLMFindDataRef("sim/flightmodel/position/vh_ind_fpm");
        plugin_fnrml = XPLMFindDataRef("sim/flightmodel/forces/fnrml_plug_acf");
        plugin_faxil = XPLMFindDataRef("sim/flightmodel/forces/faxil_plug_acf");
        fnrml_aero = XPLMFindDataRef("sim/flightmodel/forces/fnrml_aero");
        faxil_aero = XPLMFindDataRef("sim/flightmodel/forces/faxil_aero");
        elv_trim = XPLMFindDataRef("sim/flightmodel/controls/elv_trim");

        dec = new Decider();
        dec = d;
    }

    void getPosition() {
        std::string s = "Theta(Pitch): ["+ std::to_string(XPLMGetDataf(theta)) + "] - Phi (Roll): [" + std::to_string(XPLMGetDataf(phi)) + "] Psi(Heading): [" + std::to_string(XPLMGetDataf(psi)) + "] \n";
        XPLMDebugString(s.c_str());
    }



    void neutralizeRoll() {
        float p = XPLMGetDataf(phi);
        float newRatio;

        newRatio = (p / 90 * -3);
        //if (p > 5.0f && p < 15.0f) {
        //    newRatio = -0.4f;
        //}
        //if (p > 15.0f && p < 30.0f) {
        //    newRatio = -0.55f;
        //}
        //else if (p > 30.0f && p < 45.0f) {
        //    newRatio = -0.7f;
        //}
        //else if (p > 45 && p < 60) {
        //    newRatio = -0.75f;
        //}
        //else if (p > 60.0f) {
        //    newRatio = -0.9f;
        //}
        //else if (p <-5.0f && p>-15.0f) {
        //    newRatio = .4f;
        //}
        //else if (p<-15.0f && p >-30.0) {
        //    newRatio = 0.55f;
        //}
        //else if (p<-30.0f && p> -45.0f) {
        //    newRatio = 0.7f;
        //}
        //else if (p<-45.0f && p> -60.0f) {
        //    newRatio = 0.75f;
        //}
        //else if (p < -60.0f) {
        //    newRatio = 0.9f;
        //}
        //else if (p < 5.0f && p>2.0f) {
        //    newRatio = -.1f;
        //}
        //else if (p > -5.0f && p < -2.0f) {
        //    newRatio = .1f;
        //}
        //else if (p <2.0f && p>-2.0f) {
        //    newRatio = 0.0f;
        //}

        XPLMSetDataf(jsRoll, newRatio);
        float r = XPLMGetDataf(jsRoll);
        
        std::string testString = "Current Phi: " + std::to_string(p) + ". Current Roll Ratio: " + std::to_string(r) + ".\n";
        XPLMDebugString(testString.c_str());
   }

    void adjustPitchUp(float desired) {
        float t = XPLMGetDataf(theta);
        float vertvel = XPLMGetDataf(vv);
        float newRatio1;
        if (desired > 1000.0f) {
            newRatio1 = 0.9f;
        }
        else if (desired > 500.0f && desired < 1000.0f) {
            newRatio1 = 0.60f;

        }
        else if (desired > 250.0f && desired < 500.0f) {
            newRatio1 = 0.30f;
        }
        else if (desired < 250.0f) {
            newRatio1 = 0.0f;
        }

        adjustThrottle(1.0f);
        XPLMSetDataf(elv_trim, 0);
        XPLMSetDataf(jsPitch, newRatio1);
    }
    
    void adjustThrottle(float power) {
        float EngineVals[8];
        EngineVals[0] = power;
        XPLMSetDatavf(engineThrottle, EngineVals, 0, 8);
    }

    //what does decider vBuff return if there is no RA?
    double testVBuff = 1500.0f;
    //replaced dec->getVBuff() with testVBuff
    void apDecider() {
        if (testVBuff != NULL && testVBuff!= 0.000000) {
            //Get the Desired Vertical Max Ceiling from Decider 
            double deciderV = testVBuff;
            std::string vBuffString = "vBuff from decider: " + std::to_string(deciderV) + "\n";
            XPLMDebugString(vBuffString.c_str());

            float vertvel = XPLMGetDataf(vv);
            std::string vertChangeString = "Vertical Change: " + std::to_string(deciderV-vertvel) + "\n";
            double vertChange = deciderV - vertvel;
            XPLMDebugString(vertChangeString.c_str());
            adjustPitchUp(vertChange);

            //addForce(vertvel, deciderV, vertChange);

            



            //double deciderV = dec->getVBuff();
            //std::string vBuffString = "vBuff from decider: " + std::to_string(deciderV) + "\n";
            //XPLMDebugString(vBuffString.c_str());
            //vSense = dec->getSense();
            //if (vSense == Sense::UPWARD) {
            //    std::string senseString = "Sense = Upward\n";
            //    XPLMDebugString(senseString.c_str());
            //}
            //if (vSense == Sense::DOWNWARD) {
            //    std::string senseString = "Sense = Downward\n";
            //    XPLMDebugString(senseString.c_str());
            //}
            //float vertchange;
            //float vertvel = XPLMGetDataf(vv);
            ////if (vSense == Sense::UPWARD) {
            ////    vertchange = deciderV - vertvel;
            ////}
            ////else if (vSense == Sense::DOWNWARD) {
            ////    vertchange = vertvel - deciderV;
            ////}
            //(vSense == Sense::UPWARD) ? (vertchange = deciderV - vertvel) : (vertchange = vertvel - deciderV);
            ////(vSense == Sense::DOWNWARD) ? (vertchange = deciderV + vertvel) :
            //adjustPitch(vSense, vertchange);

        }

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

    //When activated it will add arbitrary force to the plane's y and z coordinates.
    //Forces are added in m/s
   
};