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

#ifndef SATELLITE_CONFIGURATOR_WALKERDELTATOPOLOGYCONFIGURATOR_H_
#define SATELLITE_CONFIGURATOR_WALKERDELTATOPOLOGYCONFIGURATOR_H_


#include <omnetpp.h>
#include "inet/common/INETMath.h"
#include "inet/common/IProtocolRegistrationListener.h"
#include "inet/networklayer/configurator/ipv4/Ipv4NetworkConfigurator.h"
#include "../common/TypeDefs.h"
#include "../wireless/DynamicChannel.h"
#include "../mobility/CircularOrbitMobility.h"

namespace leolab {

using namespace omnetpp;
using namespace inet;


class WalkerDeltaTopologyConfigurator : public cSimpleModule {
    private:
        std::string satelliteModuleName;
        std::string groundHostModuleName;

        cMessage *initTimer;
        cMessage *updateTimer;
        double updateInterval;

        cModule* network;
        
        double initRightAscension;
        double initPhase;
        double altitude;
        double alpha;
        int numSatellites;
        int numPlane;
        int F;
        int numGroundHosts;

        void initSatellitePosition();
        void createFourLinks();
        void updateGroundToSatelliteLinks();
        
        void forceOspfProtocolRegistration(cModule* node);
        int getIdx(int N, int M, int i, int j);
        cDatarateChannel* createDynamicChannel(
            cGate* srcOutGate, 
            cGate* destInGate, 
            const char* channelName = nullptr, 
            double datarate = 1e7,
            double propagationSpeed = 299792458.0,
            double minUpdateInterval = 0.1
        );
        double calculateDistance(cModule* terminal, cModule* satellite);

  	protected:
        virtual int numInitStages() const override { return NUM_INIT_STAGES; }
		virtual void initialize(int) override;
		virtual void handleMessage(cMessage *msg) override;

    public:
        WalkerDeltaTopologyConfigurator();
        virtual ~WalkerDeltaTopologyConfigurator();
};

}
#endif /* SATELLITE_CONFIGURATOR_WALKERDELTATOPOLOGYCONFIGURATOR_H_ */
