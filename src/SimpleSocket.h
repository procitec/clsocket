/*---------------------------------------------------------------------------*/
/*                                                                           */
/* SimpleSocket.h - Simple Socket base class decleration.                    */
/*                                                                           */
/* Author : Mark Carrier (mark@carrierlabs.com)                              */
/*                                                                           */
/*---------------------------------------------------------------------------*/
/* Copyright (c) 2007-2009 CarrierLabs, LLC.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * 4. The name "CarrierLabs" must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission. For written permission, please contact
 *    mark@carrierlabs.com.
 *
 * THIS SOFTWARE IS PROVIDED BY MARK CARRIER ``AS IS'' AND ANY
 * EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL MARK CARRIER OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *----------------------------------------------------------------------------*/
#ifndef __SOCKET_H__
#define __SOCKET_H__

#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>

#if defined(_LINUX) || defined (_DARWIN)
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <netinet/ip.h>
#include <netdb.h>
#endif
#ifdef _LINUX
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <linux/if.h>
#include <sys/sendfile.h>
#endif
#ifdef _DARWIN
#include <net/if.h>
#endif
#if defined(_LINUX) || defined (_DARWIN)
#include <sys/time.h>
#include <sys/uio.h>
#include <unistd.h>
#include <fcntl.h>
#endif

#ifdef _WIN32
#include <io.h>
#include <winsock2.h>
#include <Ws2tcpip.h>

#define IPTOS_LOWDELAY  0x10

#endif
#include "Host.h"
#include "StatTimer.h"

//-----------------------------------------------------------------------------
// General class macro definitions and typedefs
//-----------------------------------------------------------------------------
#ifndef INVALID_SOCKET
#define INVALID_SOCKET    ~(0)
#endif

#define SOCKET_SENDFILE_BLOCKSIZE 8192


#ifndef __cplusplus
  #error C++ compiler required!
#else
  #if ( __cplusplus >= 201103L )
    #define CLSOCKET_OVERRIDE   override
  #elif ( 0 && defined(__GNUC__) && (__GNUC__ == 4) && (__GNUC_MINOR__ > 7 || \
        (__GNUC_MINOR__ == 7 && __GNUC_PATCHLEVEL__ > 1)) )
    #define CLSOCKET_OVERRIDE   override
  #elif ( defined(_MSC_VER) && ( _MSC_VER >= 1400) )
    #define CLSOCKET_OVERRIDE   override
  #else
    #define CLSOCKET_OVERRIDE
  #endif
#endif



/// Provides a platform independent class to for socket development.
/// This class is designed to abstract socket communication development in a
/// platform independent manner.
/// - Socket types
///  -# CActiveSocket Class
///  -# CPassiveSocket Class
class CLSOCKET_API CSimpleSocket {
    friend class CPassiveSocket;
public:
    /// Defines the three possible states for shuting down a socket.
    typedef enum
    {
        Receives = SHUT_RD, ///< Shutdown passive socket.
        Sends = SHUT_WR,    ///< Shutdown active socket.
        Both = SHUT_RDWR    ///< Shutdown both active and passive sockets.
    } CShutdownMode;

    /// Defines the socket types defined by CSimpleSocket class.
    typedef enum
    {
        SocketTypeInvalid,   ///< Invalid socket type.
        SocketTypeTcp,       ///< Defines socket as TCP socket.
        SocketTypeUdp,       ///< Defines socket as UDP socket.
        SocketTypeTcp6,      ///< Defines socket as IPv6 TCP socket.
        SocketTypeUdp6,      ///< Defines socket as IPv6 UDP socket.
        SocketTypeRaw        ///< Provides raw network protocol access.
    } CSocketType;

