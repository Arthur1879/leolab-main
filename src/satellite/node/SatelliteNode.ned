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

import inet.node.ospfv2.OspfRouter;
import inet.node.rip.RipRouter;
import inet.node.inet.Router;

import leolab.satellite.routing.IIdealRouting;

module SatelliteNode extends Router
{
    parameters:
        @display("is=s;i=device/satellite");
        mobility.typename = default("CircularOrbitMobility");
        numEthInterfaces = default(5);
        
        hasDhcp = default(false);
        dhcp.interface = default(hasDhcp ? "eth4" : "");
        dhcp.numReservedAddresses = default(hasDhcp ? 2 : 0);
        dhcp.maxNumClients = default(hasDhcp ? 1 : 0);
        dhcp.leaseTime = default(hasDhcp ? 10s : 0s);

        bool hasRoutingAlgorithm = default(false);
    
    submodules:
        routingAlgorithm: <default("DijkstraRouting")> like IIdealRouting if hasRoutingAlgorithm {
                @display("p=975,226");
        }

}
