//
//  ofxOceanodeConnection.h
//  example-basic
//
//  Created by Eduard Frigola Bagué on 22/02/2018.
//

#ifndef ofxOceanodeConnection_h
#define ofxOceanodeConnection_h

#include "ofMain.h"
#include "ofxOceanodeConnectionGraphics.h"

class ofxOceanodeAbstractConnection{
public:
    ofxOceanodeAbstractConnection(ofAbstractParameter& _sourceParameter, ofAbstractParameter& _sinkParameter){
        sourceParameter = &_sourceParameter;
        sinkParameter = &_sinkParameter;
        isPersistent = false;
        active = true;
    };
    
    ofxOceanodeAbstractConnection(ofAbstractParameter& _sourceParameter){
        sourceParameter = &_sourceParameter;
        sinkParameter = nullptr;
        isPersistent = false;
    };
    virtual ~ofxOceanodeAbstractConnection(){};
    
    void setSourcePosition(glm::vec2 posVec){
        graphics.setPoint(0, posVec);
    }
    
    void setSinkPosition(glm::vec2 posVec){
        graphics.setPoint(1, posVec);
    }
    
    void moveSourcePosition(glm::vec2 moveVec){
        graphics.movePoint(0, moveVec);
    }
    
    void moveSinkPosition(glm::vec2 moveVec){
        graphics.movePoint(1, moveVec);
    }
    
    void setActive(bool a){
        active = a;
    }
    
    ofxOceanodeConnectionGraphics& getGraphics(){return graphics;};
    
    glm::vec2 getPostion(int index){return graphics.getPoint(index);};
    void setTransformationMatrix(ofParameter<glm::mat4> *m){graphics.setTransformationMatrix(m);};
    
    ofAbstractParameter& getSourceParameter(){return *sourceParameter;};
    ofAbstractParameter& getSinkParameter(){return *sinkParameter;};
    
    bool getIsPersistent(){return isPersistent;};
    void setIsPersistent(bool p){isPersistent = p;graphics.setWireColor(ofColor(255,0,0));};
    
    ofEvent<void> destroyConnection;
protected:
    ofxOceanodeConnectionGraphics graphics;
    bool active;
    
    ofAbstractParameter* sourceParameter;
    ofAbstractParameter* sinkParameter;
    
private:
    bool isPersistent;
};

class ofxOceanodeNode;

class ofxOceanodeTemporalConnection: public ofxOceanodeAbstractConnection{
public:
    ofxOceanodeTemporalConnection(ofAbstractParameter& _p) : ofxOceanodeAbstractConnection(_p){
//        graphics.setPoint(0, glm::vec2(ofGetMouseX(), ofGetMouseY()));
//        graphics.setPoint(1, glm::vec2(ofGetMouseX(), ofGetMouseY()));
        
        mouseDraggedListener = ofEvents().mouseDragged.newListener([&](ofMouseEventArgs & mouse){
            graphics.setPoint(1, graphics.getTransformationMatrix() * glm::vec4(mouse, 0, 1));
        });
        mouseReleasedListener = ofEvents().mouseReleased.newListener([&](ofMouseEventArgs & mouse){
            graphics.deactivate();
            ofNotifyEvent(destroyConnection);
        });
    }
    ~ofxOceanodeTemporalConnection(){};
    
    glm::vec2 getSourcePosition(){return graphics.getPoint(0);};
    
private:
    ofEventListener mouseDraggedListener;
    ofEventListener mouseReleasedListener;
};