    /// Defines all error codes handled by the CSimpleSocket class.
    typedef enum
    {
        SocketError = -1,          ///< Generic socket error translates to error below.
        SocketSuccess = 0,         ///< No socket error.
        SocketInvalidSocket,       ///< Invalid socket handle.
        SocketInvalidAddress,      ///< Invalid destination address specified.
        SocketInvalidPort,         ///< Invalid destination port specified.
        SocketConnectionRefused,   ///< No server is listening at remote address.
        SocketTimedout,            ///< Timed out while attempting operation.
        SocketEwouldblock,         ///< Operation would block if socket were blocking. On Accept/Receive/Send/
        SocketNotconnected,        ///< Currently not connected.
        SocketEinprogress,         ///< Socket is non-blocking and the connection cannot be completed immediately.
                                   ///  only on Open/ConnectTo/..
        SocketInterrupted,         ///< Call was interrupted by a signal that was caught before a valid connection arrived.
        SocketConnectionAborted,   ///< The connection has been aborted.
        SocketProtocolError,       ///< Invalid protocol for operation.
        SocketFirewallError,       ///< Firewall rules forbid connection.
        SocketInvalidSocketBuffer, ///< The receive buffer point outside the process's address space.
        SocketConnectionReset,     ///< Connection was forcibly closed by the remote host.
        SocketAddressInUse,        ///< Address already in use.
        SocketInvalidPointer,      ///< Pointer type supplied as argument is invalid.
        SocketEunknown,            ///< Unknown error
        SocketNetworkError         ///< Several network errors
    } CSocketError;

public:
    CSimpleSocket(CSocketType type = SocketTypeTcp);
    CSimpleSocket(CSimpleSocket &socket);

    virtual ~CSimpleSocket();

    /// Initialize instance of CSocket.  This method MUST be called before an
    /// object can be used. Errors : CSocket::SocketProtocolError,
    /// CSocket::SocketInvalidSocket,
    /// @return true if properly initialized.
    virtual bool Initialize(void);

    /// Establishes a connection to the (server-)address specified by pAddr
    ///     and (server-)port specified by nPort.
    /// Connection-based protocol sockets (CSocket::SocketTypeTcp) may
    /// successfully call Open() only once, however; connectionless protocol
    /// sockets (CSocket::SocketTypeUdp) may use Open() multiple times to
    /// change their association.
    ///  @param pAddr specifies the destination address to connect.
    ///  @param nPort specifies the destination port.
    ///  @return true if successful connection made, otherwise false.
    virtual bool Open(const char *pAddr, uint16 nPort);

    inline bool ConnectTo( const char * pAddr, uint16 nPort ) {
        return Open(pAddr, nPort);
    };

    /// Close socket
    /// @return true if successfully closed otherwise returns false.
    virtual bool Close(void);

    /// Shutdown shut down socket send and receive operations
    ///    CShutdownMode::Receives - Disables further receive operations.
    ///    CShutdownMode::Sends    - Disables further send operations.
    ///    CShutdownMode::Both     - Disables further send and receive operations.
    /// see http://stackoverflow.com/questions/4160347/close-vs-shutdown-socket
    /// @param nShutdown specifies the type of shutdown.
    /// @return true if successfully shutdown otherwise returns false.
    virtual bool Shutdown(CShutdownMode nShutdown);

    inline bool CloseForReads() {
        return Shutdown( CSimpleSocket::Receives );
    }

    inline bool CloseForWrites() {
        return Shutdown( CSimpleSocket::Sends );
    }

    /// Examine the socket descriptor sets currently owned by the instance of
    /// the socket class (the readfds, writefds, and errorfds parameters) to
    /// see whether some of their descriptors are ready for reading, are ready
    /// for writing, or have an exceptional condition pending, respectively.
    /// Block until an event happens on the specified file descriptors.
    /// @return true if socket has data ready, or false if not ready or timed out.
    virtual bool Select(void);

