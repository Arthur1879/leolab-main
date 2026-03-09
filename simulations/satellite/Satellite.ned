package leolab.simulations.satellite;

import inet.visualizer.common.IntegratedVisualizer;
import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;
import leolab.satellite.node.SatelliteNode;
import leolab.satellite.node.GroundHost;
import leolab.satellite.wireless.DynamicChannel;
import leolab.satellite.configurator.WalkerDeltaTopologyConfigurator;

network Satellite
{
    parameters:
        @display("bgi=background_earth,s;bgb=2160,1080");
        @figure[title](type=label; pos=0,-1; anchor=sw; color=darkblue);
        double initRightAscension @unit(rad) = default(0rad);
        double initPhase @unit(rad) = default(0rad);
        double alpha @unit(rad) = default(45rad);
        double altitude @unit(km) = default(550km);
        int numSatellites = default(2);
        int numPlane = default(2);
        int F = default(0);
        int numGroundHosts = default(0);
        
    submodules:
        visualizer: IntegratedVisualizer {
            @display("p=300,100");
        }
        configurator: Ipv4NetworkConfigurator {
            @display("p=200,100");
        }
        topologyConfigurator: WalkerDeltaTopologyConfigurator {
            @display("p=100,100");
            satelliteModuleName = "satelliteNode";
            groundHostModuleName = "groundHost";
        }
        satelliteNode[numSatellites]: SatelliteNode {
        }
        groundHost[numGroundHosts]: GroundHost {
        }
        
    connections allowunconnected:
}