template<typename Tsource, typename Tsink, typename Enable = void>
class ofxOceanodeConnection: public ofxOceanodeAbstractConnection{
public:
    ofxOceanodeConnection(ofParameter<Tsource>& pSource, ofParameter<Tsink>& pSink) : ofxOceanodeAbstractConnection(pSource, pSink), sourceParameter(pSource), sinkParameter(pSink){
        beforeConnectionValue = sinkParameter.get();
        linkParameters();
    }
    ~ofxOceanodeConnection(){
        sinkParameter.set(beforeConnectionValue);
        ofNotifyEvent(destroyConnection);
    };
    
private:
    void linkParameters(){
        parameterEventListener = sourceParameter.newListener([&](Tsource &p){
            if(active)
                sinkParameter = sourceParameter;
        });
        //sinkParameter = sourceParameter;
    }
    ofEventListener parameterEventListener;
    ofParameter<Tsource>& sourceParameter;
    ofParameter<Tsink>&  sinkParameter;
    Tsink beforeConnectionValue;
};

template<typename _Tsource, typename _Tsink>
class ofxOceanodeConnection<vector<_Tsource>, vector<_Tsink>, typename std::enable_if<!std::is_same<_Tsource, _Tsink>::value>::type>: public ofxOceanodeAbstractConnection{
public:
    ofxOceanodeConnection(ofParameter<vector<_Tsource>>& pSource, ofParameter<vector<_Tsink>>& pSink) : ofxOceanodeAbstractConnection(pSource, pSink), sourceParameter(pSource), sinkParameter(pSink){
        beforeConnectionValue = sinkParameter.get();
        parameterEventListener = sourceParameter.newListener([&](vector<_Tsource> &vf){
            if(active){
                //            sinkParameter = vector<_Tsink>(1, f);
                vector<_Tsink> vec(vf.size());
                for(int i = 0; i < vf.size(); i ++){
                    vec[i] = vf[i];
                }
                sinkParameter = vec;
            }
        });
        //sinkParameter = vector<T>(1, sourceParameter);
    }
    ~ofxOceanodeConnection(){
        sinkParameter.set(beforeConnectionValue);
        ofNotifyEvent(destroyConnection);
    };
    
private:
    ofEventListener parameterEventListener;
    ofParameter<vector<_Tsource>>& sourceParameter;
    ofParameter<vector<_Tsink>>&  sinkParameter;
    vector<_Tsink> beforeConnectionValue;
};

template<typename>
struct is_std_vector : std::false_type {};

template<typename T, typename A>
struct is_std_vector<std::vector<T,A>> : std::true_type {};

template<typename _Tsource, typename _Tsink>
class ofxOceanodeConnection<_Tsource, vector<_Tsink>, typename std::enable_if<!is_std_vector<_Tsource>::value>::type>: public ofxOceanodeAbstractConnection{
public:
    ofxOceanodeConnection(ofParameter<_Tsource>& pSource, ofParameter<vector<_Tsink>>& pSink) : ofxOceanodeAbstractConnection(pSource, pSink), sourceParameter(pSource), sinkParameter(pSink){
        beforeConnectionValue = sinkParameter.get();
        parameterEventListener = sourceParameter.newListener([&](_Tsource &f){
            if(active)
                sinkParameter = vector<_Tsink>(1, f);
        });
        //sinkParameter = vector<T>(1, sourceParameter);
    }
    ~ofxOceanodeConnection(){
        sinkParameter.set(beforeConnectionValue);
        ofNotifyEvent(destroyConnection);
    };
    
private:
    ofEventListener parameterEventListener;
    ofParameter<_Tsource>& sourceParameter;
    ofParameter<vector<_Tsink>>&  sinkParameter;
    vector<_Tsink> beforeConnectionValue;
};

template<typename _Tsource, typename _Tsink>
class ofxOceanodeConnection<vector<_Tsource>, _Tsink, typename std::enable_if<!is_std_vector<_Tsink>::value>::type>: public ofxOceanodeAbstractConnection{
public:
    ofxOceanodeConnection(ofParameter<vector<_Tsource>>& pSource, ofParameter<_Tsink>& pSink) : ofxOceanodeAbstractConnection(pSource, pSink), sourceParameter(pSource), sinkParameter(pSink){
        beforeConnectionValue = sinkParameter.get();
        parameterEventListener = sourceParameter.newListener([&](vector<_Tsource> &vf){
            if(active){
                if(vf.size() > 0){
                    sinkParameter = vf[0];
                }
            }
        });
        //        if(sourceParameter.get().size() > 0){
        //            sinkParameter = sourceParameter.get()[0];
        //        }
    }
    ~ofxOceanodeConnection(){
        sinkParameter.set(beforeConnectionValue);
        ofNotifyEvent(destroyConnection);
    };
    
private:
    ofEventListener parameterEventListener;
    ofParameter<vector<_Tsource>>& sourceParameter;
    ofParameter<_Tsink>&  sinkParameter;
    _Tsink beforeConnectionValue;
};


