#ifndef SATELLITE_TYPEDEFS_H_
#define SATELLITE_TYPEDEFS_H_

#include "inet/common/INETDefs.h"

namespace leolab {

using namespace inet;

class GeodeticPosition : public cObject {
  public:
    double longitude;  // 经度 (度)
    double latitude;   // 纬度 (度)  
    double altitude;   // 高度 (km)
    simtime_t timestamp;

    // 默认构造函数
    GeodeticPosition() : longitude(0), latitude(0), altitude(0), timestamp(0) {}
    
    // 带参数的构造函数
    GeodeticPosition(double lon, double lat, double alt) : 
        longitude(lon), latitude(lat), altitude(alt), timestamp(simTime()) {}
    
    virtual std::string str() const override {
        std::stringstream ss;
        ss << "lon=" << longitude << "°, lat=" << latitude 
           << "°, alt=" << altitude << "km";
        return ss.str();
    }
};

}

#endif