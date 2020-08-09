


#include "learning_gem5/tupipa/simple_memobj2.hh"

#include "debug/SimpleMemobj2.hh"

SimpleMemobj2::SimpleMemobj2(SimpleMemobj2Params *params):
        MemObject(params),
        instPort(params->name + ".inst_port", this),
        dataPort(params->name + ".data_port", this),
        memPort(params->name + ".mem_side", this)
{
}


// getMasterPort, a pure virtual from MemObject
// Inputs:
//   - if_name: the python variable name of the interface for this object
//   - idx

MasterPort&
SimpleMemobj2::getMasterPort(const std::string& if_name, PortID idx)
{
    if (if_name == "mem_side"){
        return memPort;
    } else {
        // Lele: pure virt func here, what will happen?
        return MemObject::getMasterPort(if_name, idx);
    }
}

// getSlavePort, a pure virtual from MemObject
// Inputs:
//   - if_name
//   - idx
SlavePort&
SimpleMemobj2::getSlavePort(const std::string& if_name, PortID idx)
{
    if (if_name == "inst_port") {
       return instPort;
    } else if (if_name == "data_port") {
       return dataPort;
    } else {
      return MemObject::getSlavePort(if_name, idx);
    }
}



// slave port function
// just forwards the range information request to the memory object
AddrRangeList
SimpleMemobj2::CPUSidePort::getAddrRanges() const
{
    return owner->getAddrRanges();
}

// memory object function
// forwards to range request to memory port
AddrRangeList
SimpleMemobj2::getAddrRanges() const
{
    DPRINTF(SimpleMemobj, "Sending new ranges\n");
    return memPort.getAddrRanges();
}

// memory port function
// response back data to memory object (owner)
void
SimpleMemobj2::MemSidePort::recvRangeChange()
{
    owner->sendRangeChange();
}

// memory object function; response back to i/d port
void
SimpleMemobj2::sendRangeChange()
{
    instPort.sendRangeChange();
    dataPort.sendRangeChange();
}

// forwards the packet to the memory object
void
SimpleMemobj2::CPUSidePort::recvFunctional(PacketPtr pkt)
{
    return owner->handleFunctional(pkt);
}

// memory object function
// send the packet to memory port
void
SimpleMemobj2::handleFunctional(PacketPtr pkt)
{
    memPort.sendFunctional(pkt);
}


////////////////////////////////////////////
///// Recving requests /////////////////////
// A single request blocking structure //

// CPU port receiving request
bool
SimpleMemobj2::CPUSidePort::recvTimingReq(PacketPtr pkt)
{

  // A simplification of the problem: let CPUSidePort to check retry or not
  if (!owner->handleRequest(pkt)){
    needRetry = true;
    return false;
  } else {
    return true;
  }

}


// Handling request in our memory object
bool
SimpleMemobj2::handleRequest(PacketPtr pkt)
{
  if (blocked){
    return false;
  }
  DPRINTF(SimpleMemobj2, "Got request for addr %#x\n", pkt->getAddr());

  blocked = true;

  memPort.sendPacket(pkt);

  return true;

}


// Memory side port to send request down
void
SimpleMemobj2::MemSidePort::sendPacket(PacketPtr pkt)
{
    panic_if(blockedPacket != nullptr, "Should never try to send if blocked!");
    if (!sendTimingReq(pkt)) {
        blockedPacket = pkt;
    }
}

// Memory side to resend the request to slave
// might fail again.
void
SimpleMemobj2::MemSidePort::recvReqRetry()
{
    assert(blockedPacket != nullptr);

    PacketPtr pkt = blockedPacket;
    blockedPacket = nullptr;

    sendPacket(pkt);
}

// called by mem obj when it is unblocked.
// this will ask cpu to resend a request to mem obj
void
SimpleMemobj2::CPUSidePort::trySendRetry()
{
    if (needRetry && blockedPacket == nullptr) {
        needRetry = false;
        DPRINTF(SimpleMemobj2, "Sending retry req for %d\n", id);
        sendRetryReq();
    }
}



//////////////////////////////////////////////////////////
//  Receiving response                                  //

// When the memory side port got a response
// this will be called (by memory slave?)
bool
SimpleMemobj2::MemSidePort::recvTimingResp(PacketPtr pkt)
{
    return owner->handleResponse(pkt);
}


// send the response back to i/d cpu port
// unblocking before send
// notify cpu side port to resend if failed before
bool
SimpleMemobj2::handleResponse(PacketPtr pkt)
{
    assert(blocked);
    DPRINTF(SimpleMemobj2, "Got response for addr %#x\n", pkt->getAddr());

    blocked = false;

    // Simply forward to the memory port
    if (pkt->req->isInstFetch()) {
        instPort.sendPacket(pkt);
    } else {
        dataPort.sendPacket(pkt);
    }

    instPort.trySendRetry();
    dataPort.trySendRetry();

    return true;
}


// i/d cpu port to send response to cpu
// mem obj calls this
void
SimpleMemobj2::CPUSidePort::sendPacket(PacketPtr pkt)
{
    // will this panic? mem obj does not check blockedPacket before send.
    panic_if(blockedPacket != nullptr, "Should never try to send if blocked!");

    if (!sendTimingResp(pkt)) {
        blockedPacket = pkt;
    }
}

// i/d cpu port to re-send response to cpu
// again, who calls this? the cpu? (master)
void
SimpleMemobj2::CPUSidePort::recvRespRetry()
{
    assert(blockedPacket != nullptr);

    PacketPtr pkt = blockedPacket;
    blockedPacket = nullptr;

    sendPacket(pkt);
}

