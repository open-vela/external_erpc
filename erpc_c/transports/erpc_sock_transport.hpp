/*
 * Copyright (C) 2024 Xiaomi Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _EMBEDDED_RPC__SOCK_TRANSPORT_H_
#define _EMBEDDED_RPC__SOCK_TRANSPORT_H_

#if defined(__MINGW32__)
#include <winsock2.h>
#endif

#include "erpc_framed_transport.hpp"
#include "erpc_threading.h"

/*!
 * @addtogroup sock_transport
 * @{
 * @file
 */

////////////////////////////////////////////////////////////////////////////////
// Classes
////////////////////////////////////////////////////////////////////////////////

namespace erpc {
/*!
 * @brief Client side of Socket transport.
 *
 * @ingroup sock_transport
 */
class SockTransport : public FramedTransport
{
public:
    /*!
     * @brief Constructor.
     *
     * This function initializes object attributes.
     *
     * @param[in] isServer True when this transport is used for server side application.
     */
    SockTransport(bool isServer);

    /*!
     * @brief SockTransport destructor
     */
    virtual ~SockTransport(void);

    /*!
     * @brief This function will create host on server side, or connect client to the server.
     *
     * @retval #kErpcStatus_Success When creating host was successful or client connected successfully.
     * @retval #kErpcStatus_UnknownName Host name resolution failed.
     * @retval #kErpcStatus_ConnectionFailure Connecting to the specified host failed.
     */
    virtual erpc_status_t open(void);

    /*!
     * @brief This function disconnects client or stop server host.
     *
     * @param[in] stopServer Specify is server shall be closed as well (stop listen())
     * @retval #kErpcStatus_Success Always return this.
     */
    virtual erpc_status_t close(bool stopServer = true);

protected:
    bool m_isServer;    /*!< If true then server is using transport, else client. */
#if defined(__MINGW32__)
    SOCKET m_socket; /*!< Socket number. */
#else
    int m_socket; /*!< Socket number. */
#endif
    bool m_runServer;      /*!< Thread is executed while this is true. */
    Thread m_serverThread; /*!< Pointer to server thread. */

    using FramedTransport::underlyingReceive;
    using FramedTransport::underlyingSend;

    /*!
     * @brief This function connect client to the server.
     *
     * @retval kErpcStatus_Success When client connected successfully.
     * @retval kErpcStatus_Fail When client doesn't connected successfully.
     */
    virtual erpc_status_t connectClient(void) = 0;

    /*!
     * @brief This function read data.
     *
     * @param[inout] data Preallocated buffer for receiving data.
     * @param[in] size Size of data to read.
     *
     * @retval #kErpcStatus_Success When data was read successfully.
     * @retval #kErpcStatus_ReceiveFailed When reading data ends with error.
     * @retval #kErpcStatus_ConnectionClosed Peer closed the connection.
     */
    virtual erpc_status_t underlyingReceive(uint8_t *data, uint32_t size);

    /*!
     * @brief This function writes data.
     *
     * @param[in] data Buffer to send.
     * @param[in] size Size of data to send.
     *
     * @retval #kErpcStatus_Success When data was written successfully.
     * @retval #kErpcStatus_SendFailed When writing data ends with error.
     * @retval #kErpcStatus_ConnectionClosed Peer closed the connection.
     */
    virtual erpc_status_t underlyingSend(const uint8_t *data, uint32_t size);

    /*!
     * @brief Server thread function.
     */
    virtual void serverThread(void) = 0;

    /*!
     * @brief Thread entry point.
     *
     * Control is passed to the serverThread() method of the TCPTransport instance pointed to
     * by the @c arg parameter.
     *
     * @param arg Thread argument. The pointer to the TCPTransport instance is passed through
     *  this argument.
     */
    static void serverThreadStub(void *arg);
};

} // namespace erpc

/*! @} */

#endif // _EMBEDDED_RPC__SOCK_TRANSPORT_H_
