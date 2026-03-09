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

package leolab.satellite.wireless;

import inet.node.ethernet.EthernetLink;

channel DynamicChannel extends EthernetLink {
    
    parameters:
        @class(leolab::DynamicChannel);
        double propagationSpeed = default(299792458mps) @unit(mps); // 传播速度 (默认光速)
        double minUpdateInterval = default(100ms) @unit(s);         // 最小更新间隔 - 控制更新频率
        delay = default(1s);                                        // 初始化使用，仿真运行后自动更新
        datarate = default(10Mbps);
}