    /// Examine the socket descriptor sets currently owned by the instance of
    /// the socket class (the readfds, writefds, and errorfds parameters) to
    /// see whether some of their descriptors are ready for reading, are ready
    /// for writing, or have an exceptional condition pending, respectively.
    /// @param nTimeoutSec timeout in seconds for select.
    /// @param nTimeoutUSec timeout in micro seconds for select.
    /// @return true if socket has data ready, or false if not ready or timed out.
    virtual bool Select(int32 nTimeoutSec, int32 nTimeoutUSec,
                        bool bAwakeWhenReadable = true, bool bAwakeWhenWritable = true );

    inline bool WaitUntilReadable( int32 nTimeoutMillis ) {
        return Select( nTimeoutMillis / 1000 , 1000 * (nTimeoutMillis % 1000), true, false );
    }

    inline bool WaitUntilWritable( int32 nTimeoutMillis ) {
        return Select( nTimeoutMillis / 1000 , 1000 * (nTimeoutMillis % 1000), false, true );
    }

    /// Does the current instance of the socket object contain a valid socket
    /// descriptor.
    ///  @return true if the socket object contains a valid socket descriptor.
    virtual bool IsSocketValid(void) const;


    inline bool IsSocketInvalid(void) const {
        return !IsSocketValid();
    };


    /// Is the current instance of the socket already closed from peer?
    /// Information is updated on Receive() !
    ///  @return true if the socket was closed
    virtual bool IsSocketPeerClosed(void) const;


    inline bool IsSocketPeerOpen(void) const {
        return !IsSocketPeerClosed();
    };


    /// Clear "last" system error
    /// - so that TranslateSocketError() does not try to translate someone else's error!
    void ClearSystemError(void);

    /// Provides a standard error code for cross platform development by
    /// mapping the operating system error to an error defined by the CSocket
    /// class.
    void TranslateSocketError(void);

    /// Returns a human-readable description of the given error code
    /// or the last error code of a socket
    static const char *DescribeError(CSocketError err);
    inline const char *DescribeError() {
        return DescribeError(m_socketErrno);
    };

    static inline const char * GetSocketErrorText(CSocketError err) {
        return DescribeError(err);
    }

    inline const char * GetSocketErrorText() {
        return DescribeError(m_socketErrno);
    }

    /// Attempts to receive a block of data on an established connection.
    /// @param nMaxBytes maximum number of bytes to receive.
    /// @param pBuffer, memory where to receive the data,
    ///        NULL receives to internal buffer returned with GetData()
    ///        Non-NULL receives directly there, but GetData() will return WRONG ptr!
    /// @return number of bytes actually received.
    /// @return of zero means the connection has been shutdown on the other side.
    /// @return of -1 means that an error has occurred.
    virtual int32 Receive(int32 nMaxBytes = 1, uint8 * pBuffer = 0);

    inline int32 Receive(int32 nMaxBytes, char * pBuffer) {
        return Receive(nMaxBytes, (uint8*)pBuffer);
    };


    /// Attempts to return the number of Bytes waiting at next Receive()
    /// @return number of bytes ready for Receive()
    /// @return of -1 means that an error has occurred.
    virtual int32 GetNumReceivableBytes();


    /// Attempts to send a block of data on an established connection.
    /// @param pBuf block of data to be sent.
    /// @param bytesToSend size of data block to be sent.
    /// @return number of bytes actually sent.
    /// @return of zero means the connection has been shutdown on the other side.
    /// @return of -1 means that an error has occurred.
    virtual int32 Send(const uint8 *pBuf, size_t bytesToSend);

    inline int32 Send(const char *pBuf, size_t bytesToSend) {
        return Send( (const uint8 *)pBuf, bytesToSend );
    }

    inline int32 Transmit(const uint8 *pBuf, size_t bytesToSend) {
        return Send( pBuf, bytesToSend );
    }

    inline int32 Transmit(const char *pBuf, size_t bytesToSend) {
        return Send( (const uint8 *)pBuf, bytesToSend );
    }

