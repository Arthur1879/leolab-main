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

#ifndef SATELLITE_ROUTING_BELLMANFORDROUTING_H_
#define SATELLITE_ROUTING_BELLMANFORDROUTING_H_

#include "Topology.h"
#include <omnetpp.h>
#include "inet/common/INETDefs.h"
#include "inet/networklayer/ipv4/IIpv4RoutingTable.h"
#include "inet/networklayer/contract/IInterfaceTable.h"

namespace leolab {

using namespace omnetpp;
using namespace inet;

/**
 * 基于 Dijkstra（或子类自定义）算法的路由模块。
 * - 继承 cSimpleModule（OMNeT++ 模块基类）  
 * - 通过内部嵌套类 Topology 继承 inet::Topology，以便直接修改、调用`calculateWeightedSingleShortestPathsFrom` 等成员。
 */
class BellmanFordRouting : public cSimpleModule
{
  private:
    // ---------- 计时器 ----------
    cMessage *startupTimer = nullptr;   // 延迟启动的自触发消息

    // ---------- 宿主信息 ----------
    cModule *host = nullptr;            // 拥有本模块的节点（router / host）

    ModuleRefByPar<IIpv4RoutingTable> rt;   // 宿主的 IPv4 路由表
    ModuleRefByPar<IInterfaceTable> ift;    // 宿主的接口表

    // ---------- 私有方法 ----------
    void updateRoutingTable();          // 抽取拓扑、计算路径并写入路由表

  protected:
    // OMNeT++ 生命周期
    virtual int numInitStages() const override { return NUM_INIT_STAGES; }
    virtual void initialize(int stage) override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void finish() override;    // 释放计时器

  public:
    BellmanFordRouting();
    virtual ~BellmanFordRouting();

    // 让外部代码能够直接使用 Topology 功能
    class Topology : public leolab::Topology {
    public:
        // 直接复用 inet::Topology 的构造函数
        explicit Topology(const char *name = nullptr) : leolab::Topology(name) {};
        virtual void calculateWeightedSingleShortestPathsFrom(Node *source) const;
    };
};

} // namespace leolab

#endif /* SATELLITE_ROUTING_BELLMANFORDROUTING_H_ */
