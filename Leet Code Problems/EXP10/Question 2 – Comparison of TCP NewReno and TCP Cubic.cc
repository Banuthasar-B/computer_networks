
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/tcp-newreno.h"
#include "ns3/tcp-cubic.h"

#include <iostream>
#include <fstream>
#include <cmath>

using namespace ns3;

std::ofstream cwndFile;

void CwndChange(uint32_t oldCwnd, uint32_t newCwnd)
{
    cwndFile << Simulator::Now().GetSeconds()
             << "," << newCwnd << "\n";
}

int main(int argc, char *argv[])
{
    std::string tcpType = "NewReno";
    double simTime = 30.0;
    std::string rate = "3Mbps";

    CommandLine cmd(__FILE__);
    cmd.AddValue("tcpType", "NewReno or Cubic", tcpType);
    cmd.AddValue("rate", "Bottleneck bandwidth", rate);
    cmd.Parse(argc, argv);

    // Select TCP algorithm
    if (tcpType == "NewReno")
    {
        Config::SetDefault(
            "ns3::TcpL4Protocol::SocketType",
            TypeIdValue(TcpNewReno::GetTypeId()));
    }
    else if (tcpType == "Cubic")
    {
        Config::SetDefault(
            "ns3::TcpL4Protocol::SocketType",
            TypeIdValue(TcpCubic::GetTypeId()));
    }
    else
    {
        std::cout << "Invalid TCP algorithm\n";
        return 1;
    }

    // Create four senders
    NodeContainer senders;
    senders.Create(4);

    // Create router and receiver
    Ptr<Node> router = CreateObject<Node>();
    Ptr<Node> receiver = CreateObject<Node>();

    InternetStackHelper internet;
    internet.Install(senders);
    internet.Install(router);
    internet.Install(receiver);

    // Access links
    PointToPointHelper access;
    access.SetDeviceAttribute("DataRate", StringValue("20Mbps"));
    access.SetChannelAttribute("Delay", StringValue("2ms"));

    for (uint32_t i = 0; i < 4; i++)
    {
        NodeContainer pair(senders.Get(i), router);

        NetDeviceContainer devices = access.Install(pair);

        Ipv4AddressHelper address;

        std::string network =
            "10.1." + std::to_string(i + 1) + ".0";

        address.SetBase(network.c_str(),
                        "255.255.255.0");

        address.Assign(devices);
    }

    // Common bottleneck link
    PointToPointHelper bottleneck;

    bottleneck.SetDeviceAttribute(
        "DataRate", StringValue(rate));

    bottleneck.SetChannelAttribute(
        "Delay", StringValue("20ms"));

    bottleneck.SetQueue(
        "ns3::DropTailQueue<Packet>",
        "MaxSize", StringValue("20p"));

    NodeContainer bottleneckNodes(router, receiver);

    NetDeviceContainer bottleneckDevices =
        bottleneck.Install(bottleneckNodes);

    Ipv4AddressHelper bottleneckAddress;

    bottleneckAddress.SetBase(
        "10.2.1.0", "255.255.255.0");

    Ipv4InterfaceContainer interfaces =
        bottleneckAddress.Assign(bottleneckDevices);

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    // Create four TCP flows
    uint16_t basePort = 50000;

    for (uint32_t i = 0; i < 4; i++)
    {
        uint16_t port = basePort + i;

        // Receiver
        PacketSinkHelper sink(
            "ns3::TcpSocketFactory",
            InetSocketAddress(
                Ipv4Address::GetAny(), port));

        ApplicationContainer sinkApp =
            sink.Install(receiver);

        sinkApp.Start(Seconds(0.0));
        sinkApp.Stop(Seconds(simTime));

        // Sender
        BulkSendHelper source(
            "ns3::TcpSocketFactory",
            InetSocketAddress(interfaces.GetAddress(1), port));

        source.SetAttribute("MaxBytes", UintegerValue(0));

        ApplicationContainer sourceApp =
            source.Install(senders.Get(i));

        sourceApp.Start(Seconds(1.0));
        sourceApp.Stop(Seconds(simTime - 1));
    }

    // Congestion window tracing
    cwndFile.open("comparison-cwnd.csv");
    cwndFile << "Time,CwndBytes\n";

    Simulator::Schedule(Seconds(1.1), []()
    {
        Config::ConnectWithoutContext(
            "/NodeList/*/$ns3::TcpL4Protocol/"
            "SocketList/*/CongestionWindow",
            MakeCallback(&CwndChange));
    });

    // FlowMonitor
    FlowMonitorHelper flowHelper;
    Ptr<FlowMonitor> monitor = flowHelper.InstallAll();

    Simulator::Stop(Seconds(simTime));
    Simulator::Run();

    monitor->CheckForLostPackets();

    Ptr<Ipv4FlowClassifier> classifier =
        DynamicCast<Ipv4FlowClassifier>(
            flowHelper.GetClassifier());

    auto stats = monitor->GetFlowStats();

    double totalThroughput = 0.0;
    double throughputSquare = 0.0;
    double totalDelay = 0.0;

    uint64_t totalTx = 0;
    uint64_t totalRx = 0;

    uint32_t count = 0;

    std::cout << "\nTCP " << tcpType
              << " PERFORMANCE RESULTS\n";

    for (auto flow : stats)
    {
        auto tuple = classifier->FindFlow(flow.first);

        if (tuple.destinationPort < basePort ||
            tuple.destinationPort >= basePort + 4)
            continue;

        auto s = flow.second;

        double throughput =
            s.rxBytes * 8.0 /
            ((simTime - 2.0) * 1000000.0);

        double delay = 0.0;

        if (s.rxPackets > 0)
            delay = s.delaySum.GetSeconds()
                    * 1000.0 / s.rxPackets;

        double pdr = 0.0;

        if (s.txPackets > 0)
            pdr = 100.0 * s.rxPackets / s.txPackets;

        std::cout << "\nFlow "
                  << tuple.destinationPort - basePort + 1
                  << "\n";

        std::cout << "Throughput: "
                  << throughput << " Mbps\n";

        std::cout << "Average Delay: "
                  << delay << " ms\n";

        std::cout << "Packet Delivery Ratio: "
                  << pdr << " %\n";

        std::cout << "Packet Loss: "
                  << s.lostPackets << "\n";

        totalThroughput += throughput;
        throughputSquare += throughput * throughput;
        totalDelay += delay;

        totalTx += s.txPackets;
        totalRx += s.rxPackets;

        count++;
    }

    // Jain Fairness Index
    double fairness = 0.0;

    if (count > 0 && throughputSquare > 0)
    {
        fairness =
            (totalThroughput * totalThroughput) /
            (count * throughputSquare);
    }

    std::cout << "\nOVERALL RESULTS\n";

    std::cout << "Total Throughput: "
              << totalThroughput << " Mbps\n";

    std::cout << "Average Delay: "
              << (count ? totalDelay / count : 0)
              << " ms\n";

    std::cout << "Total Transmitted: "
              << totalTx << "\n";

    std::cout << "Total Received: "
              << totalRx << "\n";

    std::cout << "Total Packet Loss: "
              << (totalTx > totalRx ? totalTx - totalRx : 0)
              << "\n";

    std::cout << "Jain Fairness Index: "
              << fairness << "\n";

    cwndFile.close();

    monitor->SerializeToXmlFile(
        "comparison-flowmon.xml", true, true);

    Simulator::Destroy();

    return 0;
}
