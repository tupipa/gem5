
#ifndef __TUPIPA_SIMPLE_MEMOBJ_HH__
#define __TUPIPA_SIMPLE_MEMOBJ_HH__

/**
 * Header file is used in the Python script SimpleMemobj2.py
 *
 * This is the implementation of a simple memory object
 *
 * With two slave ports connecting to CPU and one port connect to memory.
 */


#include "mem/mem_object.hh"
#include "mem/port.hh"
#include "params/SimpleMemobj2.hh"

//#include "sim/sim_object.hh"

/**
 * The MemObject has two pure virtual functions, used by gem5 to connect
 * memory objects together via ports.
 *  - getMasterPort
 *  - getSlavePort
 *
 */

class SimpleMemobj2: public MemObject
{
  /**
   * A SlavePort requres five (pure virtual) functions to be defined
   *   - getAddrRanges
   *   - recvAtomic
   *   - recvFunctional
   *   - recvTimingReq
   *   - recvRespRetry
   *   All receive? Where to send?
   */
  class CPUSidePort: public SlavePort
  {
    private:
        SimpleMemobj2 *owner;

    public:
        CPUSidePort(const std::string& name, SimpleMemobj2 *owner):
            SlavePort(name, owner), owner(owner)
        { }

        AddrRangeList getAddrRanges() const override;

    protected:
        Tick recvAtomic(PacketPtr pkt) override {
           panic("recvAtomic not implemented.");
        }

        void recvFunctional(PacketPtr pkt) override;
        bool recvTimingReq(PacketPtr pkt) override;
        void recvRespRetry() override;
  };

  /**
   * A MasterPort requires 3 pure virtual functions
   *   - recvTimingResp
   *   - recvReqRetry
   *   - recvRangeChange
   *
   *   All receive? where to send?
   */

  class MemSidePort: public MasterPort
  {

     private:
        SimpleMemobj2 *owner;

     public:
        MemSidePort(const std::string& name, SimpleMemobj2 *owner):
                MasterPort(name, owner), owner(owner)
        {}

     protected:
        bool recvTimingResp(PacketPtr pkt) override;
        void recvReqRetry() override;
        void recvRangeChange() override;
  };


  private:
     CPUSidePort instPort;
     CPUSidePort dataPort;

     MemSidePort memPort;

  public:
        /**
         * constructor
        */
        SimpleMemobj2(SimpleMemobj2Params *params);

        /* MasterPort& getMasterPort(const std::string & if_name,
                        PortID idx = InvalidPortID) override;

        SlavePort& getSlavePort(const std::string& if_name,
                        PortID idx = InvalidPortID) override;

        */
        Port& getPort(const std::string& if_name,
                        PortID idx=InvalidPortID) override;

};




#endif // __TUPIPA_SIMPLE_MEMOBJ_HH__


