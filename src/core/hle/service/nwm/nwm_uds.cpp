// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma optimize("", off)
#include <array>
#include <cstring>
#include <mutex>
#include <unordered_map>
#include <vector>
#include <cryptopp/md5.h>
#include "common/common_types.h"
#include "common/logging/log.h"
#include "core/core_timing.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/kernel/event.h"
#include "core/hle/kernel/shared_memory.h"
#include "core/hle/result.h"
#include "core/hle/service/nwm/nwm_uds.h"
#include "core/hle/service/nwm/uds_beacon.h"
#include "core/hle/service/nwm/uds_connection.h"
#include "core/hle/service/nwm/uds_data.h"
#include "core/memory.h"
#include "network/network.h"

namespace Service {
namespace NWM {

// Event that is signaled every time the connection status changes.
static Kernel::SharedPtr<Kernel::Event> connection_status_event;

// Shared memory provided by the application to store the receive buffer.
// This is not currently used.
static Kernel::SharedPtr<Kernel::SharedMemory> recv_buffer_memory;

// Connection status of this 3DS.
static ConnectionStatus connection_status{};

/* Node information about the current network.
 * The amount of elements in this vector is always the maximum number
 * of nodes specified in the network configuration.
 * The first node is always the host when hosting a network.
 */
static NodeList node_info;

// Node information about our own system.
static NodeInfo current_node;

struct BindNodeData {
    u32 bind_node_id;    ///< Id of the bind node associated with this data.
    u8 channel;          ///< Channel that this bind node was bound to.
    u16 network_node_id; ///< Node id this bind node is associated with, only packets from this
                         /// network node will be received.
    Kernel::SharedPtr<Kernel::Event> event; ///< Receive event for this bind node.
    int signal_event_helper; ///< CoreTiming event used to signal the Kernel event from the emu
                             /// thread.

