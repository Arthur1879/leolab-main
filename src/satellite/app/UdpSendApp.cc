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

#include "UdpSendApp.h"
#include "inet/networklayer/common/L3AddressResolver.h"

namespace leolab {

Define_Module(UdpSendApp);

UdpSendApp::UdpSendApp() {
}

UdpSendApp::~UdpSendApp() {
}

L3Address UdpSendApp::chooseDestAddr()
{
    int k = intrand(destAddresses.size());
    auto lastDestAddress = destAddresses[k];
    if (!L3AddressResolver().tryResolve(destAddressStr[k].c_str(), destAddresses[k])) {
        destAddresses[k] = lastDestAddress;
    }
    return destAddresses[k];
}

}