    /// Attempts to send at most nNumItem blocks described by sendVector
    /// to the socket descriptor associated with the socket object.
    /// @param sendVector pointer to an array of iovec structures
    /// @param nNumItems number of items in the vector to process
    /// <br>\b NOTE: Buffers are processed in the order specified.
    /// @return number of bytes actually sent, return of zero means the
    /// connection has been shutdown on the other side, and a return of -1
    /// means that an error has occurred.
    virtual int32 Send(const struct iovec *sendVector, int32 nNumItems);

    /// Copies data between one file descriptor and another.
    /// On some systems this copying is done within the kernel, and thus is
    /// more efficient than the combination of CSimpleSocket::Send and
    /// CSimpleSocket::Receive, which would require transferring data to and
    /// from user space.
    /// <br>\b Note: This is available on all implementations, but the kernel
    /// implementation is only available on Unix type systems.
    /// @param nOutFd descriptor opened for writing.
    /// @param nInFd descriptor opened for reading.
    /// @param pOffset from which to start reading data from input file.
    /// @param nCount number of bytes to copy between file descriptors.
    /// @return number of bytes written to the out socket descriptor.
    virtual int32 SendFile(int32 nOutFd, int32 nInFd, off_t *pOffset, int32 nCount);

    /// Returns blocking/non-blocking state of socket.
    /// @return true if the socket is non-blocking, else return false.
    bool IsNonblocking(void) const {
        return (m_bIsBlocking == false);
    };

    inline bool IsBlocking(void) const {
      return !IsNonblocking();
    }


    /// Set the socket to blocking.
    /// @return true if successful set to blocking, else return false;
    bool SetBlocking(void);

    /// Set the socket as non-blocking.
    /// @return true if successful set to non-blocking, else return false;
    bool SetNonblocking(void);

    /// Get a pointer to internal receive buffer.  The user MUST not free this
    /// pointer when finished.  This memory is managed internally by the CSocket
    /// class.
    /// @return pointer to data if valid, else returns NULL.
    uint8 *GetData(void)  {
        return m_pBuffer;
    };

    /// Returns the number of bytes received on the last call to
    /// CSocket::Receive().
    /// @return number of bytes received.
    int32 GetBytesReceived(void) {
        return m_nBytesReceived;
    };

    /// Returns the number of bytes sent on the last call to
    /// CSocket::Send().
    /// @return number of bytes sent.
    int32 GetBytesSent(void) {
        return m_nBytesSent;
    };

    /// Controls the actions taken when CSimpleSocket::Close is executed on a
    /// socket object that has unsent data.  The default value for this option
    /// is \b off.
    /// - Following are the three possible scenarios.
    ///  -# \b bEnable is false, CSimpleSocket::Close returns immediately, but
    ///  any unset data is transmitted (after CSimpleSocket::Close returns)
    ///  -# \b bEnable is true and \b nTime is zero, CSimpleSocket::Close return
    /// immediately and any unsent data is discarded.
    ///  -# \b bEnable is true and \b nTime is nonzero, CSimpleSocket::Close does
    ///  not return until all unsent data is transmitted (or the connection is
    ///  Closed by the remote system).
    /// <br><p>
    /// @param bEnable true to enable option false to disable option.
    /// @param nTimeInSeconds time in seconds to linger.
    /// @return true if option successfully set
    bool SetOptionLinger(bool bEnable, uint16 nTimeInSeconds);

    /// Tells the kernel that even if this port is busy (in the TIME_WAIT state),
    /// go ahead and reuse it anyway.  If it is busy, but with another state,
    /// you will still get an address already in use error.
    /// @return true if option successfully set
    bool SetOptionReuseAddr();

    /// Gets the timeout value that specifies the maximum number of seconds a
    /// call to CSimpleSocket::Open waits until it completes.
    /// @return the length of time in seconds
    int32 GetConnectTimeoutSec(void) {
        return  m_stConnectTimeout.tv_sec;
    };

