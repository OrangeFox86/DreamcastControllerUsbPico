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
    // 1. If device info received, add the sub peripheral
    // 2. Pass data to sub peripheral
    return false;
}

void DreamcastSubNode::task(uint64_t currentTimeUs)
{
    // TODO
    // 1. Request device info new device was newly attached
    // 2. Handle operations for peripherals (run task() of all peripherals)
}

void DreamcastSubNode::mainPeripheralDisconnected()
{
    mPeripherals.clear();
}

void DreamcastSubNode::setConnected(bool connected)
{
    // TODO
    // 1. Once someone new is detected, ask for device info
    // 2. Once something has been disconnected, clear all peripherals
}