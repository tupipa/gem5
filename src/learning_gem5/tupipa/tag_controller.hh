
#ifndef __TUPIPA_TAG_CONTROLLER_HH
#define __TUPIPA_TAG_CONTROLLER_HH

#include "mem/cache/cache.hh"
#include "mem/port.hh"
#include "params/TagController.hh"
#include "sim/sim_object.hh"

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

class TagController : public SimObject
{
  private:

    /**
     * Port on the CPU-side that receives requests.
     * Mostly just forwards requests to the owner.
     * Lele: one port from l2 cache
     */
    class CPUSidePort : public SlavePort
    {
      private:
        /// The object that owns this object (TagController)
        TagController *owner;

        /// True if the port needs to send a retry req.
        bool needRetry;

        /// If we tried to send a packet and it was blocked, store it here
        PacketPtr blockedPacket;

      public:
        /**
         * Constructor. Just calls the superclass constructor.
         */
        CPUSidePort(const std::string& name, TagController *owner) :
            SlavePort(name, owner), owner(owner), needRetry(false),
            blockedPacket(nullptr)
        { }

        /**
         * Send a packet across this port. This is called by the owner and
         * all of the flow control is hanled in this function.
         *
         * @param packet to send.
         */
        void sendPacket(PacketPtr pkt);

        /**
         * Get a list of the non-overlapping address ranges the owner is
         * responsible for. All slave ports must override this function
         * and return a populated list with at least one item.
         *
         * @return a list of ranges responded to
         */
        AddrRangeList getAddrRanges() const override;

        /**
         * Send a retry to the peer port only if it is needed. This is called
         * from the TagController whenever it is unblocked.
         */
        void trySendRetry();

      protected:
        /**
         * Receive an atomic request packet from the master port.
         * No need to implement in this simple memobj.
         */
        Tick recvAtomic(PacketPtr pkt) override
        { panic("recvAtomic unimpl."); }

        /**
         * Receive a functional request packet from the master port.
         * Performs a "debug" access updating/reading the data in place.
         *
         * @param packet the requestor sent.
         */
        void recvFunctional(PacketPtr pkt) override;

        /**
         * Receive a timing request from the master port.
         *
         * @param the packet that the requestor sent
         * @return whether this object can consume the packet. If false, we
         *         will call sendRetry() when we can try to receive this
         *         request again.
         */
        bool recvTimingReq(PacketPtr pkt) override;

        /**
         * Called by the master port if sendTimingResp was called on this
         * slave port (causing recvTimingResp to be called on the master
         * port) and was unsuccesful.
         */
        void recvRespRetry() override;
    };

    /**
     * Port on the memory-side that receives responses.
     *
     * Two memory side port in a tag controller:
     *
     * - One used for original data request, which are forwarded to the owner
     * - One used for tag data request, happens if tag cache miss.
     *
     */
    class MemSidePort : public MasterPort
    {
      private:
        /// The object that owns this object (TagController)
        TagController *owner;

        /// If we tried to send a packet and it was blocked, store it here
        PacketPtr blockedPacket;

      public:
        /**
         * Constructor. Just calls the superclass constructor.
         */
        MemSidePort(const std::string& name, TagController *owner) :
            MasterPort(name, owner), owner(owner), blockedPacket(nullptr)
        { }

        /**
         * Send a packet across this port. This is called by the owner and
         * all of the flow control is hanled in this function.
         *
         * @param packet to send.
         */
        void sendPacket(PacketPtr pkt);

      protected:
        /**
         * Receive a timing response from the slave port.
         */
        bool recvTimingResp(PacketPtr pkt) override;

        /**
         * Called by the slave port if sendTimingReq was called on this
         * master port (causing recvTimingReq to be called on the slave
         * port) and was unsuccesful.
         */
        void recvReqRetry() override;

        /**
         * Called to receive an address range change from the peer slave
         * port. The default implementation ignores the change and does
         * nothing. Override this function in a derived class if the owner
         * needs to be aware of the address ranges, e.g. in an
         * interconnect component like a bus.
         */
        void recvRangeChange() override;
    };

    /**
     * Handle the request from the CPU side
     *
     * @param requesting packet
     * @return true if we can handle the request this cycle, false if the
     *         requestor needs to retry later
     */
    bool handleRequest(PacketPtr pkt);

    /**
     * Handle the respone from the memory side
     *
     * @param responding packet
     * @return true if we can handle the response this cycle, false if the
     *         responder needs to retry later
     */
    bool handleResponse(PacketPtr pkt);

    /**
     * Handle a packet functionally. Update the data on a write and get the
     * data on a read.
     *
     * @param packet to functionally handle
     */
    void handleFunctional(PacketPtr pkt);

    /**
     * Return the address ranges this memobj is responsible for. Just use the
     * same as the next upper level of the hierarchy.
     *
     * @return the address ranges this memobj is responsible for
     */
    AddrRangeList getAddrRanges() const;

    /**
     * Tell the CPU side to ask for our memory ranges.
     */
    void sendRangeChange();

    /**
     * Check whether the address stores tag or data
     */
    bool isTagAddr(Addr addr);

    /**
     * Combine two response packets
     * return pkt = data_resp_pkt + tag_resp_pkt
     */
    PacketPtr combineRespPackets();

    /// Instantiation of the tagCache
    Cache *tagCache;

    /// Instantiation of the CPU-side ports
    CPUSidePort dataPort;

    /// Instantiation of the memory-side port
    MemSidePort memDataPort;
    MemSidePort memTagPort;

    /// True if this is currently blocked waiting for a response.
    bool blocked;

    /// Lele: whether tag is ready to response
    bool tag_is_ready;
    /// Lele: whetehr data is ready to response
    bool data_is_ready;

    bool grouping_factor;

    // Pointers to Tag Packet for response
    PacketPtr tag_resp_pkt;
    // Pointers to Data Packet for response
    PacketPtr data_resp_pkt;

  public:

    /** constructor
     */
    TagController(TagControllerParams *params);

    /**
     * Get a port with a given name and index. This is used at
     * binding time and returns a reference to a protocol-agnostic
     * port.
     *
     * @param if_name Port name
     * @param idx Index in the case of a VectorPort
     *
     * @return A reference to the given port
     */
    Port &getPort(const std::string &if_name,
                  PortID idx=InvalidPortID) override;
};


#endif // __TUPIPA_TAG_CONTROLLER_HH