    /// Gets the timeout value that specifies the maximum number of microseconds
    /// a call to CSimpleSocket::Open waits until it completes.
    /// @return the length of time in microseconds
    int32 GetConnectTimeoutUSec(void) {
        return  m_stConnectTimeout.tv_usec;
    };

    /// Sets the timeout value that specifies the maximum amount of time a call
    /// to CSimpleSocket::Receive waits until it completes. Use the method
    /// CSimpleSocket::SetReceiveTimeout to specify the number of seconds to wait.
    /// If a call to CSimpleSocket::Receive has blocked for the specified length of
    /// time without receiving additional data, it returns with a partial count
    /// or CSimpleSocket::GetSocketError set to CSimpleSocket::SocketEwouldblock if no data
    /// were received.
    /// @param nConnectTimeoutSec of timeout in seconds.
    /// @param nConnectTimeoutUsec of timeout in microseconds.
    /// @return true if socket connection timeout was successfully set.
    void SetConnectTimeout(int32 nConnectTimeoutSec, int32 nConnectTimeoutUsec = 0);

    inline void SetConnectTimeoutMillis(int32 nConnectTimeoutMillis) {
        SetConnectTimeout( nConnectTimeoutMillis / 1000, 1000 * ( nConnectTimeoutMillis % 1000 ) );
    };

    /// Gets the timeout value that specifies the maximum number of seconds a
    /// a call to CSimpleSocket::Receive waits until it completes.
    /// @return the length of time in seconds
    int32 GetReceiveTimeoutSec(void) {
        return  m_stRecvTimeout.tv_sec;
    };

    /// Gets the timeout value that specifies the maximum number of microseconds
    /// a call to CSimpleSocket::Receive waits until it completes.
    /// @return the length of time in microseconds
    int32 GetReceiveTimeoutUSec(void) {
        return  m_stRecvTimeout.tv_usec;
    };

    /// Sets the timeout value that specifies the maximum amount of time a call
    /// to CSimpleSocket::Receive waits until it completes. Use the method
    /// CSimpleSocket::SetReceiveTimeout to specify the number of seconds to wait.
    /// If a call to CSimpleSocket::Receive has blocked for the specified length of
    /// time without receiving additional data, it returns with a partial count
    /// or CSimpleSocket::GetSocketError set to CSimpleSocket::SocketEwouldblock if no data
    /// were received.
    ///  @param nRecvTimeoutSec of timeout in seconds.
    ///  @param nRecvTimeoutUsec of timeout in microseconds.
    ///  @return true if socket timeout was successfully set.
    bool SetReceiveTimeout(int32 nRecvTimeoutSec, int32 nRecvTimeoutUsec = 0);

    inline bool SetReceiveTimeoutMillis(int32 nRecvTimeoutMillis) {
        return SetReceiveTimeout( nRecvTimeoutMillis / 1000 , 1000 * (nRecvTimeoutMillis % 1000) );
    };

    /// Enable/disable multicast for a socket.  This options is only valid for
    /// socket descriptors of type CSimpleSocket::SocketTypeUdp.
    /// @return true if multicast was enabled or false if socket type is not
    /// CSimpleSocket::SocketTypeUdp and the error will be set to
    /// CSimpleSocket::SocketProtocolError
    bool SetMulticast(bool bEnable, uint8 multicastTTL = 1);

    /// Return true if socket is multicast or false is socket is unicast
    /// @return true if multicast is enabled
    bool GetMulticast() {
        return m_bIsMulticast;
    };

    /// Bind socket to a specific interface when using multicast.
    /// @return true if successfully bound to interface
    bool BindInterface(const char *pInterface);


    /// Bind socket to a specific interface in general for tcp/udp clients
    ///  @param pInterface - interface on which to bind.
    ///  @param nPort - port on which multicast
    ///  @return true if able to bind to interface.
    bool Bind(const char *pInterface, uint16 nPort);

