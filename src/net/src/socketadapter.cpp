///
//
// LibSourcey
// Copyright (c) 2005, Sourcey <http://sourcey.com>
//
// SPDX-License-Identifier: LGPL-2.1+
//
/// @addtogroup net
/// @{


#include "scy/net/socketadapter.h"
#include "scy/net/socket.h"

#include <iterator>


using std::endl;


namespace scy {
namespace net {


SocketAdapter::SocketAdapter(SocketAdapter* sender)
    : priority(0)
    , _sender(sender)
    , _dirty(false)
{
    // TraceS(this) << "Create" << endl;
    assert(sender != this);
}


SocketAdapter::~SocketAdapter()
{
    // TraceS(this) << "Destroy" << endl;
    // assert(_receivers.empty());
}


std::size_t SocketAdapter::send(const char* data, std::size_t len, int flags)
{
    assert(_sender); // should have output adapter if default impl is used
    if (!_sender)
        return -1;
    return _sender->send(data, len, flags);
}


std::size_t SocketAdapter::send(const char* data, std::size_t len, const Address& peerAddress, int flags)
{
    assert(_sender); // should have output adapter if default impl is used
    if (!_sender)
        return -1;
    return _sender->send(data, len, peerAddress, flags);
}


std::size_t SocketAdapter::sendPacket(const IPacket& packet, int flags)
{
    // Try to cast as RawPacket so we can send without copying any data.
    auto raw = dynamic_cast<const RawPacket*>(&packet);
    if (raw)
        return send((const char*)raw->data(), raw->size(), flags);

    // Dynamically generated packets need to be written to a
    // temp buffer for sending.
    else {
        Buffer buf;
        packet.write(buf);
        return send(buf.data(), buf.size(), flags);
    }
}


std::size_t SocketAdapter::sendPacket(const IPacket& packet, const Address& peerAddress, int flags)
{
    // Try to cast as RawPacket so we can send without copying any data.
    auto raw = dynamic_cast<const RawPacket*>(&packet);
    if (raw)
        return send((const char*)raw->data(), raw->size(), peerAddress, flags);

    // Dynamically generated packets need to be written to a
    // temp buffer for sending.
    else {
        Buffer buf;
        buf.reserve(2048);
        packet.write(buf);
        return send(buf.data(), buf.size(), peerAddress, flags);
    }
}


void SocketAdapter::sendPacket(IPacket& packet)
{
    int res = sendPacket(packet, 0);
    if (res < 0)
        throw std::runtime_error("Invalid socket operation");
}


void SocketAdapter::onSocketConnect(Socket& socket)
{
    try {
        int current = _receivers.size() - 1;
        while (current >= 0) {
            auto ref = _receivers[current--];
            if (ref->alive)
                ref->ptr->onSocketConnect(socket);
        }
        cleanupReceivers();
    }
    catch (StopPropagation&) {
    }
}


void SocketAdapter::onSocketRecv(Socket& socket, const MutableBuffer& buffer, const Address& peerAddress)
{
    try {
        int current = _receivers.size() - 1;
        while (current >= 0) {
            auto ref = _receivers[current--];
            if (ref->alive)
                ref->ptr->onSocketRecv(socket, buffer, peerAddress);
        }
        cleanupReceivers();
    }
    catch (StopPropagation&) {
    }
}


void SocketAdapter::onSocketError(Socket& socket, const scy::Error& error)
{
    try {
        int current = _receivers.size() - 1;
        while (current >= 0) {
            auto ref = _receivers[current--];
            if (ref->alive)
                ref->ptr->onSocketError(socket, error);
        }
        cleanupReceivers();
    }
    catch (StopPropagation&) {
    }
} 


void SocketAdapter::onSocketClose(Socket& socket)
{
    try {
        int current = _receivers.size() - 1;
        while (current >= 0) {
            auto ref = _receivers[current--];
            if (ref->alive) {
                ref->ptr->onSocketClose(socket);
            }
        }
        cleanupReceivers();
    }
    catch (StopPropagation&) {
    }
}


void SocketAdapter::setSender(SocketAdapter* adapter)
{
    assert(adapter != this);
    if (_sender == adapter)
        return;
    _sender = adapter;
}


bool SocketAdapter::hasReceiver(SocketAdapter* adapter)
{
    for (auto& receiver : _receivers) {
        if (receiver->ptr == adapter)
            return true;
    }
    return false;
}


void SocketAdapter::addReceiver(SocketAdapter* adapter)
{
    assert(adapter->priority <= 100);
    assert(adapter != this);
    if (hasReceiver(adapter))
        return;

    // Note that we insert new adapters in the back of the queue,
    // and iterate in reverse to ensure calling order is preserved.
    _dirty = true;
    _receivers.push_back(new Ref{ adapter, true });
    // _receivers.insert(_receivers.begin(), new Ref{ adapter, false }); // insert front
    // std::sort(_receivers.begin(), _receivers.end(),
    //     [](SocketAdapter const& l, SocketAdapter const& r) {
    //     return l.priority > r.priority; });
}


void SocketAdapter::removeReceiver(SocketAdapter* adapter)
{
    assert(adapter != this);
    auto it = std::find_if(_receivers.begin(), _receivers.end(),
        [&](const Ref* ref) { return ref->ptr == adapter; });
    if (it != _receivers.end()) { (*it)->alive = false; }
}


void SocketAdapter::cleanupReceivers()
{
    if (!_dirty) return;
    for (auto it = _receivers.begin(); it != _receivers.end();) {
        auto ref = *it;
        if (!ref->alive) {
            delete ref;
            it = _receivers.erase(it);
        }
        else ++it;
    }
    _dirty = false;
}


SocketAdapter* SocketAdapter::sender()
{
    return _sender;
}


std::vector<SocketAdapter*> SocketAdapter::receivers()
{
    std::vector<SocketAdapter*> items;
    std::transform(_receivers.begin(), _receivers.end(), std::back_inserter(items),
        [](const Ref* ref) { return ref->ptr; });
    return items;
    // return _receivers;
}


#if 0
//
// SocketSignalAdapter
//


SocketSignalAdapter::SocketSignalAdapter(SocketAdapter* sender, SocketAdapter* receiver)
    : SocketAdapter(sender, receiver)
{
}


SocketSignalAdapter::~SocketSignalAdapter()
{
}


void SocketSignalAdapter::addReceiver(SocketAdapter* adapter, int priority)
{
    Connect.attach(slot(adapter, &net::SocketAdapter::onSocketConnect, -1, priority));
    Recv.attach(slot(adapter, &net::SocketAdapter::onSocketRecv, -1, priority));
    Error.attach(slot(adapter, &net::SocketAdapter::onSocketError, -1, priority));
    Close.attach(slot(adapter, &net::SocketAdapter::onSocketClose, -1, priority));
}


void SocketSignalAdapter::removeReceiver(SocketAdapter* adapter)
{
    Connect.detach(adapter);
    Recv.detach(adapter);
    Error.detach(adapter);
    Close.detach(adapter);

    // Connect -= slot(adapter, &net::SocketAdapter::onSocketConnect);
    // Recv -= slot(adapter, &net::SocketAdapter::onSocketRecv);
    // Error -= slot(adapter, &net::SocketAdapter::onSocketError);
    // Close -= slot(adapter, &net::SocketAdapter::onSocketClose);
}


void SocketSignalAdapter::onSocketConnect(Socket& socket)
{
    if (_receiver)
        _receiver->onSocketConnect(socket);
    else
        Connect.emit(socket);
}


void SocketSignalAdapter::onSocketRecv(Socket& socket, const MutableBuffer& buffer, const Address& peerAddress)
{
    if (_receiver)
        _receiver->onSocketRecv(socket, buffer, peerAddress);
    else
        Recv.emit(socket, buffer, peerAddress);
}


void SocketSignalAdapter::onSocketError(Socket& socket, const scy::Error& error) // const Error& error
{
    if (_receiver)
        _receiver->onSocketError(socket, error);
    else
        Error.emit(socket, error);
}


void SocketSignalAdapter::onSocketClose(Socket& socket)
{
    if (_receiver)
        _receiver->onSocketClose(socket);
    else
        Close.emit(socket);
}
#endif


} // namespace net
} // namespace scy


/// @\}
