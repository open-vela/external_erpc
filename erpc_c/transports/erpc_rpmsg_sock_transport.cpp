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

#include "erpc_rpmsg_sock_transport.hpp"

#include <cstdio>
#include <cstring>

extern "C" {
// Set this to 1 to enable debug logging.
// TODO fix issue with the transport not working on Linux if debug logging is disabled.
#define RPMSG_SOCK_TRANSPORT_DEBUG_LOG (1)

#if RPMSG_SOCK_TRANSPORT_DEBUG_LOG
#if ERPC_HAS_POSIX
#include <err.h>
#endif
#endif
#include <errno.h>
#include <netpacket/rpmsg.h>
#include <sys/socket.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
}

using namespace erpc;

#if RPMSG_SOCK_TRANSPORT_DEBUG_LOG
#define RPMSG_SOCK_DEBUG_PRINT(_fmt_, ...) printf(_fmt_, ##__VA_ARGS__)
#define RPMSG_SOCK_DEBUG_ERR(_msg_) err(errno, _msg_)
#else
#define RPMSG_SOCK_DEBUG_PRINT(_fmt_, ...)
#define RPMSG_SOCK_DEBUG_ERR(_msg_)
#endif

////////////////////////////////////////////////////////////////////////////////
// Code
////////////////////////////////////////////////////////////////////////////////

RpmsgSockTransport::RpmsgSockTransport(bool isServer) :
SockTransport(isServer), m_rp_name(NULL), m_rp_cpu(NULL)
{
}

RpmsgSockTransport::RpmsgSockTransport(const char *rp_name, const char *rp_cpu, bool isServer) :
SockTransport(isServer), m_rp_name(rp_name), m_rp_cpu(rp_cpu)
{
}

RpmsgSockTransport::~RpmsgSockTransport(void) {}

void RpmsgSockTransport::configure(const char *rp_name, const char *rp_cpu)
{
    m_rp_name = rp_name;
    m_rp_cpu = rp_cpu;
}

erpc_status_t RpmsgSockTransport::connectClient(void)
{
    struct sockaddr_rpmsg res;
    int sock = -1;

    if (m_socket != -1)
    {
        RPMSG_SOCK_DEBUG_PRINT("%s", "socket already connected\n");
    }
    else
    {
        sock = socket(PF_RPMSG, SOCK_STREAM, 0);
        if (sock < 0)
        {
            RPMSG_SOCK_DEBUG_ERR("rpmsg sock create failed");
            return kErpcStatus_ConnectionFailure;
        }

        res.rp_family = AF_RPMSG;
        strlcpy(res.rp_name, m_rp_name, RPMSG_SOCKET_NAME_SIZE);
        strlcpy(res.rp_cpu, m_rp_cpu, RPMSG_SOCKET_NAME_SIZE);
        if (connect(sock, (struct sockaddr *)&res, sizeof(res)) < 0)
        {
            ::close(sock);
            sock = -1;
            RPMSG_SOCK_DEBUG_ERR("rpmsg sock create failed");
            return kErpcStatus_ConnectionFailure;
        }

        m_socket = sock;
    }

    return kErpcStatus_Success;
}

void RpmsgSockTransport::serverThread(void)
{
    int serverSocket;
    int result;
    struct sockaddr_rpmsg incomingAddress;
    socklen_t incomingAddressLength;
    int incomingSocket;
    bool status = false;
    struct sockaddr_rpmsg serverAddress;

    RPMSG_SOCK_DEBUG_PRINT("%s", "in server thread\n");

    // Create socket.
    serverSocket = socket(PF_RPMSG, SOCK_STREAM, 0);
    if (serverSocket < 0)
    {
        RPMSG_SOCK_DEBUG_ERR("failed to create server socket");
    }
    else
    {
        // Fill in address struct.
        serverAddress.rp_family = AF_RPMSG;
        strlcpy(serverAddress.rp_name, m_rp_name, RPMSG_SOCKET_NAME_SIZE);
        strlcpy(serverAddress.rp_cpu, m_rp_cpu, RPMSG_SOCKET_NAME_SIZE);

        // Bind socket to address.
        result = bind(serverSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress));
        if (result < 0)
        {
            RPMSG_SOCK_DEBUG_ERR("bind failed");
            status = true;
        }

        if (!status)
        {
            // Listen for connections.
            result = listen(serverSocket, 1);
            if (result < 0)
            {
                RPMSG_SOCK_DEBUG_ERR("listen failed");
                status = true;
            }
        }

        if (!status)
        {
            RPMSG_SOCK_DEBUG_PRINT("%s", "Listening for connections\n");

            while (m_runServer)
            {
                incomingAddressLength = sizeof(struct sockaddr);
                // we should use select() otherwise we can't end the server properly
                incomingSocket = accept(serverSocket, (struct sockaddr *)&incomingAddress,
                                        &incomingAddressLength);
                if (incomingSocket > 0)
                {
                    // Successfully accepted a connection.
                    m_socket = incomingSocket;
                }
                else
                {
                    RPMSG_SOCK_DEBUG_ERR("accept failed");
                }
            }
        }
        ::close(serverSocket);
    }
}