    /// Gets the timeout value that specifies the maximum number of seconds a
    /// a call to CSimpleSocket::Send waits until it completes.
    /// @return the length of time in seconds
    int32 GetSendTimeoutSec(void) {
        return  m_stSendTimeout.tv_sec;
    };

    /// Gets the timeout value that specifies the maximum number of microseconds
    /// a call to CSimpleSocket::Send waits until it completes.
    /// @return the length of time in microseconds
    int32 GetSendTimeoutUSec(void) {
        return  m_stSendTimeout.tv_usec;
    };

    /// Gets the timeout value that specifies the maximum amount of time a call
    /// to CSimpleSocket::Send waits until it completes.
    /// @return the length of time in seconds
    bool SetSendTimeout(int32 nSendTimeoutSec, int32 nSendTimeoutUsec = 0);

    inline bool SetSendTimeoutMillis(int32 nSendTimeoutMillis) {
        return SetSendTimeout( nSendTimeoutMillis / 1000 , 1000 * (nSendTimeoutMillis % 1000) );
    };

    /// Returns the last error that occurred for the instance of the CSimpleSocket
    /// instance.  This method should be called immediately to retrieve the
    /// error code for the failing mehtod call.
    ///  @return last error that occurred.
    CSocketError GetSocketError(void) {
        return m_socketErrno;
    };

    /// Get the total time the of the last operation in milliseconds.
    ///  @return number of milliseconds of last operation.
    uint32 GetTotalTimeMs() {
        return m_timer.GetMilliSeconds();
    };

    /// Get the total time the of the last operation in microseconds.
    ///  @return number of microseconds or last operation.
    uint32 GetTotalTimeUsec() {
        return m_timer.GetMicroSeconds();
    };

    /// Return Differentiated Services Code Point (DSCP) value currently set on the socket object.
    /// @return DSCP for current socket object.
    /// <br><br> \b NOTE: Windows special notes http://support.microsoft.com/kb/248611.
    int GetSocketDscp(void);

    /// Set Differentiated Services Code Point (DSCP) for socket object.
    ///  @param nDscp value of TOS setting which will be converted to DSCP
    ///  @return true if DSCP value was properly set
    /// <br><br> \b NOTE: Windows special notes http://support.microsoft.com/kb/248611.
    bool SetSocketDscp(int nDscp);

    /// Return socket descriptor
    ///  @return socket descriptor which is a signed 32 bit integer.
    SOCKET GetSocketDescriptor() {
        return m_socket;
    };

    /// Return socket descriptor
    ///  @return socket descriptor which is a signed 32 bit integer.
    CSocketType GetSocketType() {
        return m_nSocketType;
    };

    /// Returns clients Internet host address as a string in standard numbers-and-dots notation.
    ///   on TCP Server the Client and Peer Address/Port get valid with successful Accept()
    ///   on UDP Server the Client and Peer Address/Port get valid with successful Receive()
    /// ATTENTION: return is same static buffer as in inet_ntoa(), GetClientAddr(), GetServerAddr(), GetLocalAddr(), GetPeerAddr()
    ///  @return NULL if invalid
    const char * GetClientAddr() const;

    /// Returns the port number on which the client is connected.
    ///   on TCP Server the Client and Peer Address/Port get valid with successful Accept()
    ///   on UDP Server the Client and Peer Address/Port get valid with successful Receive()
    ///  @return client port number.
    uint16 GetClientPort() const;

    /// Returns server Internet host address as a string in standard numbers-and-dots notation.
    /// ATTENTION: return is same static buffer as in inet_ntoa(), GetClientAddr(), GetServerAddr(), GetLocalAddr(), GetPeerAddr()
    ///  @return NULL if invalid
    const char * GetServerAddr() const;

    /// Returns the port number on which the server is connected.
    ///  @return server port number.
    uint16 GetServerPort() const;

