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

#include "../wireless/DynamicChannel.h"

#include "inet/common/INETMath.h"
#include "inet/mobility/contract/IMobility.h"

namespace leolab {

using namespace omnetpp;
using namespace inet;
using namespace math;

Define_Channel(DynamicChannel);

const double EARTH_RADIUS_M = 6371000.0;

DynamicChannel::DynamicChannel(const char *name) : cDatarateChannel(name) {
}

DynamicChannel::~DynamicChannel() {
}

void DynamicChannel::initialize() {
    initParamerers();
    cDatarateChannel::initialize();
}

void DynamicChannel::initParamerers() {
    // 获取参数
    propagationSpeed = par("propagationSpeed");
    minUpdateInterval = par("minUpdateInterval");
    
    // 注册信号为“geodeticPositionChanged”
    geodeticPositionChangedSignal = cComponent::registerSignal("geodeticPositionChanged");

    // 获取连接的模块
    srcModule = getSourceGate()->getOwnerModule();
    destModule = getDestinationModule();
    
    // 订阅信号
    // 订阅源节点和目标节点的移动性模块
    cModule *srcMobility = getMobilityModule(srcModule);
    cModule *destMobility = getMobilityModule(destModule);
    
    // 添加空指针检查
    if (srcModule == nullptr) {
        EV_ERROR << "Source module is null!" << endl;
        return;
    }
    if (destModule == nullptr) {
        EV_ERROR << "Destination module is null!" << endl;
        return;
    }
    
    if (srcMobility) {
        if (srcModule->hasPar("initialLatitude") && srcModule->hasPar("initialLongitude") && srcModule->hasPar("initialAltitude")) {
            srcPosition.longitude = srcModule->par("initialLatitude");
            srcPosition.latitude = srcModule->par("initialLongitude");
            srcPosition.altitude = srcModule->par("initialAltitude");
            srcPosition.timestamp = simTime();
        }
        else {
            try {
                // 源节点订阅信号
                // ...
            }
            catch (const cRuntimeError& e) {
                EV_WARN << "cRuntimeError: Failed to subscribe to srcMobility " << srcMobility->getFullPath() << " Details: "<< e.what() << endl;
            }
            catch (const std::exception& e) {
                EV_WARN << "exception: Failed to subscribe to srcMobility " << srcMobility->getFullPath() << " Details: "<< e.what() << endl;
            }
            catch (...) {
                EV_WARN << "unknown: Failed to subscribe to srcMobility " << srcMobility->getFullPath() << endl;
            }
        }
    }
    else {
        throw cRuntimeError("The srcMobility is nullptr!");
    }
    
    if (destMobility) {
        if (destModule->hasPar("initialLatitude") && destModule->hasPar("initialLongitude") && destModule->hasPar("initialAltitude")) {
            destPosition.longitude = destModule->par("initialLatitude");
            destPosition.latitude = destModule->par("initialLongitude");
            destPosition.altitude = destModule->par("initialAltitude");
            destPosition.timestamp = simTime();
        }
        else {
            try {
                // 目的节点订阅信号
                // ...
            }
            catch (const cRuntimeError& e) {
                EV_WARN << "cRuntimeError: Failed to subscribe to srcMobility " << destMobility->getFullPath() << " Details: "<< e.what() << endl;
            }
            catch (const std::exception& e) {
                EV_WARN << "exception: Failed to subscribe to srcMobility " << destMobility->getFullPath() << " Details: "<< e.what() << endl;
            }
            catch (...) {
                EV_WARN << "unknown: Failed to subscribe to srcMobility " << destMobility->getFullPath() << endl;
            }
        }
    } 
    else {
        throw cRuntimeError("The destMobility is nullptr!");
    }
    
    EV_INFO << "DistanceBasedDelayChannel initialized: " << srcModule->getFullName() << " <-> " << destModule->getFullName() << endl;

}

void DynamicChannel::receiveSignal(cComponent *source, simsignal_t signalID, cObject *obj, cObject *details)
{
    // 处理经纬度变化信号
    if (signalID == geodeticPositionChangedSignal) {
        EV_DEBUG << "Received geodeticPositionChanged signal from " 
                 << source->getFullName() << endl;
        
        GeodeticPosition *geoPos = dynamic_cast<GeodeticPosition*>(obj);
        if (!geoPos) {
            EV_WARN << "Received geodetic signal but details is not GeodeticPosition" << endl;
            return;
        }
        
        // 确定是哪个节点的移动性模块
        cModule *nodeModule = getNodeFromMobility(dynamic_cast<cModule*>(source));
        if (!nodeModule) {
            EV_WARN << "Cannot find parent node for mobility module " 
                   << source->getFullName() << endl;
            return;
        }
        
        // 更新对应节点的位置缓存
        if (nodeModule == srcModule) {
            srcPosition.longitude = geoPos->longitude;
            srcPosition.latitude = geoPos->latitude;
            srcPosition.altitude = geoPos->altitude;
            srcPosition.timestamp = geoPos->timestamp;
            
            EV_DEBUG << "Updated source position: " << srcPosition.str() << endl;
        }
        else if (nodeModule == destModule) {
            destPosition.longitude = geoPos->longitude;
            destPosition.latitude = geoPos->latitude;
            destPosition.altitude = geoPos->altitude;
            destPosition.timestamp = geoPos->timestamp;
            
            EV_DEBUG << "Updated destination position: " << destPosition.str() << endl;
        }
        else {
            EV_WARN << "Received signal from unknown node: " << nodeModule->getFullName() << endl;
            return;
        }
        updateChannelDelay();
    }
}

DynamicChannel::Result DynamicChannel::processMessage(cMessage *msg, const SendOptions& options, simtime_t t) {

    updateChannelDelay();
    // 调用父类处理
    return cDatarateChannel::processMessage(msg, options, t);
}

void DynamicChannel::updateChannelDelay() {
    simtime_t currentTime = simTime();
    
    // 检查最小更新间隔
    if (lastUpdateTime >= 0 && (currentTime - lastUpdateTime) < minUpdateInterval) {
        return;
    }
    
    // 计算距离
    double distance = calculateDistance(srcPosition.longitude, srcPosition.latitude, srcPosition.altitude, destPosition.longitude, destPosition.latitude, destPosition.altitude);
    
    // 更新信道时延
    double newDelay = distance / propagationSpeed;
    setDelay(newDelay);

    // EV_WARN << "Succeed in setting delay: " << newDelay << endl;
    
    // 更新状态
    lastDistance = distance;
    lastUpdateTime = currentTime;
}

double DynamicChannel::calculateDistance(double lon1, double lat1, double alt1, double lon2, double lat2, double alt2) {
    // 将角度转换为弧度
    double lat1_rad = math::deg2rad(lat1);
    double lon1_rad = math::deg2rad(lon1);
    double lat2_rad = math::deg2rad(lat2);
    double lon2_rad = math::deg2rad(lon2);
    
    // 计算空间直线距离（考虑地球曲率和卫星高度）
    double r1 = EARTH_RADIUS_M + alt1 * 1000.0;  // 转换为米
    double r2 = EARTH_RADIUS_M + alt2 * 1000.0;
    
    // 球坐标转直角坐标
    double x1 = r1 * cos(lat1_rad) * cos(lon1_rad);
    double y1 = r1 * cos(lat1_rad) * sin(lon1_rad);
    double z1 = r1 * sin(lat1_rad);
    
    double x2 = r2 * cos(lat2_rad) * cos(lon2_rad);
    double y2 = r2 * cos(lat2_rad) * sin(lon2_rad);
    double z2 = r2 * sin(lat2_rad);
    
    // 欧几里得距离
    double distance = sqrt(pow(x2 - x1, 2) + pow(y2 - y1, 2) + pow(z2 - z1, 2));
    
    EV_TRACE << "Spherical distance calculated: " << distance / 1000.0 << " km" << endl;
    
    return distance;
}

cModule* DynamicChannel::getMobilityModule(cModule *module) {    // 添加空指针检查
    if (module == nullptr) {
        EV_ERROR << "getMobilityModule: module is null!" << endl;
        return nullptr;
    }
    // 查找移动性模块
    cModule *mobilityModule = module->getSubmodule("mobility");
    if (mobilityModule && dynamic_cast<IMobility*>(mobilityModule)) {
        return mobilityModule;
    }
    
    // 尝试在父模块中查找
    if (module->getParentModule()) {
        return getMobilityModule(module->getParentModule());
    }
    
    return nullptr;
}

cModule* DynamicChannel::getNodeFromMobility(cModule *mobilityModule) {
    if (!mobilityModule) {
        EV_WARN << "The mobilityModule is nullptr!" << endl;
        return nullptr;
    }
    
    // 移动性模块的直接父模块应该是节点
    cModule *nodeModule = mobilityModule->getParentModule();
    // EV_INFO << "The nodeModule is : " << nodeModule->getFullPath() << endl;
    // EV_INFO << "The srcModule is : " << srcModule->getFullPath() << endl;
    // EV_INFO << "The destModule is : " << destModule->getFullPath() << endl;
    if (nodeModule && (nodeModule == srcModule || nodeModule == destModule)) {
        return nodeModule;
    }
    
    // 如果不是直接父模块，向上查找
    while (nodeModule && nodeModule->getParentModule()) {
        nodeModule = nodeModule->getParentModule();
        if (nodeModule == srcModule || nodeModule == destModule) {
            return nodeModule;
        }
    }
    
    return nullptr;
}

cModule* DynamicChannel::getDestinationModule()
{
    // 通过源门获取连接路径，找到目的模块
    cGate *sourceGate = getSourceGate();
    if (!sourceGate) {
        EV_ERROR << "Source gate is null" << endl;
        return nullptr;
    }

    if (!sourceGate->isConnected()) {
        EV_ERROR << "Source gate is not connected!" << endl;
        return nullptr;
    }
    
    // 获取连接路径的终点门
    cGate *destGate = sourceGate->getPathEndGate();
    if (!destGate) {
        EV_ERROR << "Cannot find path end gate from source gate " << sourceGate->getFullPath() << endl;
        return nullptr;
    }
    
    // 获取目的模块
    cModule *destinationModule = destGate->getOwnerModule()->getParentModule()->getParentModule();
    if (!destinationModule) {
        EV_ERROR << "Cannot find owner module for destination gate " << destGate->getFullPath() << endl;
        return nullptr;
    }
    
    EV_DEBUG << "Found destination module: " << destinationModule->getFullPath() 
             << " via gate: " << destGate->getFullPath() << endl;
    
    return destinationModule;
}

double DynamicChannel::getNominalDatarate() const {
    return par("datarate").doubleValue();
}

}
