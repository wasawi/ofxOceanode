//
//  phasorClass.cpp
//  DGTL_Generator
//
//  Created by Eduard Frigola on 28/07/16.
//
//

#include "basePhasor.h"


basePhasor::basePhasor(){
    phasor = vector<double>(1, 0);
    phasorMod = vector<double>(1, 0);
    momentaryPhasor = vector<double>(1, 0);
    timer.setPeriodicEvent(1000000);
    startThread();
    bpm_Param = 120.00;
    beatsMult_Param = vector<float>(1, 1);
    beatsDiv_Param = vector<float>(1, 1);;
    initPhase_Param = 0;
    loop_Param = true;
    numPhasors = 1;
}

basePhasor::~basePhasor(){
    stopThread();
    phasorToSend.close();
    waitForThread(true);
}

vector<float> basePhasor::getPhasors(){
    if(loop_Param){
        while(phasorToSend.tryReceive(momentaryPhasor));
        return vector<float>(momentaryPhasor.begin(), momentaryPhasor.end());
    }else{
        return vector<float>(numPhasors, initPhase_Param);
    }
}

void basePhasor::resetPhasor(){
    fill(phasor.begin(), phasor.end(), 0);
}

void basePhasor::threadedFunction(){
    while(isThreadRunning()){
        timer.waitNext();
        for(int i = 0; i < numPhasors; i++){
            //tue phasor that goes from 0 to 1 at desired frequency
            double freq = (double)bpm_Param/(double)60;
            freq = freq * (double)getValueForIndex(beatsMult_Param, i);
            freq = (double)freq / (double)getValueForIndex(beatsDiv_Param, i);
            double increment = (1.0f/(double)((double)(1000.0)/(double)freq));
            
            phasor[i] = phasor[i] + increment;
            if(i == 0 && phasor[0] >= 1){
                ofNotifyEvent(phasorCycle);
            }
            phasor[i] -= (int)phasor[i];
            
            //Assign a copy of the phasor to add initPhase
            phasorMod[i] = phasor[i];
            
            //take the initPhase_Param as a phase offset param
            phasorMod[i] += initPhase_Param;
            phasorMod[i] -= (int)phasorMod[i];
        }
        
        if(loop_Param){
            phasorToSend.send((phasorMod));
        }
    }
}