    /// Returns if this is the Server side of the connection
    bool IsServerSide() const;

    /// Returns local Internet host address as a string in standard numbers-and-dots notation.
    /// ATTENTION: return is same static buffer as in inet_ntoa(), GetClientAddr(), GetServerAddr(), GetLocalAddr(), GetPeerAddr()
    ///  @return NULL if invalid
    const char * GetLocalAddr() const;

    /// Returns the port number on which the local socket is connected.
    ///  @return client port number.
    uint16 GetLocalPort() const;


    /// Returns Peer's Internet host address as a string in standard numbers-and-dots notation.
    ///   on TCP Server the Client and Peer Address/Port get valid with successful Accept()
    ///   on UDP Server the Client and Peer Address/Port get valid with successful Receive()
    /// ATTENTION: return is same static buffer as in inet_ntoa(), GetClientAddr(), GetServerAddr(), GetLocalAddr(), GetPeerAddr()
    ///  @return NULL if invalid
    const char * GetPeerAddr() const;

    /// Returns the port number on which the peer is connected.
    ///   on TCP Server the Client and Peer Address/Port get valid with successful Accept()
    ///   on UDP Server the Client and Peer Address/Port get valid with successful Receive()
    ///  @return client port number.
    uint16 GetPeerPort() const;


    /// Get the TCP receive buffer window size for the current socket object.
    /// <br><br>\b NOTE: Linux will set the receive buffer to twice the value passed.
    ///  @return zero on failure else the number of bytes of the TCP receive buffer window size if successful.
    uint32 GetReceiveWindowSize();

    /// Get the TCP send buffer window size for the current socket object.
    /// <br><br>\b NOTE: Linux will set the send buffer to twice the value passed.
    ///  @return zero on failure else the number of bytes of the TCP receive buffer window size if successful.
    uint32 GetSendWindowSize();

    /// Set the TCP receive buffer window size for the current socket object.
    /// <br><br>\b NOTE: Linux will set the receive buffer to twice the value passed.
    ///  @return zero on failure else the number of bytes of the TCP send buffer window size if successful.
    uint32 SetReceiveWindowSize(uint32 nWindowSize);

    /// Set the TCP send buffer window size for the current socket object.
    /// <br><br>\b NOTE: Linux will set the send buffer to twice the value passed.
    ///  @return zero on failure else the number of bytes of the TCP send buffer window size if successful.
    uint32 SetSendWindowSize(uint32 nWindowSize);

    /// Disable the Nagle algorithm (Set TCP_NODELAY to true)
    /// @return false if failed to set socket option otherwise return true;
    bool DisableNagleAlgoritm();

    /// Enable the Nagle algorithm (Set TCP_NODELAY to false)
    /// @return false if failed to set socket option otherwise return true;
    bool EnableNagleAlgoritm();

    /// retrieve IPv4 address via getaddrinfo() as uint32 in HostByteOrder
    ///   as static member
    /// @return 0 if failed
    static uint32 GetIPv4AddrInfoStatic( const char *pAddr, CSocketType nSocketType = SocketTypeTcp );

    /// retrieve IPv4 address via getaddrinfo() as uint32 in HostByteOrder
    /// @return 0 if failed
    uint32 GetIPv4AddrInfo( const char *pAddr );

protected:
    /// Set internal socket error to that specified error
    ///  @param error type of error
    void SetSocketError(CSimpleSocket::CSocketError error) {
        m_socketErrno = error;
    };

    /// Set object socket handle to that specified as parameter
    ///  @param socket value of socket descriptor
    void SetSocketHandle(SOCKET socket, bool bIsServerSide = true );

private:
    /// Generic function used to get the send/receive window size
    ///  @return zero on failure else the number of bytes of the TCP window size if successful.
    CLSOCKET_NO_EXPORT uint32 GetWindowSize(uint32 nOptionName);

