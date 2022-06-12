#include "DreamcastSubNode.hpp"

DreamcastSubNode::DreamcastSubNode(uint8_t addr, MapleBus& bus, uint32_t playerIndex) :
    DreamcastNode(addr)
{
    // TODO
}

DreamcastSubNode::DreamcastSubNode(const DreamcastSubNode& rhs) :
    DreamcastNode(rhs)
{
    // TODO
}

bool DreamcastSubNode::handleData(uint8_t len,
                        uint8_t cmd,
                        const uint32_t *payload)
{
    // TODO
    return false;
}

void DreamcastSubNode::task(uint64_t currentTimeUs)
{
    // TODO
}

void DreamcastSubNode::mainPeripheralDisconnected()
{
    mPeripherals.clear();
}

void DreamcastSubNode::setConnected(bool connected)
{
    // TODO
}