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

#include "erpc_sock_transport.hpp"

#include <cstdio>
#include <cstring>

extern "C" {
// Set this to 1 to enable debug logging.
// TODO fix issue with the transport not working on Linux if debug logging is disabled.
#define SOCK_TRANSPORT_DEBUG_LOG (0)

#if SOCK_TRANSPORT_DEBUG_LOG
#if ERPC_HAS_POSIX
#if defined(__MINGW32__)
#error Missing implementation for mingw.
#endif
#include <err.h>
#endif
#endif
#include <errno.h>
#if defined(__MINGW32__)
#include <ws2def.h>
#else
#include <sys/socket.h>
#endif
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
}

using namespace erpc;

////////////////////////////////////////////////////////////////////////////////
// Code
////////////////////////////////////////////////////////////////////////////////

SockTransport::SockTransport(bool isServer) :
m_isServer(isServer)
#if defined(__MINGW32__)
,
m_socket(INVALID_SOCKET)
#else
,
m_socket(-1)
#endif
,
m_runServer(true), m_serverThread(serverThreadStub)
{
#if defined(__MINGW32__)
    WSADATA ws;
    WSAStartup(MAKEWORD(2, 2), &ws);
#endif
}

SockTransport::~SockTransport(void) {}

erpc_status_t SockTransport::open(void)
{
    erpc_status_t status;

    if (m_isServer)
    {
        m_runServer = true;
        m_serverThread.start(this);
        status = kErpcStatus_Success;
    }
    else
    {
        status = connectClient();
    }

    return status;
}

erpc_status_t SockTransport::close(bool stopServer)
{
    if (m_isServer && stopServer)
    {
        m_runServer = false;
    }

#if defined(__MINGW32__)
    if (m_socket != INVALID_SOCKET)
    {
        closesocket(m_socket);
        m_socket = INVALID_SOCKET;
    }
#else
    if (m_socket != -1)
    {
        ::close(m_socket);
        m_socket = -1;
    }
#endif

    return kErpcStatus_Success;
}

erpc_status_t SockTransport::underlyingReceive(uint8_t *data, uint32_t size)
{
    ssize_t length;
    erpc_status_t status = kErpcStatus_Success;

    // Block until we have a valid connection.
#if defined(__MINGW32__)
    while (m_socket == INVALID_SOCKET)
#else
    while (m_socket <= 0)
#endif
    {
        // Sleep 10 ms.
        Thread::sleep(10000);
    }

    // Loop until all requested data is received.
    while (size > 0U)
    {
#if defined(__MINGW32__)
        length = recv(m_socket, data, size, 0);
#else
        length = read(m_socket, data, size);
#endif

        // Length will be zero if the connection is closed.
        if (length > 0)
        {
            size -= length;
            data += length;
        }
        else
        {
            if (length == 0)
            {
                // close socket, not server
                close(false);
                status = kErpcStatus_ConnectionClosed;
            }
            else
            {
                status = kErpcStatus_ReceiveFailed;
            }
            break;
        }
    }

    return status;
}

erpc_status_t SockTransport::underlyingSend(const uint8_t *data, uint32_t size)
{
    erpc_status_t status = kErpcStatus_Success;
    ssize_t result;

#if defined(__MINGW32__)
    if (m_socket == INVALID_SOCKET)
#else
    if (m_socket <= 0)
#endif
    {
        // we should not pretend to have a succesful Send or we create a deadlock
        status = kErpcStatus_ConnectionFailure;
    }
    else
    {
        // Loop until all data is sent.
        while (size > 0U)
        {
#if defined(__MINGW32__)
            result = ::send(m_socket, data, size, 0);
#else
            result = write(m_socket, data, size);
#endif
            if (result >= 0)
            {
                size -= result;
                data += result;
            }
            else
            {
                if (result == EPIPE)
                {
                    // close socket, not server
                    close(false);
                    status = kErpcStatus_ConnectionClosed;
                }
                else
                {
                    status = kErpcStatus_SendFailed;
                }
                break;
            }
        }
    }

    return status;
}

void SockTransport::serverThreadStub(void *arg)
{
    SockTransport *This = reinterpret_cast<SockTransport *>(arg);

    if (This != NULL)
    {
        This->serverThread();
    }
}
