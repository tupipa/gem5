/**

Lele Ma, a tag controller similar to CHERI's implementation:

1. forwards all data request from l2 to memory, without processing

2. each data request additionally generates a tag memory request to
   tag cache, and a tag cache miss will also send request to mem.

One cpu side port, and two memside port.

##############################
## Define a TagController
#
#  L1 -- L2 -- TagController -- Memory
#
#  TagController:
#
#  One input from L2; internal two path(tag + data); merged path to memory
#
# L2 -> request -> |  tag controller                 | --> Memory
#               <- |-> data request forword -------->| <--
#                  |-> tag request ->|  tag cache    |
#                                    |-> tag req   ->|
#
#
 */

#include "debug/TagController.hh"
#include "learning_gem5/part2/simple_memobj.hh"

TagController::TagController(TagControllerParams *params) :
    SimObject(params),
    instPort(params->name + ".inst_port", this),
    dataPort(params->name + ".data_port", this),
    memPort(params->name + ".mem_side", this),
    blocked(false)
{
}

Port &
TagController::getPort(const std::string &if_name, PortID idx)
{
    panic_if(idx != InvalidPortID, "This object doesn't support vector ports");

    //This is the name from the Python SimObject declaration (TagController.py)
    if (if_name == "mem_side") {
        return memPort;
    } else if (if_name == "inst_port") {
        return instPort;
    } else if (if_name == "data_port") {
        return dataPort;
    } else {
        // pass it along to our super class
        return SimObject::getPort(if_name, idx);
    }
}

void
TagController::CPUSidePort::sendPacket(PacketPtr pkt)
{
    // Note: This flow control is very simple since the memobj is blocking.

    panic_if(blockedPacket != nullptr, "Should never try to send if blocked!");

    DPRINTF(TagController,
            "Sending packet to CPUSidePort. %#x\n", pkt->getAddr());
    // If we can't send the packet across the port, store it for later.
    if (!sendTimingResp(pkt)) {
        blockedPacket = pkt;
    }
}

AddrRangeList
TagController::CPUSidePort::getAddrRanges() const
{
    return owner->getAddrRanges();
}

void
TagController::CPUSidePort::trySendRetry()
{
    if (needRetry && blockedPacket == nullptr) {
        // Only send a retry if the port is now completely free
        needRetry = false;
        DPRINTF(TagController, "Sending retry req for %d\n", id);
        sendRetryReq();
    }
}

void
TagController::CPUSidePort::recvFunctional(PacketPtr pkt)
{
    // Just forward to the memobj.
    return owner->handleFunctional(pkt);
}

bool
TagController::CPUSidePort::recvTimingReq(PacketPtr pkt)
{
    // Just forward to the memobj.
    if (!owner->handleRequest(pkt)) {
        needRetry = true;
        return false;
    } else {
        return true;
    }
}

void
TagController::CPUSidePort::recvRespRetry()
{
    // We should have a blocked packet if this function is called.
    assert(blockedPacket != nullptr);

    // Grab the blocked packet.
    PacketPtr pkt = blockedPacket;
    blockedPacket = nullptr;

    DPRINTF(TagController, "Re-send the response to CPUSidePort %#x\n",
                    pkt->getAddr());
    // Try to resend it. It's possible that it fails again.
    sendPacket(pkt);
}

void
TagController::MemSidePort::sendPacket(PacketPtr pkt)
{
    // Note: This flow control is very simple since the memobj is blocking.

    panic_if(blockedPacket != nullptr, "Should never try to send if blocked!");

    DPRINTF(TagController, "Sending TimingRequest to MemSidePort %#x\n",
                    pkt->getAddr());

    // If we can't send the packet across the port, store it for later.
    if (!sendTimingReq(pkt)) {
        blockedPacket = pkt;
    }
}

bool
TagController::MemSidePort::recvTimingResp(PacketPtr pkt)
{

    DPRINTF(TagController, "Get a response from MemSidePort %#x, packet:%s\n",
                    pkt->getAddr(), pkt->print());

    // Just forward to the memobj.
    return owner->handleResponse(pkt);
}

void
TagController::MemSidePort::recvReqRetry()
{
    // We should have a blocked packet if this function is called.
    assert(blockedPacket != nullptr);

    // Grab the blocked packet.
    PacketPtr pkt = blockedPacket;
    blockedPacket = nullptr;

    // Try to resend it. It's possible that it fails again.
    sendPacket(pkt);
}

void
TagController::MemSidePort::recvRangeChange()
{
    owner->sendRangeChange();
}

bool
TagController::handleRequest(PacketPtr pkt)
{
    if (blocked) {
        // There is currently an outstanding request. Stall.
        return false;
    }

    DPRINTF(TagController, "Got request for addr %#x\n", pkt->getAddr());

    // This memobj is now blocked waiting for the response to this packet.
    blocked = true;

    // Simply forward to the memory port
    memPort.sendPacket(pkt);

    return true;
}

bool
TagController::handleResponse(PacketPtr pkt)
{
    assert(blocked);
    DPRINTF(TagController, "Got response for addr %#x\n", pkt->getAddr());

    // The packet is now done. We're about to put it in the port, no need for
    // this object to continue to stall.
    // We need to free the resource before sending the packet in case the CPU
    // tries to send another request immediately (e.g., in the same callchain).
    blocked = false;

    // Simply forward to the memory port
    if (pkt->req->isInstFetch()) {
        instPort.sendPacket(pkt);
    } else {
        dataPort.sendPacket(pkt);
    }

    // For each of the cpu ports, if it needs to send a retry, it should do it
    // now since this memory object may be unblocked now.
    instPort.trySendRetry();
    dataPort.trySendRetry();

    return true;
}

void
TagController::handleFunctional(PacketPtr pkt)
{
    // Just pass this on to the memory side to handle for now.
    memPort.sendFunctional(pkt);
}

AddrRangeList
TagController::getAddrRanges() const
{
    DPRINTF(TagController, "Sending new ranges\n");
    // Just use the same ranges as whatever is on the memory side.
    return memPort.getAddrRanges();
}

void
TagController::sendRangeChange()
{
    instPort.sendRangeChange();
    dataPort.sendRangeChange();
}



TagController*
TagControllerParams::create()
{
    return new TagController(this);
}
