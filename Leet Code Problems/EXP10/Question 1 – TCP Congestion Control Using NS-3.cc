
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/tcp-newreno.h"

#include <iostream>
#include <fstream>

using namespace ns3;

std::ofstream cwndFile;

void CwndChange(uint32_t oldCwnd, uint32_t newCwnd)
{
    cwndFile << Simulator::Now().GetSeconds()
             << "," << newCwnd << std::endl;
}

int main(int argc, char *argv[])
{
    // TCP NewReno
    Config::SetDefault(
        "ns3::TcpL4Protocol::SocketType",
        TypeIdValue(TcpNewReno::GetTypeId()));

    // Create three nodes
    NodeContainer nodes;
    nodes.Create(3);

    // Create point-to-point links
    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    p2p.SetChannelAttribute("Delay", StringValue("10ms"));
    p2p.SetQueue("ns3::DropTailQueue<Packet>",
                 "MaxSize", StringValue("20p"));

    // Sender to Router
    NodeContainer link1(nodes.Get(0), nodes.Get(1));
    NetDeviceContainer devices1 = p2p.Install(link1);

    // Router to Receiver
    NodeContainer link2(nodes.Get(1), nodes.Get(2));
    NetDeviceContainer devices2 = p2p.Install(link2);

    // Install Internet stack
    InternetStackHelper internet;
    internet.Install(nodes);

    // Assign IP addresses
    Ipv4AddressHelper address;

    address.SetBase("10.1.1.0", "255.255.255.0");
    address.Assign(devices1);

    address.SetBase("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces2 =
        address.Assign(devices2);

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    // TCP Receiver
    uint16_t port = 50000;

    PacketSinkHelper sink(
        "ns3::TcpSocketFactory",
        InetSocketAddress(Ipv4Address::GetAny(), port));

    ApplicationContainer sinkApp =
        sink.Install(nodes.Get(2));

    sinkApp.Start(Seconds(0.0));
    sinkApp.Stop(Seconds(20.0));

    // TCP Sender
    BulkSendHelper source(
        "ns3::TcpSocketFactory",
        InetSocketAddress(interfaces2.GetAddress(1), port));

    source.SetAttribute("MaxBytes", UintegerValue(0));
    source.SetAttribute("SendSize", UintegerValue(1024));

    ApplicationContainer sourceApp =
        source.Install(nodes.Get(0));

    sourceApp.Start(Seconds(1.0));
    sourceApp.Stop(Seconds(19.0));

    // Congestion Window tracing
    cwndFile.open("tcp-cwnd.csv");
    cwndFile << "Time,CwndBytes\n";

    Simulator::Schedule(Seconds(1.1), []()
    {
        Config::ConnectWithoutContext(
            "/NodeList/0/$ns3::TcpL4Protocol/"
            "SocketList/0/CongestionWindow",
            MakeCallback(&CwndChange));
    });

    // Flow Monitor
    FlowMonitorHelper flowHelper;
    Ptr<FlowMonitor> monitor = flowHelper.InstallAll();

    Simulator::Stop(Seconds(20.0));
    Simulator::Run();

    // Receiver statistics
    Ptr<PacketSink> sinkPtr =
        DynamicCast<PacketSink>(sinkApp.Get(0));

    uint64_t receivedBytes = sinkPtr->GetTotalRx();

    double throughput =
        receivedBytes * 8.0 / (18.0 * 1000000.0);

    std::cout << "\nTCP CONGESTION CONTROL RESULTS\n";
    std::cout << "Received Bytes: " << receivedBytes << "\n";
    std::cout << "Throughput: " << throughput << " Mbps\n";

    // Flow statistics
    monitor->CheckForLostPackets();

    Ptr<Ipv4FlowClassifier> classifier =
        DynamicCast<Ipv4FlowClassifier>(
            flowHelper.GetClassifier());

    auto stats = monitor->GetFlowStats();

    for (auto flow : stats)
    {
        auto tuple = classifier->FindFlow(flow.first);
        auto s = flow.second;

        if (tuple.destinationPort != port)
            continue;

        double pdr = 0.0;
        double delay = 0.0;

        if (s.txPackets > 0)
            pdr = 100.0 * s.rxPackets / s.txPackets;

        if (s.rxPackets > 0)
            delay = 1000.0 * s.delaySum.GetSeconds()
                    / s.rxPackets;

        std::cout << "Transmitted Packets: "
                  << s.txPackets << "\n";

        std::cout << "Received Packets: "
                  << s.rxPackets << "\n";

        std::cout << "Packet Delivery Ratio: "
                  << pdr << " %\n";

        std::cout << "Packet Loss: "
                  << s.lostPackets << "\n";

        std::cout << "Average Delay: "
                  << delay << " ms\n";
    }

    cwndFile.close();

    monitor->SerializeToXmlFile(
        "tcp-flowmon.xml", true, true);

    Simulator::Destroy();

    return 0;
}
