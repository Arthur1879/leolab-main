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

package leolab.satellite.node;

import inet.applications.contract.IApp;
import inet.node.inet.StandardHost;

module GroundHost extends StandardHost
{
    parameters:
        @display("is=s;i=device/receiverdish");
        numEthInterfaces = default(1);

        double longitude @unit(deg) = default(uniform(-180deg, 180deg));
        double latitude @unit(deg) = default(uniform(-90deg, 90deg));
        double altitude @unit(km) = default(0km);
        mobility.typename = "StationaryMobility";
        mobility.initFromDisplayString = false;
        mobility.initialLongitude = longitude;
        mobility.initialLatitude = latitude;
        mobility.initialAltitude = altitude;
        mobility.initialX = mobility.constraintAreaMinX + (longitude/1deg + 180) * (mobility.constraintAreaMaxX - mobility.constraintAreaMinX) / 360;
        mobility.initialY = mobility.constraintAreaMinY + (90 - latitude/1deg) * (mobility.constraintAreaMaxY - mobility.constraintAreaMinY) / 180;
        mobility.initialZ = altitude;

        bool hasDhcp = default(false);

        submodules:
            dhcp: <default("DhcpClient")> like IApp if hasDhcp {
                startTime = default(uniform(0s, 1s));
            }

        connections allowunconnected:
            dhcp.socketOut --> at.in++ if hasDhcp;
            dhcp.socketIn <-- at.out++ if hasDhcp;
}
