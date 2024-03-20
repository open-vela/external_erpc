/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016 NXP
 * All rights reserved.
 *
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef _EMBEDDED_RPC__TCP_TRANSPORT_H_
#define _EMBEDDED_RPC__TCP_TRANSPORT_H_

#include "erpc_sock_transport.hpp"

/*!
 * @addtogroup tcp_transport
 * @{
 * @file
 */

////////////////////////////////////////////////////////////////////////////////
// Classes
////////////////////////////////////////////////////////////////////////////////

namespace erpc {
/*!
 * @brief Client side of TCP/IP transport.
 *
 * @ingroup tcp_transport
 */
class TCPTransport : public SockTransport
{
public:
    /*!
     * @brief Constructor.
     *
     * This function initializes object attributes.
     *
     * @param[in] isServer True when this transport is used for server side application.
     */
    explicit TCPTransport(bool isServer);

    /*!
     * @brief Constructor.
     *
     * This function initializes object attributes.
     *
     * @param[in] host Specify the host name or IP address of the computer.
     * @param[in] port Specify the listening port number.
     * @param[in] isServer True when this transport is used for server side application.
     */
    TCPTransport(const char *host, uint16_t port, bool isServer);

    /*!
     * @brief TCPTransport destructor
     */
    virtual ~TCPTransport(void);

    /*!
     * @brief This function set host and port of this transport layer.
     *
     * @param[in] host Specify the host name or IP address of the computer.
     * @param[in] port Specify the listening port number.
     */
    void configure(const char *host, uint16_t port);

protected:
    const char *m_host; /*!< Specify the host name or IP address of the computer. */
    uint16_t m_port;    /*!< Specify the listening port number. */

    /*!
     * @brief This function connect client to the server.
     *
     * @retval kErpcStatus_Success When client connected successfully.
     * @retval kErpcStatus_Fail When client doesn't connected successfully.
     */
    virtual erpc_status_t connectClient(void);

    /*!
     * @brief Server thread function.
     */
    virtual void serverThread(void);
};

} // namespace erpc

/*! @} */

#endif // _EMBEDDED_RPC__TCP_TRANSPORT_H_
