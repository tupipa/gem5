/******************************************************************

Lele Ma, a tag controller sits between L2 and Memory:

1. forwards all data request from l2 to memory, without processing

2. each data request additionally generates a tag memory request to
   tag cache, and a tag cache miss will also send request to mem.

One cpu side port, and two memside port.

###########################################################################
## Define a TagController
#
#  L1 -- L2 -- TagController -- Memory
#
#  TagController:
#
#  One input from L2; internal two path(tag + data); merged path to memory
#
#  L2 -> request -> |  tag controller                 | --> Memory bus
#                <- |-> data request forword -------->| <--
#                   |-> tag request ->|  tag cache    |
#                                     |-> tag req   ->|
#
#
***************************************************************************/

#include "learning_gem5/tupipa/tag_controller.hh"

#include "debug/TagController.hh"

TagController::TagController(TagControllerParams *params) :
    SimObject(params),
    tagCache(params->tag_cache),
    dataPort(params->name + ".data_port", this),
    memDataPort(params->name + ".mem_side_data", this),
    memTagPort(params->name + ".mem_side_tag", this),
    blocked(false)
{
    //Lele: connect the tag cache port with memory bus
    // memTagPort = tagCache.slave

    // tag addr is first half (0x00 -- range/2)
    // data addr is second half (range/2 -- range)

    has_max_set = false;
    _data_max_addr = 0x0;
    _tag_max_addr = 0x0;

}

bool
TagController::initDataTagBoundary(){

    _data_max_addr = 0x0;
    int ii = 0;
    for (auto range : memDataPort.getAddrRanges()){
        if (_data_max_addr < range.end()){
           _data_max_addr = range.end();
        }
        DPRINTF(TagController,
          "End Addr in range[%d]: 0x%lx\n", ii++, _data_max_addr);

   }

   DPRINTF(TagController,
     "Max Addr: 0x%lx\n", _data_max_addr);

   _tag_max_addr = _data_max_addr >> 1;
   _data_tag_address_diff = _data_max_addr - _tag_max_addr;


   // Flagging the _tag/data_max_addr are availabe for use
   has_max_set = true;

   DPRINTF(TagController,
      "TagController tag/data max address set(0x%lx/0x%x)!\n",
      _tag_max_addr, _data_max_addr);

   return true;


 }

// #define MOST_SIG_BIT (1UL<<63)

bool
TagController::isTagAddr(Addr addr){

   if (!has_max_set){
      initDataTagBoundary();
   }
   if (addr >= _tag_max_addr){
       return false;
   }
   return true;
}

Port &
TagController::getPort(const std::string &if_name, PortID idx)
{
    panic_if(idx != InvalidPortID, "This object doesn't support vector ports");

    //This is the name from the Python SimObject declaration (TagController.py)
    if (if_name == "mem_side_data") {
        return memDataPort;
    } else if (if_name == "mem_side_tag") {
        return memTagPort;
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
        DPRINTF(TagController,
          "WARNING: Cannot handle request while blocked: %s\n", pkt->print());
        return false;
    }

    // Lele:
    // - check valid data address
    // - convert the pkt address
    // - check short cut route (via compression) (todo)
    // - If no short cut, then send it to memTagPort
    //    - duplicate the packet
    //    - update its physical address
    //    - send the packet via memTagPort

    if (isTagAddr(pkt->getAddr())){

        DPRINTF(TagController,
          "WARNING: Invalid Request for Tag Addr %#x\n", pkt->getAddr());
        DPRINTF(TagController,
          "WARNING: Now drop it without proceeding\n");

        // assume is handled?
        //   return true;

        // Do not handle it.
        // This acts as if tag controller is blocked.
        // Will the caller resend the packet?
        return false;
    }else{
        DPRINTF(TagController,
          "Got valid request for addr %#x\n", pkt->getAddr());
    }
    // Lele: check needResponse()
    // if response needed, block the tag controller to wait for
    // the response to this packet.
    if (pkt->needsResponse()){
       blocked = true;
    }else{
       DPRINTF(TagController,
          "No response needed, so no blocking %s\n", pkt->print());
    }

    Addr newAddr = pkt->getAddr() - _data_tag_address_diff;

    DPRINTF(TagController,
        "Request Tag Addr %#x\n", newAddr);
    ///////////////////////////////////////////////////
    //   Here we should add our opt
    //   TODO:
    //   1. grouping fix size tags
    ////////////////////////////////////////////////////

    // Now just forward without compression

    // Create a new request
    PacketPtr tag_pkt = new Packet(pkt, false, true);
    // new request with new addr.
    RequestPtr tag_req = std::make_shared<Request>(*(tag_pkt->req));
    tag_req->setPaddr(newAddr);
    tag_pkt->req = tag_req;
    tag_pkt->setAddr(newAddr);

    memTagPort.sendPacket(tag_pkt);
    tag_is_ready = false;

    DPRINTF(TagController,
        "Tag Packet Sent %#x\n", newAddr);

    // Also forward data request to the memory port
    memDataPort.sendPacket(pkt);
    data_is_ready = false;

    DPRINTF(TagController,
        "Data Packet Sent %#x\n", pkt->getAddr());

    return true;
}

// Combine the tag and data packet for response
PacketPtr
TagController::combineRespPackets(){
    // Ignore tag packet right now since only simulation
    PacketPtr pkt = data_resp_pkt;
    return pkt;
}


bool
TagController::handleResponse(PacketPtr pkt)
{
    assert(blocked);

    // Lele:
    // Check the type of the response, and set the corresponding flag

    Addr reqAddr = pkt->getAddr();

    if (isTagAddr(reqAddr)){
        DPRINTF(TagController, "Got response for tag addr %#x\n",
                               pkt->getAddr());
        tag_is_ready = true;
        tag_resp_pkt = pkt;
    }else{
        DPRINTF(TagController, "Got response for data addr %#x\n",
                               pkt->getAddr());
        data_is_ready = true;
        data_resp_pkt = pkt;
    }
    // If both tag and data is ready,
    // forward to the memory port
    if (tag_is_ready && data_is_ready)
    {
      // The packet is now done. We're about to put it in the port,
      // no need for this object to continue to stall.
      // We need to free the resource before sending the packet in
      // case the CPU tries to send another request immediately
      // (e.g., in the same callchain).
      blocked = false;


      // Lele: combine both tag and data into one packet
      // Send it back
      pkt = combineRespPackets();

      dataPort.sendPacket(pkt);
      // For each of the cpu ports, if it needs to send a retry, it should do
      // now since this memory object may be unblocked now.
      dataPort.trySendRetry();

    }

    return true;
}

void
TagController::handleFunctional(PacketPtr pkt)
{
    // Just pass this on to the memory side to handle for now.
    memDataPort.sendFunctional(pkt);
}

AddrRangeList
TagController::getAddrRanges() const
{

    AddrRangeList ranges = memDataPort.getAddrRanges();

    DPRINTF(TagController, "Sending new ranges:\n");
    // Just use the same ranges as whatever is on the memory side.
    for (auto i : ranges){
        DPRINTF(TagController, "\t%s\n", i.to_string());
    }
    // Just use the same ranges as whatever is on the memory side.

    return ranges;
}

void
TagController::sendRangeChange()
{
    //instPort.sendRangeChange();
    dataPort.sendRangeChange();
}



TagController*
TagControllerParams::create()
{
    return new TagController(this);
}
