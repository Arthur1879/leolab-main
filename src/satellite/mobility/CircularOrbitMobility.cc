//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

#include "CircularOrbitMobility.h"
#include "inet/common/INETMath.h"

namespace leolab {

Define_Module(CircularOrbitMobility);

using namespace inet;
using namespace math;

const double EARTH_RADIUS_KM = 6371.0;
const double GM = 3.986004418e5;

CircularOrbitMobility::CircularOrbitMobility() {
    initPhase = 0;
    alpha = 0;
    altitude = 0;
    rightAscension = 0;
    earthRotationRate = 0;

    phase = 0;
    omega = 0;
    longitude = 0;
    latitude = 0;

    currentGeoPos = new GeodeticPosition();
}

CircularOrbitMobility::~CircularOrbitMobility() {
    // 清理分配的对象
    if (currentGeoPos) {
        delete currentGeoPos;
        currentGeoPos = nullptr;
    }
}

void CircularOrbitMobility::initialize(int stage) {

    MovingMobilityBase::initialize(stage);

    EV_TRACE << "initializing CircularOrbitMobility stage " << stage << endl;

    if (stage == INITSTAGE_LOCAL) {
        // 注册信号为“geodeticPositionChanged”
        // geodeticPositionChangedSignal = cComponent::registerSignal("geodeticPositionChanged");
        initParamerers();
    }

}

void CircularOrbitMobility::initParamerers() {
    // 注册信号为“geodeticPositionChanged”
    geodeticPositionChangedSignal = cComponent::registerSignal("geodeticPositionChanged");

    initPhase = par("initPhase").doubleValue();
    alpha = par("alpha").doubleValue();
    altitude = par("altitude").doubleValue();
    rightAscension = par("rightAscension").doubleValue();
    earthRotationRate = par("earthRotationRate").doubleValue();

    double radius = EARTH_RADIUS_KM + altitude;
    double period = 2 * M_PI * sqrt(pow(radius, 3) / GM);
    omega = 2 * M_PI / period;

    constraintAreaCenter = Coord((constraintAreaMax.x + constraintAreaMin.x) / 2, 
                                (constraintAreaMax.y + constraintAreaMin.y) / 2, 
                                (constraintAreaMax.z + constraintAreaMin.z) / 2);
                                
    move();
}

void CircularOrbitMobility::setInitialPosition() {
    move();
}

void CircularOrbitMobility::move() {
    // 父类周期性调用move()，每次调用时根据仿真时间计算新的经纬度位置，并更新lastPosition
    // 基于仿真时间计算相位
    // ...
    while(phase > 2 * M_PI) phase -= 2 * M_PI;  // 避免phase无限累积

    // 计算经度
    // ...
    while (longitude >  2 * M_PI)  longitude -= 2 * M_PI;   // 避免longitude无限累积

    // 计算纬度
    // ...

    // 将经纬度映射至2D平面
    // lastPosition.x = constraintAreaCenter.x + longitude * (constraintAreaMax.x - constraintAreaMin.x) / (2 * M_PI);
    // lastPosition.y = -(constraintAreaCenter.y + latitude * (constraintAreaMax.y - constraintAreaMin.y) / M_PI);
    // lastPosition.z = altitude;

    // 边界处理，WRAP代表遇到边界从另一边出来
    Coord dummyCoord;
    handleIfOutside(WRAP, dummyCoord, dummyCoord);

    // 更新经纬度信息对象
    // currentGeoPos->longitude = rad2deg(longitude);
    // currentGeoPos->latitude = rad2deg(latitude);
    // currentGeoPos->altitude = altitude;
    // currentGeoPos->timestamp = simTime();
    
    // 发射包含经纬度对象的信号
    // ...
    

}

const GeodeticPosition* CircularOrbitMobility::getCurrentGeoPos() {
    return currentGeoPos;
}

}

