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

#ifndef SATELLITE_DYNAMICCHANNEL_H_
#define SATELLITE_DYNAMICCHANNEL_H_

#include <omnetpp.h>
#include "inet/common/INETDefs.h"
#include "../common/TypeDefs.h"

namespace leolab {

using namespace omnetpp;
using namespace inet;

class DynamicChannel : public cDatarateChannel, public cListener {
    private:    
        // 传播速度 (默认光速)
        double propagationSpeed;
        // 更新时间间隔阈值
        simtime_t minUpdateInterval;
        simtime_t lastUpdateTime;
        // 连接的模块
        cModule *srcModule;
        cModule *destModule;
        // 定义信号geodeticPositionChangedSignal
        simsignal_t geodeticPositionChangedSignal;
        GeodeticPosition srcPosition;
        GeodeticPosition destPosition;
        double lastDistance;

    protected:
        virtual void initialize() override;

        virtual Result processMessage(cMessage *msg, const SendOptions& options, simtime_t t) override;
        // cListener接口 - 核心信号处理函数
        virtual void receiveSignal(cComponent *source, simsignal_t signalID, cObject *obj, cObject *details) override;

        void updateChannelDelay();
        double calculateDistance(double lon1, double lat1, double alt1, double lon2, double lat2, double alt2);

        // 辅助函数
        virtual cModule* getMobilityModule(cModule *module);    
        virtual cModule* getNodeFromMobility(cModule *mobilityModule);
        virtual cModule* getDestinationModule();

    public:
        DynamicChannel(const char *name = nullptr);
        ~DynamicChannel();
        void initParamerers();

        virtual double getNominalDatarate() const override;
        virtual bool isDisabled() const override  {return flags & (1 << 10);}
};

}
#endif /* SATELLITE_DYNAMICCHANNEL_H_ */
