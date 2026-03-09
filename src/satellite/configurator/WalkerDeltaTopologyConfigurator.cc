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

#include "WalkerDeltaTopologyConfigurator.h"

namespace leolab {
    
using namespace math;

Define_Module(WalkerDeltaTopologyConfigurator);

const double EARTH_RADIUS_M = 6371000.0;

WalkerDeltaTopologyConfigurator::WalkerDeltaTopologyConfigurator() {}

WalkerDeltaTopologyConfigurator::~WalkerDeltaTopologyConfigurator() {}

void WalkerDeltaTopologyConfigurator::initialize(int stage) {
    
    if (stage == INITSTAGE_LOCAL) {
        network = getParentModule();
        if (!network) {
            throw cRuntimeError("");
        }
        satelliteModuleName = par("satelliteModuleName").stdstringValue();
        groundHostModuleName = par("groundHostModuleName").stdstringValue();
        updateInterval = par("updateInterval").doubleValue();
        initRightAscension = network->par("initRightAscension").doubleValue();
        initPhase = network->par("initPhase").doubleValue();
        altitude = network->par("altitude").doubleValue();
        alpha = network->par("alpha").doubleValue();
        numSatellites = network->par("numSatellites").intValue();
        numPlane = network->par("numPlane").intValue();
        F = network->par("F").intValue();
        numGroundHosts = network->par("numGroundHosts").intValue();
        
        initSatellitePosition();
        createFourLinks();
        updateGroundToSatelliteLinks();

        updateTimer = new cMessage("updateTimer");
        scheduleAt(simTime() + updateInterval, updateTimer);
    }
}

void WalkerDeltaTopologyConfigurator::handleMessage(cMessage *msg) {
    if (msg == updateTimer) {
        updateGroundToSatelliteLinks();
        scheduleAt(simTime() + updateInterval, updateTimer);
    }

}


void WalkerDeltaTopologyConfigurator::updateGroundToSatelliteLinks() {
    int N = numPlane, M = numSatellites / numPlane;
    
    EV_INFO << "=== Updating ground to satellite links ===" << endl;
    EV_INFO << "Number of terminals: " << numGroundHosts << ", satellites: " << numSatellites << endl;
    
    for (int i = 0; i < numGroundHosts; ++i) {
        cModule* terminalModule = network->getSubmodule(groundHostModuleName.c_str(), i);
        
        // 检查终端模块是否存在
        if (!terminalModule) {
            EV_ERROR << "Terminal module " << groundHostModuleName << "[" << i << "] not found!" << endl;
            continue;
        }
        
        EV_DEBUG << "Processing terminal [" << i << "]: " << terminalModule->getFullPath() << endl;
        
        // 获取终端gate
        cGate* terminalGate = terminalModule->gate("ethg$o", 0);
        if (!terminalGate) {
            EV_ERROR << "Terminal [" << i << "] gate ethg$o[0] not found!" << endl;
            continue;
        }
        
        int currentIdx = -1;
        double currentDistance = DBL_MAX;
        bool isUpdate = false;
        
        // 检查gate是否连接
        if (terminalGate->isConnected()) {
            // 获取连接的gate
            cGate* connectedGate = terminalGate->getNextGate();
            if (connectedGate) {
                cModule* satelliteModule = connectedGate->getOwnerModule();
                if (satelliteModule) {
                    currentIdx = satelliteModule->getIndex();
                    currentDistance = calculateDistance(terminalModule, satelliteModule);
                    EV_DEBUG << "Terminal [" << i << "] currently connected to satellite [" << currentIdx 
                             << "], distance: " << currentDistance << endl;
                } else {
                    EV_WARN << "Connected satellite module is null for terminal [" << i << "]" << endl;
                }
            } else {
                EV_WARN << "Connected gate is null for terminal [" << i << "]" << endl;
            }
        } 
        
        for (int k = 0; k < numSatellites; ++k) {
            cModule* satelliteModule = network->getSubmodule(satelliteModuleName.c_str(), k);
            if (!satelliteModule) continue;
            
            cGate* satelliteGate = satelliteModule->gate("ethg$i", 4);
            if (!satelliteGate) continue;
            
            if (!satelliteGate->isConnected()) {
                double distance = calculateDistance(terminalModule, satelliteModule);
                if (currentDistance > distance) {
                    currentIdx = k;
                    currentDistance = calculateDistance(terminalModule, satelliteModule);
                }
            }
        }
        
        EV_INFO << "Terminal [" << i << "] final connection: satellite [" << currentIdx 
                << "], distance is " << currentDistance / 1000 << "km." << endl;

        // 获取目标卫星模块
        cModule* targetSatellite = network->getSubmodule(satelliteModuleName.c_str(), currentIdx);
        if (!targetSatellite) {
            EV_ERROR << "Target satellite [" << currentIdx << "] not found for terminal [" << i << "]" << endl;
            continue;
        }
        
        cGate* targetSatelliteInputGate = targetSatellite->gate("ethg$i", 4);
        cGate* targetSatelliteOutputGate = targetSatellite->gate("ethg$o", 4);
        cGate* terminalInputGate = terminalModule->gate("ethg$i", 0);
        
        if (!targetSatelliteInputGate || !targetSatelliteOutputGate || !terminalInputGate) {
            EV_ERROR << "Required gates not found for terminal [" << i << "] and satellite [" << currentIdx << "]" << endl;
            continue;
        }

        // 处理连接更新
        if (terminalGate->isConnected()) {
            cGate* connectedGate = terminalGate->getNextGate();
            if (connectedGate) {
                cModule* currentSatellite = connectedGate->getOwnerModule();
                connectedGate = currentSatellite->gate("ethg$o", 4);
                if (currentSatellite && currentSatellite->getIndex() != currentIdx) {
                    EV_INFO << "Updating connection for terminal [" << i << "]: from satellite [" 
                            << currentSatellite->getIndex() << "] to [" << currentIdx << "]" << endl;
                    
                    // 断开旧连接
                    terminalGate->disconnect();
                    connectedGate->disconnect();
                    // 创建新连接
                    try {
                        // 双向链路
                        createDynamicChannel(terminalGate, targetSatelliteInputGate);
                        createDynamicChannel(targetSatelliteOutputGate, terminalInputGate);
                        EV_INFO << "Successfully created dynamic channels for terminal [" << i << "] and satellite [" << currentIdx << "]" << endl;
                    }
                    catch (const cRuntimeError& e) {
                        EV_ERROR << "Runtime error while creating dynamic channels for terminal [" << i 
                                << "] and satellite [" << currentIdx << "]: " << e.what() << endl;
                    }
                    catch (const std::exception& e) {
                        EV_ERROR << "Standard exception while creating dynamic channels for terminal [" << i 
                                << "] and satellite [" << currentIdx << "]: " << e.what() << endl;
                    }
                    catch (...) {
                        EV_ERROR << "Unknown exception while creating dynamic channels for terminal [" << i 
                                << "] and satellite [" << currentIdx << "]" << endl;
                    }
                } else {
                    EV_DEBUG << "Terminal [" << i << "] already connected to optimal satellite [" << currentIdx << "]" << endl;
                }
            }
        } else {
            EV_INFO << "Creating new connection for terminal [" << i << "] to satellite [" << currentIdx << "]" << endl;
            // 创建新连接
            try {
                createDynamicChannel(terminalGate, targetSatelliteInputGate);
                createDynamicChannel(targetSatelliteOutputGate, terminalInputGate);
                EV_INFO << "Successfully created dynamic channels for terminal [" << i << "] and satellite [" << currentIdx << "]" << endl;
            }
            catch (const cRuntimeError& e) {
                EV_ERROR << "Runtime error while creating dynamic channels for terminal [" << i 
                        << "] and satellite [" << currentIdx << "]: " << e.what() << endl;
                // 可以选择记录错误但继续执行，或者采取其他恢复措施
            }
            catch (const std::exception& e) {
                EV_ERROR << "Standard exception while creating dynamic channels for terminal [" << i 
                        << "] and satellite [" << currentIdx << "]: " << e.what() << endl;
            }
            catch (...) {
                EV_ERROR << "Unknown exception while creating dynamic channels for terminal [" << i 
                        << "] and satellite [" << currentIdx << "]" << endl;
            }
        }
    }
    
    EV_INFO << "=== Finished updating ground to satellite links ===" << endl;
}

void WalkerDeltaTopologyConfigurator::initSatellitePosition() {
    int N = numPlane, M = numSatellites / numPlane;
    if (N * M != numSatellites) {
        throw cRuntimeError("%d satellites cannot be divided into %d equal parts\n", numSatellites, numPlane);
    }
    double rightAscension;
    double phase;

    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < M; ++j) {
            int idx = getIdx(N, M, i, j);
            CircularOrbitMobility* satelliteMobility = dynamic_cast<CircularOrbitMobility *>(network->getSubmodule(satelliteModuleName.c_str(), idx)->getSubmodule("mobility"));
            
            // 计算每一个卫星的升交点赤经rightAscension和初始相位phase
            // ...
            rightAscension = 2 * M_PI / N * i + initRightAscension;
            phase = 2 * M_PI / M * (j + i * F / N) + initPhase;

            // 初始化卫星节点
            satelliteMobility->par("initPhase").setDoubleValue(phase);
            satelliteMobility->par("alpha").setDoubleValue(alpha);
            satelliteMobility->par("altitude").setDoubleValue(altitude);
            satelliteMobility->par("rightAscension").setDoubleValue(rightAscension);
            satelliteMobility->initParamerers();
        }
    }
}

