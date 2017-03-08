///
//
// LibSourcey
// Copyright (c) 2005, Sourcey <http://sourcey.com>
//
// SPDX-License-Identifier: LGPL-2.1+
//
/// @addtogroup net
/// @{


#include "scy/net/socketemitter.h"


using std::endl;


namespace scy {
namespace net {


SocketEmitter::SocketEmitter(const Socket::Ptr& socket)
    : SocketAdapter(socket.get())
    , impl(socket)
{
    TraceS(this) << "Create: " << impl.get() << endl;
    if (impl)
        impl->addReceiver(this);
}


//SocketEmitter::SocketEmitter(const SocketEmitter& that)
//    : SocketAdapter(that.impl.get())
//    , Connect(that.Connect)
//    , Recv(that.Recv)
//    , Error(that.Error)
//    , Close(that.Close)
//    , impl(that.impl)
//{
//    if (impl)
//        impl->addReceiver(this);
//}


SocketEmitter::~SocketEmitter()
{
    TraceS(this) << "Destroy: " << impl.get() << endl;
    if (impl)
        impl->removeReceiver(this);
}


void SocketEmitter::swap(const Socket::Ptr& socket)
{
    assert(!impl && "must not be initialized");
    if (impl)
        impl->removeReceiver(this);
    if (socket)
        socket->addReceiver(this);
    impl = socket;
}


void SocketEmitter::addReceiver(SocketAdapter* adapter)
{
    assert(adapter->priority <= 100);
    Connect.attach(slot(adapter, &net::SocketAdapter::onSocketConnect, -1, adapter->priority));
    Recv.attach(slot(adapter, &net::SocketAdapter::onSocketRecv, -1, adapter->priority));
    Error.attach(slot(adapter, &net::SocketAdapter::onSocketError, -1, adapter->priority));
    Close.attach(slot(adapter, &net::SocketAdapter::onSocketClose, -1, adapter->priority));
}


void SocketEmitter::removeReceiver(SocketAdapter* adapter)
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


void SocketEmitter::onSocketConnect(Socket& socket)
{
    assert(&socket == impl.get());
    //if (_receiver)
    //    _receiver->onSocketConnect(socket);
    //else
    //    Connect.emit(socket);

    SocketAdapter::onSocketConnect(socket);
    Connect.emit(socket);
}


void SocketEmitter::onSocketRecv(Socket& socket, const MutableBuffer& buffer, const Address& peerAddress)
{
    assert(&socket == impl.get());
    //if (_receiver)
    //    _receiver->onSocketRecv(socket, buffer, peerAddress);
    //else
    //    Recv.emit(socket, buffer, peerAddress);

    SocketAdapter::onSocketRecv(socket, buffer, peerAddress);
    Recv.emit(socket, buffer, peerAddress);
}


void SocketEmitter::onSocketError(Socket& socket, const scy::Error& error) // const Error& error
{
    assert(&socket == impl.get());
    //if (_receiver)
    //    _receiver->onSocketError(socket, error);
    //else
    //    Error.emit(socket, error);

    SocketAdapter::onSocketError(socket, error);
    Error.emit(socket, error);
}


void SocketEmitter::onSocketClose(Socket& socket)
{
    assert(&socket == impl.get());
    //if (_receiver)
    //    _receiver->onSocketClose(socket);
    //else
    //    Close.emit(socket);

    SocketAdapter::onSocketClose(socket);
    Close.emit(socket);
}


} // namespace net
} // namespace scy


/// @\}
