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

#ifndef _EMBEDDED_RPC__RPMSG_SOCK_TRANSPORT_H_
#define _EMBEDDED_RPC__RPMSG_SOCK_TRANSPORT_H_

#include "erpc_sock_transport.hpp"

/*!
 * @addtogroup rpmsg_sock_transport
 * @{
 * @file
 */

////////////////////////////////////////////////////////////////////////////////
// Classes
////////////////////////////////////////////////////////////////////////////////

namespace erpc {
/*!
 * @brief Client side of RpmsgSock transport.
 *
 * @ingroup rpmsg_sock_transport
 */
class RpmsgSockTransport : public SockTransport
{
public:
    /*!
     * @brief Constructor.
     *
     * This function initializes object attributes.
     *
     * @param[in] isServer True when this transport is used for server side application.
     */
    explicit RpmsgSockTransport(bool isServer);

    /*!
     * @brief Constructor.
     *
     * This function initializes object attributes.
     *
     * @param[in] rp_name Specify the remote socket name.
     * @param[in] rp_cpu Specify the remote cpu name.
     * @param[in] isServer True when this transport is used for server side application.
     */
    RpmsgSockTransport(const char *rp_name, const char *rp_cpu, bool isServer);

    /*!
     * @brief RpmsgSockTransport destructor
     */
    virtual ~RpmsgSockTransport(void);

    /*!
     * @brief This function set remote socket name and remote cpu name of this transport layer.
     *
     * @param[in] rp_name Specify the remote socket name.
     * @param[in] rp_cpu Specify the remote cpu name.
     */
    void configure(const char *rp_name, const char *rp_cpu);

protected:
    const char *m_rp_name; /*!< Specify the remote socket name. */
    const char *m_rp_cpu;  /*!< Specify the remote cpu name. */

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

#endif // _EMBEDDED_RPC__RPMSG_SOCK_TRANSPORT_H_