void WalkerDeltaTopologyConfigurator::createFourLinks() {
    // 每颗卫星eth0、eth1、eth2、eth3接口依次连接上、右、下、左方向上的卫星
    // 相应的，被访问卫星对应的接口依次为eth2、eth3、eth0、eth1
    const int gatesIdx[] = {2, 3, 0, 1};
    int N = numPlane, M = numSatellites / numPlane;

    if (N * M != numSatellites) {
        throw cRuntimeError("%d satellites cannot be divided into %d equal parts\n", numSatellites, numPlane);
    }

    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < M; ++j) {
            int idx = getIdx(N, M, i, j);
            cModule* srcModule = network->getSubmodule(satelliteModuleName.c_str(), idx);
            cModule* destModule = nullptr;

            // 获取上、右、下、左方向卫星的索引
            int neighborsIdx[] = {
                getIdx(N, M, i, j + 1),
                getIdx(N, M, i + 1, j),
                getIdx(N, M, i, j - 1),
                getIdx(N, M, i - 1, j)
            };
            
            for (int k = 0; k < 4; ++k) {
                destModule = network->getSubmodule(satelliteModuleName.c_str(), neighborsIdx[k]);
                // 构建星间链路，可在此添加判断条件，不符合建链要求的链路跳过
                // ...
                createDynamicChannel(srcModule->gate("ethg$o", k), destModule->gate("ethg$i", gatesIdx[k]));
            }

        }
    }
}

