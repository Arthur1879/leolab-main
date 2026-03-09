#include "DijkstraRouting.h"
#include <inet/common/ModuleAccess.h>
#include <inet/networklayer/ipv4/Ipv4InterfaceData.h>
#include <inet/networklayer/ipv4/Ipv4Route.h>
#include <inet/networklayer/contract/IRoutingTable.h>   // 使用接口而非实现类
#include <unordered_map>

namespace leolab {

using namespace omnetpp;
using namespace inet;

// 必须使用 OMNeT++ 提供的宏把类注册为模块
Define_Module(DijkstraRouting);

DijkstraRouting::DijkstraRouting() { }

DijkstraRouting::~DijkstraRouting()
{
    // 若仿真提前结束，计时器仍可能存在
    cancelAndDelete(startupTimer);
}

void DijkstraRouting::initialize(int stage)
{
    if (stage == INITSTAGE_LOCAL) {
        // 获取宿主节点
        host = getContainingNode(this);
        if (!host)
            throw cRuntimeError("DijkstraRouting: cannot find containing node");

        // 通过 NED 参数绑定接口表和路由表模块（与 INET 默认参数名保持一致）
        ift.reference(this, "interfaceTableModule", true);
        rt.reference(this, "routingTableModule", true);

        // 创建并安排启动计时器（仿真时间 0 立即触发一次）
        startupTimer = new cMessage("DijkstraRouting-startup");
        scheduleAt(simTime(), startupTimer);
    }
}

void DijkstraRouting::handleMessage(cMessage *msg)
{
    if (msg == startupTimer) {
        // 计时器触发后执行一次拓扑构建与路由更新
        updateRoutingTable();

        // 计时器只用一次，安全删除
        delete startupTimer;
        startupTimer = nullptr;
    }
}

void DijkstraRouting::finish()
{
    cancelAndDelete(startupTimer);
    startupTimer = nullptr;
}

void DijkstraRouting::updateRoutingTable()
{
    // 创建 Topology 实例
    Topology topo("topo");

    // 获取宿主节点类型，用于构建拓扑
    const char *hostType = host->getNedTypeName();
    // EV_INFO << "宿主节点类型: " << hostType << endl;

    // 通过 NED 类型抽取拓扑
    std::vector<std::string> typeVec = { std::string(hostType) };
    topo.extractByNedTypeName(typeVec);

    // 找到本节点在拓扑中的对应 Node 对象
    Topology::Node *hostNode = topo.getNodeFor(host);
    if (!hostNode) {
        EV_INFO << "Warning: host not found in topology, aborting routing update.\n";
        return;
    }


    // 计算单源最短路径 
    topo.calculateWeightedSingleShortestPathsFrom(hostNode);


    // 获取直连邻居cModule->eth的映射关系
    // key: 远端模块cModule指针，value: 本节点对应的NetworkInterface*
    std::unordered_map<cModule*, NetworkInterface*> neighborMap;
    for (int i = 0; i < ift->getNumInterfaces(); ++i) {
        NetworkInterface *intf = ift->getInterface(i);
        const char *ifName = intf->getInterfaceName();   // 形如 "eth0"
        // 判断接口名前缀是否为eth
        if (strncmp(ifName, "eth", 3) == 0) {
            int idx = -1;
            // 判断接口索引是否在[0,3]范围内
            if (sscanf(ifName, "eth%d", &idx) == 1 && idx >= 0 && idx <= 3) {
                cGate *gate = intf->getParentModule()->gate("ethg$o", idx);
                if (gate && gate->isConnected()) {
                    cGate *remoteGate = gate->getNextGate();
                    if (remoteGate) {
                        cModule *remoteMod = remoteGate->getOwnerModule();
                        neighborMap[remoteMod] = intf;   // 记录映射关系
                    }
                }
            }
        }
    }

    // 为每个可达目的节点写入路由
    // 使用接口返回的抽象路由表（IIpv4RoutingTable）
    IIpv4RoutingTable *rtMod = rt.get();

    for (int i = 0; i < topo.getNumNodes(); ++i) {
        Topology::Node *dstNode = topo.getNode(i);
        if (dstNode == hostNode) continue;   // 跳过自己

        // 取得目的节点 eth4 接口的 IPv4 地址和子网掩码
        Ipv4Address destAddr = Ipv4Address::UNSPECIFIED_ADDRESS;
        Ipv4Address destMask = Ipv4Address::UNSPECIFIED_ADDRESS;   // 用来保存子网掩码

        // 1) 取得目的节点的根模块
        cModule *dstMod = dstNode->getModule();   // dstNode 为 cObject*，已指向目标节点

        // 2) 取得该节点的 IInterfaceTable（默认名字为 "interfaceTable"）
        IInterfaceTable *ifTable = check_and_cast<IInterfaceTable*>(dstMod->getSubmodule("interfaceTable"));
        if (!ifTable) {
            throw cRuntimeError("Destination node %s 没有 interfaceTable", dstMod->getFullPath().c_str());
        }

        // 3) 在 InterfaceTable 中遍历，寻找 eth4 的接口
        bool found = false;
        for (int i = 0; i < ifTable->getNumInterfaces(); ++i) {
            NetworkInterface *ifData = ifTable->getInterface(i);
            if (!ifData) continue;

            const char *ifName = ifData->getInterfaceName();
            if (strcmp(ifName, "eth4") == 0) {
                destAddr = ifData->getIpv4Address();            // IPv4 地址
                destMask = ifData->getIpv4Netmask();            // 子网掩码
                found = true;
                break;
            }
        }

        // 4) 错误处理
        if (!found || destAddr.isUnspecified()) {
            throw cRuntimeError("Destination node %s 没有 eth4 接口或该接口未配置 IP 地址", dstMod->getFullPath().c_str());
        }

        for (int i = 0; i < dstNode->getNumOutLinks(); ++i) {
            EV_INFO << "link: " << dstNode->getLinkOut(i)->getLinkOutRemoteNode()->getModule()->getFullPath() << endl;
        }

        // 回溯得到从 host 到 dst 的下一跳节点
        Topology::Link *nextLink = dstNode->getPath(0);
        while (nextLink && nextLink->getLinkInRemoteNode() != hostNode) {
            nextLink = nextLink->getLinkInRemoteNode()->getPath(0);
        }

        // 确定出接口
        NetworkInterface *outIf = nullptr;

        // 若 nextHop 正好是 eth[0]~eth[3] 直接相连的邻居，则使用对应的接口
        auto it = neighborMap.find(nextLink->getLinkOutRemoteNode()->getModule());
        if (it != neighborMap.end())
            outIf = it->second;

        // 若不在直接邻居集合中，回退到默认的非 loopback 接口（保持兼容）
        if (!outIf) {
            throw cRuntimeError("Output interface error!\n");
        }

        // 添加路由条目
        Ipv4Route *entry = new Ipv4Route();
        entry->setDestination(destAddr.doAnd(destMask));
        entry->setNetmask(destMask);
        entry->setInterface(outIf);
        rtMod->addRoute(entry);
    }

    EV_INFO << "Dijkstra 路由表已更新，共 " << rtMod->getNumRoutes() << " 条路由。" << endl;
}

} // namespace leolab
