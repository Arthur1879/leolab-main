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

#ifndef SATELLITE_CIRCULARORBITMOBILITY_H_
#define SATELLITE_CIRCULARORBITMOBILITY_H_

#include "../common/TypeDefs.h"
#include "inet/mobility/base/MovingMobilityBase.h"


namespace leolab {

using namespace omnetpp;
using namespace inet;

class CircularOrbitMobility : public MovingMobilityBase {
    private:
        double initPhase;
        double alpha;
        double altitude;
        double rightAscension;
        double earthRotationRate;

        double phase;
        double omega;
        double longitude;
        double latitude;
        Coord constraintAreaCenter;

        // 定义信号geodeticPositionChangedSignal
        simsignal_t geodeticPositionChangedSignal;
        
        GeodeticPosition *currentGeoPos;

  	protected:
		virtual void initialize(int) override;
        virtual void setInitialPosition() override;
        virtual void move() override;
        

    public:
        CircularOrbitMobility();
        ~CircularOrbitMobility();
        void initParamerers();
        const GeodeticPosition* getCurrentGeoPos();
};

}
#endif /* SATELLITE_CIRCULARORBITMOBILITY_H_ */