int WalkerDeltaTopologyConfigurator::getIdx(int N, int M, int i, int j) {
    i = (i + N) % N;
    j = (j + M) % M;
    return i * M + j;
}


cDatarateChannel* WalkerDeltaTopologyConfigurator::createDynamicChannel(
    cGate* srcOutGate, 
    cGate* destInGate, 
    const char* channelName,
    double datarate,
    double propagationSpeed,
    double minUpdateInterval
)
{
    // 使用 cChannelType 创建信道（推荐方式）
    cChannelType *chType = cChannelType::get("leolab.satellite.wireless.DynamicChannel");
    
    const char* name = channelName ? channelName : "dynamicChannel";
    DynamicChannel *channel = dynamic_cast<DynamicChannel*>(chType->create(name));
    // cDatarateChannel* channel = dynamic_cast<cDatarateChannel*>(chType->create(name));
    
    // 设置参数
    channel->par("propagationSpeed").setDoubleValue(propagationSpeed);
    channel->par("minUpdateInterval").setDoubleValue(minUpdateInterval);
    channel->par("delay").setDoubleValue(1);
    channel->par("datarate").setDoubleValue(datarate);

    // channel->setSourceGate(srcOutGate);
    
    srcOutGate->connectTo(destInGate, channel);
    channel->callInitialize();
    
    return channel;
}

double WalkerDeltaTopologyConfigurator::calculateDistance(cModule* node1, cModule* node2) {

    double lon1, lat1, alt1, lon2, lat2, alt2;

    if (node1->hasPar("longitude") && node1->hasPar("latitude") && node1->hasPar("altitude")) {
        lon1 = node1->par("longitude").doubleValue();
        lat1 = node1->par("latitude").doubleValue();
        alt1 = node1->par("altitude").doubleValue();
    }
    else {
        CircularOrbitMobility* nodeMobility = dynamic_cast<CircularOrbitMobility *>(node1->getSubmodule("mobility"));
        const GeodeticPosition* satelliteGeoPos = nodeMobility->getCurrentGeoPos();
        lon1 = satelliteGeoPos->longitude;
        lat1 = satelliteGeoPos->latitude;
        alt1 = satelliteGeoPos->altitude;
    }
    
    if (node2->hasPar("longitude") && node2->hasPar("latitude") && node2->hasPar("altitude")) {
        lon2 = node2->par("longitude").doubleValue();
        lat2 = node2->par("latitude").doubleValue();
        alt2 = node2->par("altitude").doubleValue();
    }
    else {
        CircularOrbitMobility* nodeMobility = dynamic_cast<CircularOrbitMobility *>(node2->getSubmodule("mobility"));
        const GeodeticPosition* satelliteGeoPos = nodeMobility->getCurrentGeoPos();
        lon2 = satelliteGeoPos->longitude;
        lat2 = satelliteGeoPos->latitude;
        alt2 = satelliteGeoPos->altitude;
    }

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
    
    return distance;
}

}