template<typename T>
class ofxOceanodeConnection<void, T>: public ofxOceanodeAbstractConnection{
public:
    ofxOceanodeConnection(ofParameter<void>& pSource, ofParameter<T>& pSink) : ofxOceanodeAbstractConnection(pSource, pSink), sourceParameter(pSource), sinkParameter(pSink){
        linkParameters();
    }
    ~ofxOceanodeConnection(){
        ofNotifyEvent(destroyConnection);
    };
    
private:
    void linkParameters(){
        parameterEventListener = sourceParameter.newListener([&](){
            if(active)
                sinkParameter = sinkParameter;
        });
    }
    
    ofEventListener parameterEventListener;
    ofParameter<void>& sourceParameter;
    ofParameter<T>&  sinkParameter;
};

template<>
class ofxOceanodeConnection<void, void>: public ofxOceanodeAbstractConnection{
public:
    ofxOceanodeConnection(ofParameter<void>& pSource, ofParameter<void>& pSink) : ofxOceanodeAbstractConnection(pSource, pSink), sourceParameter(pSource), sinkParameter(pSink){
        linkParameters();
    }
    ~ofxOceanodeConnection(){
        ofNotifyEvent(destroyConnection);
    };
    
private:
    void linkParameters(){
        parameterEventListener = sourceParameter.newListener([&](){
            if(active)
                sinkParameter.trigger();
        });
    }
    
    ofEventListener parameterEventListener;
    ofParameter<void>& sourceParameter;
    ofParameter<void>&  sinkParameter;
};

template<>
class ofxOceanodeConnection<void, bool>: public ofxOceanodeAbstractConnection{
public:
    ofxOceanodeConnection(ofParameter<void>& pSource, ofParameter<bool>& pSink) : ofxOceanodeAbstractConnection(pSource, pSink), sourceParameter(pSource), sinkParameter(pSink){
        linkParameters();
    }
    ~ofxOceanodeConnection(){
        ofNotifyEvent(destroyConnection);
    };
    
private:
    void linkParameters(){
        parameterEventListener = sourceParameter.newListener([&](){
            if(active)
                sinkParameter = !sinkParameter;
        });
    }
    
    ofEventListener parameterEventListener;
    ofParameter<void>& sourceParameter;
    ofParameter<bool>&  sinkParameter;
};

template<>
class ofxOceanodeConnection<float, bool>: public ofxOceanodeAbstractConnection{
public:
    ofxOceanodeConnection(ofParameter<float>& pSource, ofParameter<bool>& pSink) : ofxOceanodeAbstractConnection(pSource, pSink), sourceParameter(pSource), sinkParameter(pSink){
        linkParameters();
    }
    ~ofxOceanodeConnection(){
        ofNotifyEvent(destroyConnection);
    };
    
private:
    void linkParameters(){
        parameterEventListener = sourceParameter.newListener([&](float &f){
            if(active){
                bool newValue = (f > ((sourceParameter.getMax() - sourceParameter.getMin())/2.0 + sourceParameter.getMin())) ? true : false;
                if(newValue != sinkParameter) sinkParameter = newValue;
            }
        });
    }
    
    ofEventListener parameterEventListener;
    ofParameter<float>& sourceParameter;
    ofParameter<bool>&  sinkParameter;
};



#endif /* ofxOceanodeConnection_h */
