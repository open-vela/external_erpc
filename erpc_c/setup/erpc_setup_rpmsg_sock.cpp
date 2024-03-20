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

#include "erpc_manually_constructed.hpp"
#include "erpc_rpmsg_sock_transport.hpp"
#include "erpc_transport_setup.h"

using namespace erpc;

////////////////////////////////////////////////////////////////////////////////
// Variables
////////////////////////////////////////////////////////////////////////////////

ERPC_MANUALLY_CONSTRUCTED_STATIC(RpmsgSockTransport, s_rpmsgSockTransport);

////////////////////////////////////////////////////////////////////////////////
// Code
////////////////////////////////////////////////////////////////////////////////

erpc_transport_t erpc_transport_rpmsg_sock_init(const char *rp_name,
                                                const char *rp_cpu, bool isServer)
{
    erpc_transport_t transport;
    RpmsgSockTransport *rpmsgSockTransport;

#if ERPC_ALLOCATION_POLICY == ERPC_ALLOCATION_POLICY_STATIC
    if (s_rpmsgSockTransport.isUsed())
    {
        rpmsgSockTransport = NULL;
    }
    else
    {
        s_rpmsgSockTransport.construct(host, port, isServer);
        rpmsgSockTransport = s_rpmsgSockTransport.get();
    }
#elif ERPC_ALLOCATION_POLICY == ERPC_ALLOCATION_POLICY_DYNAMIC
    rpmsgSockTransport = new RpmsgSockTransport(rp_name, rp_cpu, isServer);
#else
#error "Unknown eRPC allocation policy!"
#endif

    transport = reinterpret_cast<erpc_transport_t>(rpmsgSockTransport);

    if (rpmsgSockTransport != NULL)
    {
        if (rpmsgSockTransport->open() != kErpcStatus_Success)
        {
            erpc_transport_rpmsg_sock_deinit(transport);
            transport = NULL;
        }
    }

    return transport;
}

void erpc_transport_rpmsg_sock_close(erpc_transport_t transport)
{
    erpc_assert(transport != NULL);

    RpmsgSockTransport *rpmsgSockTransport = reinterpret_cast<RpmsgSockTransport *>(transport);

    rpmsgSockTransport->close(true);
}

void erpc_transport_rpmsg_sock_deinit(erpc_transport_t transport)
{
#if ERPC_ALLOCATION_POLICY == ERPC_ALLOCATION_POLICY_STATIC
    (void)transport;
    s_rpmsgSockTransport.destroy();
#elif ERPC_ALLOCATION_POLICY == ERPC_ALLOCATION_POLICY_DYNAMIC
    erpc_assert(transport != NULL);

    RpmsgSockTransport *rpmsgSockTransport = reinterpret_cast<RpmsgSockTransport *>(transport);

    delete rpmsgSockTransport;
#endif
}