    /// Generic function used to set the send/receive window size
    ///  @return zero on failure else the number of bytes of the TCP window size if successful.
    CLSOCKET_NO_EXPORT uint32 SetWindowSize(uint32 nOptionName, uint32 nWindowSize);


    /// Attempts to send at most nNumItem blocks described by sendVector
    /// to the socket descriptor associated with the socket object.
    /// @param sendVector pointer to an array of iovec structures
    /// @param nNumItems number of items in the vector to process
    /// <br>\b Note: This implementation is for systems that don't natively
    /// support this functionality.
    /// @return number of bytes actually sent, return of zero means the
    /// connection has been shutdown on the other side, and a return of -1
    /// means that an error has occurred.
    CLSOCKET_NO_EXPORT int32 Writev(const struct iovec *pVector, size_t nCount);

    /// Flush the socket descriptor owned by the object.
    /// @return true data was successfully sent, else return false;
    CLSOCKET_NO_EXPORT bool Flush();

    CLSOCKET_NO_EXPORT CSimpleSocket *operator=(CSimpleSocket &socket);

    static bool GetAddrInfoStatic(const char *pAddr, uint16 nPort, struct in_addr * pOutIpAddress, CSocketType nSocketType = SocketTypeTcp );

    CLSOCKET_NO_EXPORT bool GetAddrInfo(const char *pAddr, uint16 nPort, struct in_addr * pOutIpAddress );

    /// Utility function used to create a TCP connection, called from Open().
    ///  @return true if successful connection made, otherwise false.
    CLSOCKET_NO_EXPORT bool ConnectTCP(const char *pAddr, uint16 nPort);

    /// Utility function used to create a UDP connection, called from Open().
    ///  @return true if successful connection made, otherwise false.
    CLSOCKET_NO_EXPORT bool ConnectUDP(const char *pAddr, uint16 nPort);

    /// Utility function used to create a RAW connection, called from Open().
    ///  @return true if successful connection made, otherwise false.
    CLSOCKET_NO_EXPORT bool ConnectRAW(const char *pAddr, uint16 nPort);

protected:
    SOCKET               m_socket;            /// socket handle
    CSocketError         m_socketErrno;       /// number of last error
    uint8               *m_pBuffer;           /// internal send/receive buffer
    int32                m_nBufferSize;       /// size of internal send/receive buffer
    int32                m_nSocketDomain;     /// socket type PF_INET, PF_INET6
    CSocketType          m_nSocketType;       /// socket type - UDP, TCP or RAW
    int32                m_nBytesReceived;    /// number of bytes received
    int32                m_nBytesSent;        /// number of bytes sent
    uint32               m_nFlags;            /// socket flags
    bool                 m_bIsBlocking;       /// is socket blocking
    bool                 m_bIsMulticast;      /// is the UDP socket multicast;
    bool                 m_bIsServerSide;     /// is this the server? => m_stServerSockaddr == localAddr()
                                              ///    and m_stClientSockaddr == peerAddr()
                                              /// else: m_stClientSockaddr == localAddr()
    bool                 m_bPeerHasClosed;    /// is socket closed from peer?
    struct timeval       m_stConnectTimeout;  /// connection timeout
    struct timeval       m_stRecvTimeout;     /// receive timeout
    struct timeval       m_stSendTimeout;     /// send timeout
    struct sockaddr_in   m_stServerSockaddr;  /// server address
    struct sockaddr_in   m_stClientSockaddr;  /// client address
    struct sockaddr_in   m_stMulticastGroup;  /// multicast group to bind to
    struct linger        m_stLinger;          /// linger flag
    CStatTimer           m_timer;             /// internal statistics.
#ifdef WIN32
    WSADATA              m_hWSAData;          /// Windows
#endif
    fd_set               m_writeFds;          /// write file descriptor set
    fd_set               m_readFds;           /// read file descriptor set
    fd_set               m_errorFds;          /// error file descriptor set
};


#endif /*  __SOCKET_H__  */

