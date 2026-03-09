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

package leolab.satellite.mobility;

import inet.mobility.base.MovingMobilityBase;

simple CircularOrbitMobility extends MovingMobilityBase {
    
    parameters:
        double initPhase = default(0rad) @unit(rad) @mutable; // 初始相位
        double alpha = default(45rad)  @unit(rad) @mutable; // 轨道倾角
        double altitude = default(550km) @unit(km) @mutable; // 卫星海拔
        double rightAscension = default(0rad) @unit(rad) @mutable; // 升交点赤经
        double earthRotationRate = default(7.292e-5rad) @unit(rad); // 地球自转速度
        @class(leolab::CircularOrbitMobility);
}