    std::deque<std::vector<u8>> received_packets; ///< List of packets received on this channel.
};

// Mapping of data channels to their internal data.
static std::unordered_map<u32, BindNodeData> channel_data;

// The WiFi network channel that the network is currently on.
// Since we're not actually interacting with physical radio waves, this is just a dummy value.
static u8 network_channel = DefaultNetworkChannel;

// Information about the network that we're currently connected to.
static NetworkInfo network_info;

// Event that will generate and send the 802.11 beacon frames.
static int beacon_broadcast_event;

// Callback identifier for the OnWifiPacketReceived event.
static Network::RoomMember::CallbackHandle<Network::WifiPacket> wifi_packet_received;

// Mutex to synchronize access to the connection status between the emulation thread and the
// network thread.
static std::mutex connection_status_mutex;

// Mutex to synchronize access to the list of received beacons between the emulation thread and the
// network thread.
static std::mutex beacon_mutex;

// Number of beacons to store before we start dropping the old ones.
// TODO(Subv): Find a more accurate value for this limit.
constexpr size_t MaxBeaconFrames = 1;

// List of the last <MaxBeaconFrames> beacons received from the network.
static std::deque<Network::WifiPacket> received_beacons;

// CoreTiming event to signal the connection_status_event from the emulation thread.
static int connection_status_changed_event = -1;

// Network node id used when a SecureData packet is addressed to every connected node.
constexpr u16 BroadcastNetworkNodeId = 0xFFFF;

/**
 * Returns a list of received 802.11 frames from the specified sender
 * matching the type since the last call.
 */
std::deque<Network::WifiPacket> GetReceivedPackets(Network::WifiPacket::PacketType type,
                                                   const MacAddress& sender) {
    if (type == Network::WifiPacket::PacketType::Beacon) {
        std::lock_guard<std::mutex> lock(beacon_mutex);

        return std::move(received_beacons);
    }

    return {};
}

/// Sends a WifiPacket to the room we're currently connected to.
void SendPacket(Network::WifiPacket& packet) {
    if (auto room_member = Network::GetRoomMember().lock()) {
        if (room_member->GetState() == Network::RoomMember::State::Joined) {
            packet.transmitter_address = room_member->GetMacAddress();
            room_member->SendWifiPacket(packet);
        }
    }
}

/*
 * Returns an available index in the nodes array for the
 * currently-hosted UDS network.
 */
static u16 GetNextAvailableNodeId() {
    ASSERT_MSG(connection_status.status == static_cast<u32>(NetworkStatus::ConnectedAsHost),
               "Can not accept clients if we're not hosting a network");

    for (u16 index = 0; index < connection_status.max_nodes; ++index) {
        if ((connection_status.node_bitmask & (1 << index)) == 0)
            return index;
    }

    // Any connection attempts to an already full network should have been refused.
    ASSERT_MSG(false, "No available connection slots in the network");
}

/*
 * Start a connection sequence with an UDS server. The sequence starts by sending an 802.11
 * authentication frame with SEQ1.
 */
void StartConnectionSequence(const MacAddress& server) {
    {
        std::lock_guard<std::mutex> lock(connection_status_mutex);

        ASSERT(connection_status.status == static_cast<u32>(NetworkStatus::NotConnected));

        // TODO(Subv): Handle timeout.
    }

    // Send an authentication frame with SEQ1
    using Network::WifiPacket;
    WifiPacket auth_request;
    auth_request.channel = network_channel;
    auth_request.data = GenerateAuthenticationFrame(AuthenticationSeq::SEQ1);
    auth_request.destination_address = server;
    auth_request.type = WifiPacket::PacketType::Authentication;

    SendPacket(auth_request);
}

/// Sends an Association Response frame to the specified mac address
void SendAssociationResponseFrame(const MacAddress& address) {
    {
        std::lock_guard<std::mutex> lock(connection_status_mutex);
        ASSERT_MSG(connection_status.status == static_cast<u32>(NetworkStatus::ConnectedAsHost));
    }

    using Network::WifiPacket;
    WifiPacket assoc_response;
    assoc_response.channel = network_channel;
    // TODO(Subv): This might cause multiple clients to end up with the same association id.
    assoc_response.data =
        GenerateAssocResponseFrame(AssocStatus::Successful, 1, network_info.network_id);
    assoc_response.destination_address = address;
    assoc_response.type = WifiPacket::PacketType::AssociationResponse;

    SendPacket(assoc_response);
}

void HandleAuthenticationFrame(const Network::WifiPacket& packet) {
    // Only the SEQ1 auth frame is handled here, the SEQ2 frame doesn't need any special behavior
    if (GetAuthenticationSeqNumber(packet.data) == AuthenticationSeq::SEQ1) {
        {
            std::lock_guard<std::mutex> lock(connection_status_mutex);
            ASSERT_MSG(connection_status.status ==
                       static_cast<u32>(NetworkStatus::ConnectedAsHost));
        }

        // Respond with an authentication response frame with SEQ2
        // TODO(B3N30): Find out why Subv sends SEQ2 when he doesn't handle it
        using Network::WifiPacket;
        WifiPacket auth_request;
        auth_request.channel = network_channel;
        auth_request.data = GenerateAuthenticationFrame(AuthenticationSeq::SEQ2);
        auth_request.destination_address = packet.transmitter_address;
        auth_request.type = WifiPacket::PacketType::Authentication;

        SendPacket(auth_request);

        SendAssociationResponseFrame(packet.transmitter_address);
    }
}

/// Stores the received beacon in the buffer of beacon frames.
void HandleBeaconFrame(const Network::WifiPacket& packet) {
    std::lock_guard<std::mutex> lock(beacon_mutex);

    received_beacons.emplace_back(packet);

    // Discard old beacons if the buffer is full.
    if (received_beacons.size() > MaxBeaconFrames)
        received_beacons.pop_front();
}

void HandleAssociationResponseFrame(const Network::WifiPacket& packet) {
    auto assoc_result = GetAssociationResult(packet.data);

    ASSERT_MSG(std::get<AssocStatus>(assoc_result) == AssocStatus::Successful,
               "Could not join network");
    {
        std::lock_guard<std::mutex> lock(connection_status_mutex);
        ASSERT(connection_status.status == static_cast<u32>(NetworkStatus::NotConnected));
    }

    // Send the EAPoL-Start packet to the server.
    using Network::WifiPacket;
    WifiPacket eapol_start;
    eapol_start.channel = network_channel;
    eapol_start.data = GenerateEAPoLStartFrame(std::get<u16>(assoc_result), current_node);
    // TODO(Subv): Encrypt the packet.
    eapol_start.destination_address = packet.transmitter_address;
    eapol_start.type = WifiPacket::PacketType::Data;

    SendPacket(eapol_start);
}

static void HandleEAPoLPacket(const Network::WifiPacket& packet) {
    std::lock_guard<std::mutex> lock(connection_status_mutex);

    if (GetEAPoLFrameType(packet.data) == EAPoLStartMagic) {
        ASSERT(connection_status.status == static_cast<u32>(NetworkStatus::ConnectedAsHost));

        auto node = DeserializeNodeInfoFromFrame(packet.data);

        if (connection_status.max_nodes == connection_status.total_nodes) {
            // Reject connection attempt
            // TODO(B3N30): Figure out what packet is sent here
            return;
        }

        // Get an unused network node id
        u16 node_id = GetNextAvailableNodeId();
        node.network_node_id = node_id + 1;

        connection_status.node_bitmask |= 1 << node_id;
        connection_status.changed_nodes |= 1 << node_id;
        connection_status.nodes[node_id] = node.network_node_id;
        connection_status.total_nodes++;

        u8 current_nodes = network_info.total_nodes;
        node_info[current_nodes] = node;

        network_info.total_nodes++;

        // Send the EAPoL-Logoff packet.
        using Network::WifiPacket;
        WifiPacket eapol_logoff;
        eapol_logoff.channel = network_channel;
        eapol_logoff.data =
            GenerateEAPoLLogoffFrame(packet.transmitter_address, node.network_node_id, node_info,
                                     network_info.max_nodes, network_info.total_nodes);
        // TODO(Subv): Encrypt the packet.
        eapol_logoff.destination_address = packet.transmitter_address;
        eapol_logoff.type = WifiPacket::PacketType::Data;

        SendPacket(eapol_logoff);

        CoreTiming::ScheduleEvent_Threadsafe_Immediate(connection_status_changed_event);
    } else {
        ASSERT(connection_status.status == static_cast<u32>(NetworkStatus::NotConnected));
        auto logoff = ParseEAPoLLogoffFrame(packet.data);

        network_info.total_nodes = logoff.connected_nodes;
        network_info.max_nodes = logoff.max_nodes;

        connection_status.network_node_id = logoff.assigned_node_id;
        connection_status.total_nodes = logoff.connected_nodes;
        connection_status.max_nodes = logoff.max_nodes;

        node_info.clear();
        node_info.reserve(network_info.max_nodes);
        for (size_t index = 0; index < logoff.connected_nodes; ++index) {
            connection_status.node_bitmask |= 1 << index;
            connection_status.changed_nodes |= 1 << index;
            connection_status.nodes[index] = logoff.nodes[index].network_node_id;

            node_info.emplace_back(DeserializeNodeInfo(logoff.nodes[index]));
        }

        // We're now connected, signal the application
        connection_status.status = static_cast<u32>(NetworkStatus::ConnectedAsClient);
        connection_status.state = static_cast<u32>(ConnectionState::Connected);
    }
}

static void HandleSecureDataPacket(const Network::WifiPacket& packet) {
    auto secure_data = ParseSecureDataHeader(packet.data);

    std::lock_guard<std::mutex> lock(connection_status_mutex);

    if (secure_data.src_node_id == connection_status.network_node_id) {
        // Ignore packets that came from ourselves.
        return;
    }

    if (secure_data.dest_node_id != connection_status.network_node_id &&
        secure_data.dest_node_id != BroadcastNetworkNodeId) {
        // The packet wasn't addressed to us, we can only act as a router if we're the host.
        // However, we might have received this packet due to a broadcast from the host, in that
        // case just ignore it.
        ASSERT_MSG(packet.destination_address == Network::BroadcastMac ||
                       connection_status.status == static_cast<u32>(NetworkStatus::ConnectedAsHost),
                   "Can't be a router if we're not a host");

        if (connection_status.status == static_cast<u32>(NetworkStatus::ConnectedAsHost) &&
            secure_data.dest_node_id != BroadcastNetworkNodeId) {
            // Broadcast the packet so the right receiver can get it.
            // TODO(Subv): Is there a flag that makes this kind of routing be unicast instead of
            // multicast? Perhaps this is a way to allow spectators to see some of the packets.
            Network::WifiPacket out_packet = packet;
            out_packet.destination_address = Network::BroadcastMac;
            SendPacket(out_packet);
        }
        return;
    }

    // The packet is addressed to us (or to everyone using the broadcast node id), handle it.
    // TODO(Subv): We don't currently send nor handle management frames.
    ASSERT(!secure_data.is_management);

    // TODO(Subv): Allow more than one bind node per channel.
    auto channel_info = channel_data.find(secure_data.data_channel);
    // Ignore packets from channels we're not interested in.
    if (channel_info == channel_data.end())
        return;

    if (channel_info->second.network_node_id != BroadcastNetworkNodeId &&
        channel_info->second.network_node_id != secure_data.src_node_id)
        return;

    // Add the received packet to the data queue.
    channel_info->second.received_packets.emplace_back(packet.data);

    // Signal the data event. We can't do this directly because this function is called from the
    // network thread.
    // LOG_CRITICAL(Network, "Scheduling bind node event signal");
    CoreTiming::ScheduleEvent_Threadsafe_Immediate(channel_info->second.signal_event_helper,
                                                   secure_data.data_channel);
}

static void HandleDataFrame(const Network::WifiPacket& packet) {
    switch (GetFrameEtherType(packet.data)) {
    case EtherType::EAPoL:
        HandleEAPoLPacket(packet);
        break;
    case EtherType::SecureData:
        HandleSecureDataPacket(packet);
        break;
    }
}

static void HandleDisconnectFrame(const Network::WifiPacket& packet) {
    LOG_ERROR(Service_NWM, "called, this will most likely fail");
    return;
    if (connection_status.status == static_cast<u8>(NetworkStatus::ConnectedAsClient)) {
        std::lock_guard<std::mutex> lock(connection_status_mutex);
        connection_status = {};
        connection_status.status = static_cast<u32>(NetworkStatus::NotConnected);
        connection_status.state = static_cast<u32>(ConnectionState::LostConnection);
        connection_status_event->Signal();
    } else if (connection_status.status == static_cast<u8>(NetworkStatus::ConnectedAsHost)) {
        // TODO(B3N30): Figure out what vars to set here
    }
}

/// Callback to parse and handle a received wifi packet.
void OnWifiPacketReceived(const Network::WifiPacket& packet) {
    switch (packet.type) {
    case Network::WifiPacket::PacketType::Beacon:
        HandleBeaconFrame(packet);
        break;
    case Network::WifiPacket::PacketType::Authentication:
        HandleAuthenticationFrame(packet);
        break;
    case Network::WifiPacket::PacketType::AssociationResponse:
        HandleAssociationResponseFrame(packet);
        break;
    case Network::WifiPacket::PacketType::Data:
        HandleDataFrame(packet);
        break;
    case Network::WifiPacket::PacketType::Disconnect:
        HandleDisconnectFrame(packet);
        break;
    }
}

/**
 * NWM_UDS::Shutdown service function
 *  Inputs:
 *      1 : None
 *  Outputs:
 *      0 : Return header
 *      1 : Result of function, 0 on success, otherwise error code
 */
static void Shutdown(Interface* self) {
    u32* cmd_buff = Kernel::GetCommandBuffer();

    if (auto room_member = Network::GetRoomMember().lock())
        room_member->Unbind(wifi_packet_received);

    for (auto data : channel_data) {
        data.second.event->Signal();
    }
    channel_data.clear();

    recv_buffer_memory.reset();
    // TODO(purpasmart): Verify return header on HW
    cmd_buff[1] = RESULT_SUCCESS.raw;

    LOG_WARNING(Service_NWM, "(STUBBED) called");
}

/**
 * NWM_UDS::RecvBeaconBroadcastData service function
 * Returns the raw beacon data for nearby networks that match the supplied WlanCommId.
 *  Inputs:
 *      1 : Output buffer max size
 *    2-3 : Unknown
 *    4-5 : Host MAC address.
 *   6-14 : Unused
 *     15 : WLan Comm Id
 *     16 : Id
 *     17 : Value 0
 *     18 : Input handle
 *     19 : (Size<<4) | 12
 *     20 : Output buffer ptr
 *  Outputs:
 *      0 : Return header
 *      1 : Result of function, 0 on success, otherwise error code
 */
static void RecvBeaconBroadcastData(Interface* self) {
    IPC::RequestParser rp(Kernel::GetCommandBuffer(), 0x0F, 16, 4);

    u32 out_buffer_size = rp.Pop<u32>();
    u32 unk1 = rp.Pop<u32>();
    u32 unk2 = rp.Pop<u32>();

    MacAddress mac_address;
    rp.PopRaw(mac_address);

    rp.Skip(9, false);

    u32 wlan_comm_id = rp.Pop<u32>();
    u32 id = rp.Pop<u32>();
    Kernel::Handle input_handle = rp.PopHandle();

    size_t desc_size;
    const VAddr out_buffer_ptr = rp.PopMappedBuffer(&desc_size);
    ASSERT(desc_size == out_buffer_size);

    VAddr current_buffer_pos = out_buffer_ptr;
    u32 total_size = sizeof(BeaconDataReplyHeader);

    // Retrieve all beacon frames that were received from the desired mac address.
    auto beacons = GetReceivedPackets(Network::WifiPacket::PacketType::Beacon, mac_address);

    BeaconDataReplyHeader data_reply_header{};
    data_reply_header.total_entries = beacons.size();
    data_reply_header.max_output_size = out_buffer_size;

    Memory::WriteBlock(current_buffer_pos, &data_reply_header, sizeof(BeaconDataReplyHeader));
    current_buffer_pos += sizeof(BeaconDataReplyHeader);

    // Write each of the received beacons into the buffer
    for (const auto& beacon : beacons) {
        BeaconEntryHeader entry{};
        // TODO(Subv): Figure out what this size is used for.
        entry.unk_size = sizeof(BeaconEntryHeader) + beacon.data.size();
        entry.total_size = sizeof(BeaconEntryHeader) + beacon.data.size();
        entry.wifi_channel = beacon.channel;
        entry.header_size = sizeof(BeaconEntryHeader);
        entry.mac_address = beacon.transmitter_address;

        ASSERT(current_buffer_pos < out_buffer_ptr + out_buffer_size);

        Memory::WriteBlock(current_buffer_pos, &entry, sizeof(BeaconEntryHeader));
        current_buffer_pos += sizeof(BeaconEntryHeader);

        Memory::WriteBlock(current_buffer_pos, beacon.data.data(), beacon.data.size());
        current_buffer_pos += beacon.data.size();

        total_size += sizeof(BeaconEntryHeader) + beacon.data.size();
    }

    // Update the total size in the structure and write it to the buffer again.
    data_reply_header.total_size = total_size;
    Memory::WriteBlock(out_buffer_ptr, &data_reply_header, sizeof(BeaconDataReplyHeader));

    IPC::RequestBuilder rb = rp.MakeBuilder(1, 0);
    rb.Push(RESULT_SUCCESS);

    LOG_DEBUG(Service_NWM, "called out_buffer_size=0x%08X, wlan_comm_id=0x%08X, id=0x%08X,"
                           "input_handle=0x%08X, out_buffer_ptr=0x%08X, unk1=0x%08X, unk2=0x%08X",
              out_buffer_size, wlan_comm_id, id, input_handle, out_buffer_ptr, unk1, unk2);
}

/**
 * NWM_UDS::Initialize service function
 *  Inputs:
 *      1 : Shared memory size
 *   2-11 : Input NodeInfo Structure
 *     12 : 2-byte Version
 *     13 : Value 0
 *     14 : Shared memory handle
 *  Outputs:
 *      0 : Return header
 *      1 : Result of function, 0 on success, otherwise error code
 *      2 : Value 0
 *      3 : Output event handle
 */
static void InitializeWithVersion(Interface* self) {
    IPC::RequestParser rp(Kernel::GetCommandBuffer(), 0x1B, 12, 2);

    u32 sharedmem_size = rp.Pop<u32>();

    // Update the node information with the data the game gave us.
    rp.PopRaw(current_node);

    u16 version = rp.Pop<u16>();

    Kernel::Handle sharedmem_handle = rp.PopHandle();

    recv_buffer_memory = Kernel::g_handle_table.Get<Kernel::SharedMemory>(sharedmem_handle);

    ASSERT_MSG(recv_buffer_memory->size == sharedmem_size, "Invalid shared memory size.");

    if (auto room_member = Network::GetRoomMember().lock()) {
        wifi_packet_received = room_member->BindOnWifiPacketReceived(OnWifiPacketReceived);
    } else {
        LOG_ERROR(Service_NWM, "Network isn't initalized");
        IPC::RequestBuilder rb = rp.MakeBuilder(1, 2);
        // TODO(B3N30): Find the correct error code and return it;
        rb.Push(RESULT_SUCCESS);
        return;
    }

    IPC::RequestBuilder rb = rp.MakeBuilder(1, 2);
    rb.Push(RESULT_SUCCESS);
    rb.PushCopyHandles(Kernel::g_handle_table.Create(connection_status_event).Unwrap());
    {
        std::lock_guard<std::mutex> lock(connection_status_mutex);

        // Reset the connection status, it contains all zeros after initialization,
        // except for the actual status value.
        connection_status = {};
        connection_status.status = static_cast<u32>(NetworkStatus::NotConnected);
        connection_status.state = static_cast<u32>(ConnectionState::NotConnected);
    }

    LOG_DEBUG(Service_NWM, "called sharedmem_size=0x%08X, version=0x%08X, sharedmem_handle=0x%08X",
              sharedmem_size, version, sharedmem_handle);
}

static void DisconnectNetwork(Interface* self) {
    LOG_ERROR(Service_NWM, "called");
    IPC::RequestParser rp(Kernel::GetCommandBuffer(), 0xA, 0, 0);
    IPC::RequestBuilder rb = rp.MakeBuilder(1, 0);

    rb.Push(RESULT_SUCCESS);
    using Network::WifiPacket;

    // TODO(B3N30): Find out what the correct packet for this is
    WifiPacket disconnect_packet;
    disconnect_packet.channel = network_channel;
    disconnect_packet.destination_address = Network::BroadcastMac;
    disconnect_packet.type = WifiPacket::PacketType::Disconnect;

    {
        std::lock_guard<std::mutex> lock(connection_status_mutex);
        disconnect_packet.data.push_back(connection_status.network_node_id);

        if (connection_status.status == static_cast<u32>(NetworkStatus::ConnectedAsHost)) {
            // A real 3ds makes starnge things here. We do the same
            u16_le tmp_node_id = connection_status.network_node_id;
            connection_status = {};
            connection_status.status = static_cast<u32>(NetworkStatus::ConnectedAsHost);
            connection_status.state = static_cast<u32>(ConnectionState::Connected);
            connection_status.network_node_id = tmp_node_id;
            LOG_ERROR(Service_NWM, "called as a host");
            return;
        }
        u16_le tmp_node_id = connection_status.network_node_id;
        connection_status = {};
        connection_status.status = static_cast<u32>(NetworkStatus::NotConnected);
        connection_status.state = static_cast<u32>(ConnectionState::Ended);
        connection_status.network_node_id = tmp_node_id;
        connection_status_event->Signal();
    }
    SendPacket(disconnect_packet);
}

/**
 * NWM_UDS::GetConnectionStatus service function.
 * Returns the connection status structure for the currently open network connection.
 * This structure contains information about the connection,
 * like the number of connected nodes, etc.
 *  Inputs:
 *      0 : Command header.
 *  Outputs:
 *      0 : Return header
 *      1 : Result of function, 0 on success, otherwise error code
 *      2-13 : Channel of the current WiFi network connection.
 */
static void GetConnectionStatus(Interface* self) {
    IPC::RequestParser rp(Kernel::GetCommandBuffer(), 0xB, 0, 0);
    IPC::RequestBuilder rb = rp.MakeBuilder(13, 0);

    rb.Push(RESULT_SUCCESS);

    {
        std::lock_guard<std::mutex> lock(connection_status_mutex);
        rb.PushRaw(connection_status);

        // Reset the bitmask of changed nodes after each call to this
        // function to prevent falsely informing games of outstanding
        // changes in subsequent calls.
        // TODO(Subv): Find exactly where the NWM module resets this value.
        connection_status.changed_nodes = 0;
    }

    LOG_DEBUG(Service_NWM, "called");
}

static void GetNodeInformation(Interface* self) {
    IPC::RequestParser rp(Kernel::GetCommandBuffer(), 0xD, 1, 0);

    u16 network_node_id = rp.Pop<u16>();

    IPC::RequestBuilder rb = rp.MakeBuilder(11, 0);

    rb.Push(RESULT_SUCCESS);

    {
        std::lock_guard<std::mutex> lock(connection_status_mutex);

        auto itr = std::find_if(node_info.begin(), node_info.end(),
                                [network_node_id](const NodeInfo& node) {
                                    return node.network_node_id == network_node_id;
                                });

        ASSERT(itr != node_info.end());

        rb.PushRaw<NodeInfo>(*itr);
    }

    LOG_DEBUG(Service_NWM, "called");
}

/**
 * NWM_UDS::Bind service function.
 * Binds a BindNodeId to a data channel and retrieves a data event.
 *  Inputs:
 *      1 : BindNodeId
 *      2 : Receive buffer size.
 *      3 : u8 Data channel to bind to.
 *      4 : Network node id.
 *  Outputs:
 *      0 : Return header
 *      1 : Result of function, 0 on success, otherwise error code
 *      2 : Copy handle descriptor.
 *      3 : Data available event handle.
 */
static void Bind(Interface* self) {
    IPC::RequestParser rp(Kernel::GetCommandBuffer(), 0x12, 4, 0);

    u32 bind_node_id = rp.Pop<u32>();
    u32 recv_buffer_size = rp.Pop<u32>();
    u8 data_channel = rp.Pop<u8>();
    u16 network_node_id = rp.Pop<u16>();

    // TODO(Subv): Store the data channel and verify it when receiving data frames.

    LOG_DEBUG(Service_NWM, "called");

    if (data_channel == 0) {
        IPC::RequestBuilder rb = rp.MakeBuilder(1, 0);
        rb.Push(ResultCode(ErrorDescription::NotAuthorized, ErrorModule::UDS,
                           ErrorSummary::WrongArgument, ErrorLevel::Usage));
        return;
    }

    // Create a new event for this bind node.
    auto event = Kernel::Event::Create(Kernel::ResetType::OneShot,
                                       "NWM::BindNodeEvent" + std::to_string(bind_node_id));

    std::lock_guard<std::mutex> lock(connection_status_mutex);

    ASSERT(channel_data.find(data_channel) == channel_data.end());

    char* name = new char[300];
    sprintf(name, "NWM::BindNodeEvent%d", bind_node_id);

    int signal_helper = CoreTiming::RegisterEvent(name, [](u64 channel, int cycles_late) {
        std::lock_guard<std::mutex> lock(connection_status_mutex);
        // TODO(Subv): Support more than one bind node per channel.
        auto itr = channel_data.find(channel);
        if (itr == channel_data.end())
            return;

        // LOG_CRITICAL(Network, "Signaling bind node event");
        channel_data[channel].event->Signal();
    });

    channel_data[data_channel] = {bind_node_id, data_channel, network_node_id, event,
                                  signal_helper};

    IPC::RequestBuilder rb = rp.MakeBuilder(1, 2);

    rb.Push(RESULT_SUCCESS);
    rb.PushCopyHandles(Kernel::g_handle_table.Create(event).Unwrap());
}

static void Unbind(Interface* self) {
    IPC::RequestParser rp(Kernel::GetCommandBuffer(), 0x12, 1, 0);

    u32 bind_node_id = rp.Pop<u32>();

    std::lock_guard<std::mutex> lock(connection_status_mutex);

    auto itr =
        std::find_if(channel_data.begin(), channel_data.end(), [bind_node_id](const auto& data) {
            return data.second.bind_node_id == bind_node_id;
        });

    if (itr == channel_data.end()) {
        IPC::RequestBuilder rb = rp.MakeBuilder(1, 0);
        rb.Push(RESULT_SUCCESS);
        return;
    }

    channel_data.erase(itr);
    IPC::RequestBuilder rb = rp.MakeBuilder(1, 0);
    rb.Push(RESULT_SUCCESS);
}

/**
 * NWM_UDS::BeginHostingNetwork service function.
 * Creates a network and starts broadcasting its presence.
 *  Inputs:
 *      1 : Passphrase buffer size.
 *      3 : VAddr of the NetworkInfo structure.
 *      5 : VAddr of the passphrase.
 *  Outputs:
 *      0 : Return header
 *      1 : Result of function, 0 on success, otherwise error code
 */
static void BeginHostingNetwork(Interface* self) {
    IPC::RequestParser rp(Kernel::GetCommandBuffer(), 0x1D, 1, 4);

    const u32 passphrase_size = rp.Pop<u32>();

    size_t desc_size;
    const VAddr network_info_address = rp.PopStaticBuffer(&desc_size, false);
    ASSERT(desc_size == sizeof(NetworkInfo));
    const VAddr passphrase_address = rp.PopStaticBuffer(&desc_size, false);
    ASSERT(desc_size == passphrase_size);

    // TODO(Subv): Store the passphrase and verify it when attempting a connection.

    LOG_DEBUG(Service_NWM, "called");

    {
        std::lock_guard<std::mutex> lock(connection_status_mutex);

        Memory::ReadBlock(network_info_address, &network_info, sizeof(NetworkInfo));

        // The real UDS module throws a fatal error if this assert fails.
        ASSERT_MSG(network_info.max_nodes > 1, "Trying to host a network of only one member.");

        connection_status.status = static_cast<u32>(NetworkStatus::ConnectedAsHost);
        connection_status.state = static_cast<u32>(ConnectionState::Connected);

        // Ensure the application data size is less than the maximum value.
        ASSERT_MSG(network_info.application_data_size <= ApplicationDataSize,
                   "Data size is too big.");

        // Set up basic information for this network.
        network_info.oui_value = NintendoOUI;
        network_info.oui_type = static_cast<u8>(NintendoTagId::NetworkInfo);

        connection_status.max_nodes = network_info.max_nodes;

        // Resize the nodes list to hold max_nodes.
        node_info.clear();
        node_info.resize(network_info.max_nodes);

        // There's currently only one node in the network (the host).
        connection_status.total_nodes = 1;
        network_info.total_nodes = 1;
        // The host is always the first node
        connection_status.network_node_id = 1;
        current_node.network_node_id = 1;
        connection_status.nodes[0] = connection_status.network_node_id;
        // Set the bit 0 in the nodes bitmask to indicate that node 1 is already taken.
        connection_status.node_bitmask |= 1;
        // Notify the application that the first node was set.
        connection_status.changed_nodes |= 1;

        if (auto room_member = Network::GetRoomMember().lock()) {
            if (room_member->IsConnected()) {
                network_info.host_mac_address = room_member->GetMacAddress();
            } else {
                network_info.host_mac_address = {{0x0,0x0,0x0,0x0,0x0,0x0}};
            }
        }

        current_node.address = network_info.host_mac_address;
        node_info[0] = current_node;

        // If the game has a preferred channel, use that instead.
        if (network_info.channel != 0)
            network_channel = network_info.channel;
        else
            network_info.channel = DefaultNetworkChannel;
    }

    connection_status_event->Signal();

    // Start broadcasting the network, send a beacon frame every 102.4ms.
    CoreTiming::ScheduleEvent(msToCycles(DefaultBeaconInterval * MillisecondsPerTU),
                              beacon_broadcast_event, 0);

    LOG_WARNING(Service_NWM, "An UDS network has been created.");

    IPC::RequestBuilder rb = rp.MakeBuilder(1, 0);
    rb.Push(RESULT_SUCCESS);
}

/**
 * NWM_UDS::DestroyNetwork service function.
 * Closes the network that we're currently hosting.
 *  Inputs:
 *      0 : Command header.
 *  Outputs:
 *      0 : Return header
 *      1 : Result of function, 0 on success, otherwise error code
 */
static void DestroyNetwork(Interface* self) {
    IPC::RequestParser rp(Kernel::GetCommandBuffer(), 0x08, 0, 0);

    // Unschedule the beacon broadcast event.
    CoreTiming::UnscheduleEvent(beacon_broadcast_event, 0);

    // Only a host can destroy
    std::lock_guard<std::mutex> lock(connection_status_mutex);
    if (connection_status.status != static_cast<u8>(NetworkStatus::ConnectedAsHost)) {
        IPC::RequestBuilder rb = rp.MakeBuilder(1, 0);
        rb.Push(RESULT_SUCCESS);
        LOG_WARNING(Service_NWM, "called with status %u", connection_status.status);
        return;
    }

    LOG_DEBUG(Service_NWM, "called");

    using Network::WifiPacket;
    WifiPacket disconnect_packet;
    disconnect_packet.channel = network_channel;
    disconnect_packet.destination_address = Network::BroadcastMac;
    disconnect_packet.type = WifiPacket::PacketType::Disconnect;
    disconnect_packet.data.push_back(connection_status.network_node_id);
    SendPacket(disconnect_packet);

    u16_le tmp_node_id = connection_status.network_node_id;
    connection_status = {};
    connection_status.status = static_cast<u32>(NetworkStatus::NotConnected);
    connection_status.network_node_id = tmp_node_id;
    connection_status.state = static_cast<u32>(ConnectionState::Ended);
    connection_status_event->Signal();

    IPC::RequestBuilder rb = rp.MakeBuilder(1, 0);

    for (auto data : channel_data) {
        data.second.event->Signal();
    }
    channel_data.clear();

    rb.Push(RESULT_SUCCESS);

    LOG_WARNING(Service_NWM, "called");
}

/**
 * NWM_UDS::SendTo service function.
 * Sends a data frame to the UDS network we're connected to.
 *  Inputs:
 *      0 : Command header.
 *      1 : Unknown.
 *      2 : u16 Destination network node id.
 *      3 : u8 Data channel.
 *      4 : Buffer size >> 2
 *      5 : Data size
 *      6 : Flags
 *      7 : Input buffer descriptor
 *      8 : Input buffer address
 *  Outputs:
 *      0 : Return header
 *      1 : Result of function, 0 on success, otherwise error code
 */
static void SendTo(Interface* self) {
    IPC::RequestParser rp(Kernel::GetCommandBuffer(), 0x17, 6, 2);

    rp.Skip(1, false);
    u16 dest_node_id = rp.Pop<u16>();
    u8 data_channel = rp.Pop<u8>();
    rp.Skip(1, false);
    u32 data_size = rp.Pop<u32>();
    u32 flags = rp.Pop<u32>();

    size_t desc_size;
    const VAddr input_address = rp.PopStaticBuffer(&desc_size, false);
    ASSERT(desc_size >= data_size);

    IPC::RequestBuilder rb = rp.MakeBuilder(1, 0);

    /*LOG_WARNING(Service_NWM, "(STUB) called dest_node_id=%u size=%u flags=%u channel=%u",
                static_cast<u32>(dest_node_id), data_size, flags, static_cast<u32>(data_channel));*/

    std::lock_guard<std::mutex> lock(connection_status_mutex);

    if (connection_status.status != static_cast<u32>(NetworkStatus::ConnectedAsClient) &&
        connection_status.status != static_cast<u32>(NetworkStatus::ConnectedAsHost)) {
        rb.Push(ResultCode(ErrorDescription::NotAuthorized, ErrorModule::UDS,
                           ErrorSummary::InvalidState, ErrorLevel::Status));
        return;
    }

    if (dest_node_id == connection_status.network_node_id) {
        rb.Push(ResultCode(ErrorDescription::NotFound, ErrorModule::UDS,
                           ErrorSummary::WrongArgument, ErrorLevel::Status));
        return;
    }

    u16 network_node_id = connection_status.network_node_id;

    // TODO(Subv): Do something with the flags.

    constexpr size_t MaxSize = 0x5C6;
    if (data_size > MaxSize) {
        rb.Push(ResultCode(ErrorDescription::TooLarge, ErrorModule::UDS,
                           ErrorSummary::WrongArgument, ErrorLevel::Usage));
        return;
    }

    std::vector<u8> data(data_size);
    Memory::ReadBlock(input_address, data.data(), data.size());

    // TODO(Subv): Increment the sequence number after each sent packet.
    u16 sequence_number = 0;
    std::vector<u8> data_payload =
        GenerateDataPayload(data, data_channel, dest_node_id, network_node_id, sequence_number);

    // TODO(Subv): Retrieve the MAC address of the dest_node_id and our own to encrypt
    // and encapsulate the payload.

    Network::WifiPacket packet;
    // Data frames are sent to the host, who then decides where to route it to. If we're the host,
    // just directly broadcast the frame.
    if (connection_status.status == static_cast<u32>(NetworkStatus::ConnectedAsHost))
        packet.destination_address = Network::BroadcastMac;
    else
        packet.destination_address = network_info.host_mac_address;
    packet.channel = network_channel;
    packet.data = std::move(data_payload);
    packet.type = Network::WifiPacket::PacketType::Data;

    SendPacket(packet);

    rb.Push(RESULT_SUCCESS);
}

/**
 * NWM_UDS::PullPacket service function.
 * Receives a data frame from the specified bind node id
 *  Inputs:
 *      0 : Command header.
 *      1 : Bind node id.
 *      2 : Max out buff size >> 2.
 *      3 : Max out buff size.
 *     64 : Output buffer descriptor
 *     65 : Output buffer address
 *  Outputs:
 *      0 : Return header
 *      1 : Result of function, 0 on success, otherwise error code
 *      2 : Received data size
 *      3 : u16 Source network node id
 *      4 : Buffer descriptor
 *      5 : Buffer address
 */
static void PullPacket(Interface* self) {
    IPC::RequestParser rp(Kernel::GetCommandBuffer(), 0x14, 3, 0);

    u32 bind_node_id = rp.Pop<u32>();
    u32 max_out_buff_size_aligned = rp.Pop<u32>();
    u32 max_out_buff_size = rp.Pop<u32>();

    size_t desc_size;
    const VAddr output_address = rp.PeekStaticBuffer(0, &desc_size);
    ASSERT(desc_size == max_out_buff_size);

    // LOG_WARNING(Service_NWM, "(STUB) called");

    std::lock_guard<std::mutex> lock(connection_status_mutex);

    auto channel =
        std::find_if(channel_data.begin(), channel_data.end(), [bind_node_id](const auto& data) {
            return data.second.bind_node_id == bind_node_id;
        });

    if (channel == channel_data.end()) {
        IPC::RequestBuilder rb = rp.MakeBuilder(1, 0);
        // TODO(Subv): Find the right error code
        rb.Push<u32>(-1);
        return;
    }

    if (channel->second.received_packets.empty()) {
        Memory::ZeroBlock(output_address, desc_size);
        IPC::RequestBuilder rb = rp.MakeBuilder(3, 2);
        rb.Push(RESULT_SUCCESS);
        rb.Push<u32>(0);
        rb.Push<u16>(0);
        rb.PushStaticBuffer(output_address, desc_size, 0);
        return;
    }

    const auto& next_packet = channel->second.received_packets.front();

    auto secure_data = ParseSecureDataHeader(next_packet);
    auto data_size = secure_data.GetActualDataSize();

    if (channel->second.received_packets.size() > 1) {
        auto& ev = channel->second.event;
        // LOG_CRITICAL(Network, "More packets, is event signaled %s", ev->signaled ? "yes" : "no");
    }

    if (data_size > max_out_buff_size) {
        IPC::RequestBuilder rb = rp.MakeBuilder(1, 0);
        rb.Push<u32>(0xE10113E9);
        return;
    }

    IPC::RequestBuilder rb = rp.MakeBuilder(3, 2);
    Memory::ZeroBlock(output_address, desc_size);
    // Write the actual data.
    Memory::WriteBlock(output_address,
                       next_packet.data() + sizeof(LLCHeader) + sizeof(SecureDataHeader),
                       data_size);

    rb.Push(RESULT_SUCCESS);
    rb.Push<u32>(data_size);
    rb.Push<u16>(secure_data.src_node_id);
    rb.PushStaticBuffer(output_address, desc_size, 0);

    channel->second.received_packets.pop_front();
}

/**
 * NWM_UDS::GetChannel service function.
 * Returns the WiFi channel in which the network we're connected to is transmitting.
 *  Inputs:
 *      0 : Command header.
 *  Outputs:
 *      0 : Return header
 *      1 : Result of function, 0 on success, otherwise error code
 *      2 : Channel of the current WiFi network connection.
 */
static void GetChannel(Interface* self) {
    IPC::RequestParser rp(Kernel::GetCommandBuffer(), 0x1A, 0, 0);
    IPC::RequestBuilder rb = rp.MakeBuilder(2, 0);

    std::lock_guard<std::mutex> lock(connection_status_mutex);

    bool is_connected = connection_status.status != static_cast<u32>(NetworkStatus::NotConnected);

    u8 channel = is_connected ? network_channel : 0;

    rb.Push(RESULT_SUCCESS);
    rb.Push(channel);

    LOG_DEBUG(Service_NWM, "called");
}

/**
 * NWM_UDS::SetApplicationData service function.
 * Updates the application data that is being broadcast in the beacon frames
 * for the network that we're hosting.
 *  Inputs:
 *      1 : Data size.
 *      3 : VAddr of the data.
 *  Outputs:
 *      0 : Return header
 *      1 : Result of function, 0 on success, otherwise error code
 *      2 : Channel of the current WiFi network connection.
 */
static void SetApplicationData(Interface* self) {
    IPC::RequestParser rp(Kernel::GetCommandBuffer(), 0x1A, 1, 2);

    u32 size = rp.Pop<u32>();

    size_t desc_size;
    const VAddr address = rp.PopStaticBuffer(&desc_size, false);
    ASSERT(desc_size == size);

    LOG_DEBUG(Service_NWM, "called");

    IPC::RequestBuilder rb = rp.MakeBuilder(1, 0);

    if (size > ApplicationDataSize) {
        rb.Push(ResultCode(ErrorDescription::TooLarge, ErrorModule::UDS,
                           ErrorSummary::WrongArgument, ErrorLevel::Usage));
        return;
    }

    network_info.application_data_size = size;
    Memory::ReadBlock(address, network_info.application_data.data(), size);

    rb.Push(RESULT_SUCCESS);
}

static void ConnectToNetwork(Interface* self) {
    IPC::RequestParser rp(Kernel::GetCommandBuffer(), 0x1E, 2, 4);

    u8 connection_type = rp.Pop<u8>();
    u32 passphrase_size = rp.Pop<u32>();

    size_t desc_size;
    const VAddr network_struct_addr = rp.PopStaticBuffer(&desc_size);
    ASSERT(desc_size == sizeof(NetworkInfo));

    size_t passphrase_desc_size;
    const VAddr passphrase_addr = rp.PopStaticBuffer(&passphrase_desc_size);

    Memory::ReadBlock(network_struct_addr, &network_info, sizeof(network_info));

    // Start the connection sequence
    StartConnectionSequence(network_info.host_mac_address);

    while (true) {
        std::lock_guard<std::mutex> lock(connection_status_mutex);
        // TODO(B3N30): Handle when connection failed, e.g. Host is full
        // Return error 0xc8611001 when host is full
        if (connection_status.status == static_cast<u32>(NetworkStatus::ConnectedAsClient))
            break;
    }

    connection_status_event->Signal();

    IPC::RequestBuilder rb = rp.MakeBuilder(1, 0);
    rb.Push(RESULT_SUCCESS);

    LOG_WARNING(Service_NWM, "called");
}

/**
 * NWM_UDS::DecryptBeaconData service function.
 * Decrypts the encrypted data tags contained in the 802.11 beacons.
 *  Inputs:
 *      1 : Input network struct buffer descriptor.
 *      2 : Input network struct buffer ptr.
 *      3 : Input tag0 encrypted buffer descriptor.
 *      4 : Input tag0 encrypted buffer ptr.
 *      5 : Input tag1 encrypted buffer descriptor.
 *      6 : Input tag1 encrypted buffer ptr.
 *     64 : Output buffer descriptor.
 *     65 : Output buffer ptr.
 *  Outputs:
 *      0 : Return header
 *      1 : Result of function, 0 on success, otherwise error code
 */
static void DecryptBeaconData(Interface* self) {
    IPC::RequestParser rp(Kernel::GetCommandBuffer(), 0x1F, 0, 6);

    size_t desc_size;
    const VAddr network_struct_addr = rp.PopStaticBuffer(&desc_size);
    ASSERT(desc_size == sizeof(NetworkInfo));

    size_t data0_size;
    const VAddr encrypted_data0_addr = rp.PopStaticBuffer(&data0_size);

    size_t data1_size;
    const VAddr encrypted_data1_addr = rp.PopStaticBuffer(&data1_size);

    size_t output_buffer_size;
    const VAddr output_buffer_addr = rp.PeekStaticBuffer(0, &output_buffer_size);

    // This size is hardcoded in the 3DS UDS code.
    ASSERT(output_buffer_size == sizeof(NodeInfo) * UDSMaxNodes);

    LOG_WARNING(Service_NWM, "called in0=%08X in1=%08X out=%08X", encrypted_data0_addr,
                encrypted_data1_addr, output_buffer_addr);

    NetworkInfo net_info;
    Memory::ReadBlock(network_struct_addr, &net_info, sizeof(net_info));

    // Read the encrypted data.
    // The first 4 bytes should be the OUI and the OUI Type of the tags.
    std::array<u8, 3> oui;
    Memory::ReadBlock(encrypted_data0_addr, oui.data(), oui.size());
    ASSERT_MSG(oui == NintendoOUI, "Unexpected OUI");
    // TODO(Subv): The second tag data can be all zeros.
    // Memory::ReadBlock(encrypted_data1_addr, oui.data(), oui.size());
    // ASSERT_MSG(oui == NintendoOUI, "Unexpected OUI");

    ASSERT_MSG(Memory::Read8(encrypted_data0_addr + 3) ==
                   static_cast<u8>(NintendoTagId::EncryptedData0),
               "Unexpected tag id");
    /*ASSERT_MSG(Memory::Read8(encrypted_data1_addr + 3) ==
                   static_cast<u8>(NintendoTagId::EncryptedData1),
               "Unexpected tag id");*/

    std::vector<u8> beacon_data(data0_size + data1_size);
    Memory::ReadBlock(encrypted_data0_addr + 4, beacon_data.data(), data0_size);
    Memory::ReadBlock(encrypted_data1_addr + 4, beacon_data.data() + data0_size, data1_size);

    // Decrypt the data
    DecryptBeaconData(net_info, beacon_data);

    // The beacon data header contains the MD5 hash of the data.
    BeaconData beacon_header;
    std::memcpy(&beacon_header, beacon_data.data(), sizeof(beacon_header));

    std::array<u8, CryptoPP::MD5::DIGESTSIZE> hash;
    CryptoPP::MD5().CalculateDigest(hash.data(), beacon_data.data() + offsetof(BeaconData, bitmask),
                                    sizeof(BeaconNodeInfo) * net_info.max_nodes +
                                        sizeof(beacon_header.bitmask));

    if (beacon_header.md5_hash != hash) {
        // TODO(Subv): Return 0xE1211005 here
    }

    u8 num_nodes = net_info.max_nodes;

    std::vector<NodeInfo> nodes;

    for (int i = 0; i < num_nodes; ++i) {
        BeaconNodeInfo info;
        std::memcpy(&info, beacon_data.data() + sizeof(beacon_header) + i * sizeof(info),
                    sizeof(info));

        // Deserialize the node information.
        NodeInfo node{};
        node.friend_code_seed = info.friend_code_seed;
        node.network_node_id = info.network_node_id;
        for (int i = 0; i < info.username.size(); ++i)
            node.username[i] = info.username[i];

        nodes.push_back(node);
    }

    Memory::ZeroBlock(output_buffer_addr, sizeof(NodeInfo) * UDSMaxNodes);
    Memory::WriteBlock(output_buffer_addr, nodes.data(), sizeof(NodeInfo) * nodes.size());

    IPC::RequestBuilder rb = rp.MakeBuilder(1, 2);
    rb.PushStaticBuffer(output_buffer_addr, output_buffer_size, 0);
    rb.Push(RESULT_SUCCESS);
}

// Sends a 802.11 beacon frame with information about the current network.
static void BeaconBroadcastCallback(u64 userdata, int cycles_late) {
    std::lock_guard<std::mutex> lock(connection_status_mutex);

    // Don't do anything if we're not actually hosting a network
    if (connection_status.status != static_cast<u32>(NetworkStatus::ConnectedAsHost))
        return;

    std::vector<u8> frame = GenerateBeaconFrame(network_info, node_info);

    using Network::WifiPacket;
    WifiPacket packet;
    packet.type = WifiPacket::PacketType::Beacon;
    packet.data = std::move(frame);
    packet.destination_address = Network::BroadcastMac;
    packet.channel = network_channel;

    SendPacket(packet);

    // Start broadcasting the network, send a beacon frame every 102.4ms.
    CoreTiming::ScheduleEvent(msToCycles(DefaultBeaconInterval * MillisecondsPerTU) - cycles_late,
                              beacon_broadcast_event, 0);
}

const Interface::FunctionInfo FunctionTable[] = {
    {0x000102C2, nullptr, "Initialize (deprecated)"},
    {0x00020000, nullptr, "Scrap"},
    {0x00030000, Shutdown, "Shutdown"},
    {0x00040402, nullptr, "CreateNetwork (deprecated)"},
    {0x00050040, nullptr, "EjectClient"},
    {0x00060000, nullptr, "EjectSpectator"},
    {0x00070080, nullptr, "UpdateNetworkAttribute"},
    {0x00080000, DestroyNetwork, "DestroyNetwork"},
    {0x00090442, nullptr, "ConnectNetwork (deprecated)"},
    {0x000A0000, DisconnectNetwork, "DisconnectNetwork"},
    {0x000B0000, GetConnectionStatus, "GetConnectionStatus"},
    {0x000D0040, GetNodeInformation, "GetNodeInformation"},
    {0x000E0006, nullptr, "DecryptBeaconData (deprecated)"},
    {0x000F0404, RecvBeaconBroadcastData, "RecvBeaconBroadcastData"},
    {0x00100042, SetApplicationData, "SetApplicationData"},
    {0x00110040, nullptr, "GetApplicationData"},
    {0x00120100, Bind, "Bind"},
    {0x00130040, Unbind, "Unbind"},
    {0x001400C0, PullPacket, "PullPacket"},
    {0x00150080, nullptr, "SetMaxSendDelay"},
    {0x00170182, SendTo, "SendTo"},
    {0x001A0000, GetChannel, "GetChannel"},
    {0x001B0302, InitializeWithVersion, "InitializeWithVersion"},
    {0x001D0044, BeginHostingNetwork, "BeginHostingNetwork"},
    {0x001E0084, ConnectToNetwork, "ConnectToNetwork"},
    {0x001F0006, DecryptBeaconData, "DecryptBeaconData"},
    {0x00200040, nullptr, "Flush"},
    {0x00210080, nullptr, "SetProbeResponseParam"},
    {0x00220402, nullptr, "ScanOnConnection"},
};

NWM_UDS::NWM_UDS() {
    connection_status_event =
        Kernel::Event::Create(Kernel::ResetType::OneShot, "NWM::connection_status_event");

    Register(FunctionTable);

    beacon_broadcast_event =
        CoreTiming::RegisterEvent("UDS::BeaconBroadcastCallback", BeaconBroadcastCallback);

    connection_status_changed_event = CoreTiming::RegisterEvent(
        "UDS::ConnectionStatusChanged", [](u64 userdata, int cycles_late) {
            connection_status_event->Signal();
            LOG_CRITICAL(Network, "Event triggered");
        });
}

NWM_UDS::~NWM_UDS() {
    network_info = {};
    channel_data.clear();
    connection_status_event = nullptr;
    recv_buffer_memory = nullptr;

    {
        std::lock_guard<std::mutex> lock(connection_status_mutex);
        connection_status = {};
        connection_status.status = static_cast<u32>(NetworkStatus::NotConnected);
        connection_status.state = static_cast<u32>(ConnectionState::NotConnected);
    }

    CoreTiming::UnscheduleEvent(beacon_broadcast_event, 0);
}

} // namespace NWM
} // namespace Service
