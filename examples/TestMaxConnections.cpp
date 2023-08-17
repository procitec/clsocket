
#include "PassiveSocket.h"

#include <thread>
#include <functional>
#include <cstdio>
#include <cstring>

#define MAX_CLIENTS 65536
#define MAX_PACKET  4096
#define TEST_PACKET "Test"


static inline const char * errtxt(CPassiveSocket& s) {
    return s.GetSocketErrorText();
}

static inline const char * errtxt(CSimpleSocket& s) {
    return s.GetSocketErrorText();
}


typedef CSimpleSocket * CSimpleSocketPtr;

struct thread_data
{
    const int       max_clients;
    const char    * pszServerAddr;
    short int       nPort;
    int             nNumBytesToReceive;
    int             nTotalPayloadSize;
    CPassiveSocket  server_socket;
    int             nClients;
    CSimpleSocketPtr client_sockets[MAX_CLIENTS];
    bool            terminate;
    bool            ready;

    thread_data(int max_clients_)
        : max_clients(max_clients_)
    {
        pszServerAddr = "127.0.0.1";
        nPort = 6789;
        nNumBytesToReceive = 1;
        nTotalPayloadSize = (int)strlen(TEST_PACKET);
        nClients = 0;
        terminate = false;
        ready = false;
        if (!max_clients)
        {
            ready = true; // pseudo, that clients can run
            return;
        }
        for (int k=0; k < max_clients; ++k)
            client_sockets[k] = 0;
        server_socket.EnablePerror();
        if (!server_socket.Initialize())
        {
            fprintf(stderr, "Error: initializing socket: %s\n", errtxt(server_socket));
            return;
        }
        if (!server_socket.SetNonblocking())
        {
            fprintf(stderr, "Error: server/listen socket could set Nonblocking: %s\n", errtxt(server_socket));
            return;
        }
        if (!server_socket.Listen(pszServerAddr, nPort))
        {
            fprintf(stderr, "Error: listen failed: %s\n", errtxt(server_socket));
            return;
        }
        ready = true;
    }

    ~thread_data()
    {
        if (!max_clients)
            return;
        printf("\nserver: shutting down server!\n");
        server_socket.Close();
        printf("server: cleanup/delete connections ..\n");
        for (int k = 0; k < nClients; ++k)
            delete client_sockets[k];
    }

    bool serve();
};


bool thread_data::serve()
{
    if (!ready)
        return false;

    if (!max_clients)
    {
        printf("empty server thread. shuts down immediately.\n");
        return false;
    }

    int fd_max = 0;
    while (!terminate)
    {
        if (nClients >= max_clients)
        {
            fprintf(stderr, "\nserver: have served %d clients. shut down\n", nClients);
            ready = false;
            return false;
        }
        if (!server_socket.WaitUntilReadable(100))
        {
            // fprintf(stderr, "server: wait select\n");
            continue;
        }

        // printf("server: accept next connection %d ..\n", nClients);
        CSimpleSocket *pClient = server_socket.Accept();
        if (!pClient)
        {
            if (server_socket.GetSocketError() == CPassiveSocket::SocketEwouldblock)
            {
                fprintf(stderr, "server: accept() would block\n");
                continue;
            }

            fprintf(stderr, "server: Error at accept() for %d: code %d: %s\n",
                    nClients, (int)server_socket.GetSocketError(), errtxt(server_socket));
            break;
        }
        if ( fd_max < pClient->GetSocketDescriptor() )
            fd_max = pClient->GetSocketDescriptor();
        printf("server: accepted # %d. max fd %d\n", nClients, fd_max);

        client_sockets[nClients++] = pClient;
        int nBytesReceived = 0;
        // following line consumes additional file-descriptor
        pClient->WaitUntilReadable(100);  // wait up to 100 ms
        while (!terminate && nBytesReceived < nTotalPayloadSize)
        {
            if (nBytesReceived += pClient->Receive(nNumBytesToReceive))
            {
                pClient->Send((const uint8 *)pClient->GetData(), pClient->GetBytesReceived());
            }
        }
        // keep the new connection open!
    }

    ready = false;
    return true;
}


static bool init_client(CSimpleSocket &client, int k)
{
    client.EnablePerror();
    if (!client.Initialize())
    {
        fprintf(stderr, "Error: socket %d could not get initialized: %s\n", k, errtxt(client));
        return false;
    }
    if (!client.SetNonblocking())
    {
        fprintf(stderr, "Error: socket %d could not set Nonblocking: %s\n", k, errtxt(client));
        return false;
    }
    return true;
}


static bool connect_new_client(CSimpleSocket &client, int k, const thread_data &thData)
{
    bool conn_ok = client.Open("127.0.0.1", 6789);
    if (!conn_ok)
    {
        fprintf(stderr, "Error: %d could not connect to server: %s\n", k, errtxt(client));
        return false;
    }

    int32 r = client.Send((uint8 *)TEST_PACKET, strlen(TEST_PACKET));
    if (!r)
    {
        fprintf(stderr, "Error: connection %d closed from peer while sending packet: %s\n", k, errtxt(client));
        return false;
    }

    int numBytes = -1;
    int bytesReceived = 0;
    char result[1024];

    client.WaitUntilReadable(100);  // wait up to 100 ms

    // Receive() until socket gets closed or we already received the full packet
    while ( numBytes != 0 && thData.ready && bytesReceived < thData.nTotalPayloadSize )
    {
        numBytes = client.Receive(MAX_PACKET);

        if (numBytes > 0)
        {
            bytesReceived += numBytes;
            // memcpy(result, client.GetData(), numBytes);
            // printf("\nreceived %d bytes: '%s'\n", numBytes, result);
        }
        else if ( CSimpleSocket::SocketEwouldblock == client.GetSocketError() )
            fprintf(stderr, ".");
        else if ( !client.IsNonblocking() && CSimpleSocket::SocketTimedout == client.GetSocketError() )
            fprintf(stderr, "w");
        else
        {
            printf("\n%d: received %d bytes\n", k, numBytes);

            fprintf(stderr, "Error: %s\n", errtxt(client) );
            fprintf(stderr, "Error: %s, code %d\n"
                    , ( client.IsNonblocking() ? "NonBlocking socket" : "Blocking socket" )
                    , (int) client.GetSocketError()
                    );
        }

        if ( bytesReceived >= thData.nTotalPayloadSize )
            return true;  // no need to wait
        // else
        //    printf("read %d of %d\n", bytesReceived, thData.nTotalPayloadSize);

        client.WaitUntilReadable(100);  // wait up to 100 ms
    }

    if (bytesReceived >= thData.nTotalPayloadSize)
        ;  // printf("%d: received %d bytes response.\n", k, bytesReceived);
    else
    {
        fprintf(stderr, "Error at %d: received %d bytes - expected %d!\n", k, bytesReceived, thData.nTotalPayloadSize);
        return false;
    }
    if (!numBytes)
    {
        fprintf(stderr, "Error: connection %d closed from peer: %s\n", k, errtxt(client));
        return false;
    }
    return true;
}



int main(int argc, char **argv)
{
    // max calculation:
    // assuming 'ulimit -n' = 8192:
    // - 4 descriptors for stdin, stdout, stderr and also one server socket
    // - 4 descriptors per connection: 2 at server (socket + epoll) and 2 at client
    // the additional file-descriptor for epoll is needed with first internal Select() ..
    int max_clients = (8192 - 4) / 4;

    int init_sockets_at_startup = 1;

    if ( 1 < argc
        && (!strcmp(argv[1], "--help") || !strcmp(argv[1], "-help") || !strcmp(argv[1], "-h") || !strcmp(argv[1], "/h"))
        )
    {
        printf("usage: %s [-s|-c] [<max_num_connections> [<init_sockets_at_startup>] ]\n", argv[0]);
        printf("  -s : run only server\n");
        printf("  -c : run only client(s)\n");
        printf("       by default, server and client(s) are run\n");
        printf("  max_num_connections: number of connections to test. default = %d, max = %d\n", max_clients, max_clients);
        printf("  init_sockets_at_startup: 0 or 1. default = %d\n", init_sockets_at_startup);
#ifdef __linux__
        printf("\nprobably necessary things on Linux:\n");
        printf("an additional 'ulimit -n <max>' might be necessary\n");
        printf("see https://www.cyberciti.biz/faq/linux-increase-the-maximum-number-of-open-files/\n");
        printf("  cat /proc/sys/fs/file-max\n");
        printf("  sysctl fs.file-max\n");
        printf("  sysctl -w fs.file-max=100000\n");
        printf("  sudo nano /etc/security/limits.conf   # and add following lines replacing 'user'\n");
        printf("    user soft nofile 1048576\n");
        printf("    user hard nofile 1048576  # save and exit - probably logoff and login\n");
        printf("  ulimit -n <max>\n");
        printf("  ulimit -n  # to verfiy new soft limit\n\n");
#endif
        return 0;
    }

    int argoff = 0;
    bool run_server = true, run_clients = true;
    if (1 < argc && !strcmp(argv[1], "-s"))
    {
        ++argoff;
        run_clients = false;
    }
    if (1 < argc && !strcmp(argv[1], "-c"))
    {
        ++argoff;
        run_server = false;
    }

    if ( argoff+1 < argc )
    {
        max_clients = atoi(argv[argoff+1]);
    }

    if ( argoff+2 < argc )
        init_sockets_at_startup = atoi(argv[argoff+2]);

    if (max_clients >= MAX_CLIENTS)
        max_clients = MAX_CLIENTS;

    printf("try to setup/communicate %d conncections ..\n", max_clients);

    thread_data *pThData = new thread_data{run_server ? max_clients : 0};
    thread_data &thData = *pThData;
    if (run_server && !thData.ready)
        return 1;

    std::thread server_thread(&thread_data::serve, &thData);

    if (run_clients)
    {
        CSimpleSocketPtr * clients = new CSimpleSocketPtr[max_clients];
        int fd_max = 0;

        bool have_err = false;
        if (init_sockets_at_startup)
        {
            for (int k = 0; k < max_clients; ++k)
            {
                clients[k] = new CSimpleSocket();
                CSimpleSocket &client = *clients[k];
                printf("init socket %d ..\n", k);
                if (!init_client(client, k))
                {
                    have_err = true;
                    break;
                }
            }
        }

        for (int k = 0; !have_err && thData.ready && k < max_clients; ++k)
        {
            if (!init_sockets_at_startup)
            {
                clients[k] = new CSimpleSocket();
                if (!init_client(*clients[k], k))
                    break;
            }
            CSimpleSocket &client = *clients[k];
            int fd = client.GetSocketDescriptor();
            if ( fd_max < fd )
                fd_max = fd;

            printf("\nclient %d (socket fd %d, max fd %d) ..\n", k, fd, fd_max);
            have_err = !connect_new_client(client, k, thData);
        }

        thData.terminate = true;
        if (run_server)
            printf("trigger server to terminate ..\n");
        else
            printf("can't trigger server to terminate. close server process.\n");

        printf("cleanup clients connections ..\n");
        for (int k=0; k < max_clients; ++k)
        {
            if (!clients[k])
                continue;
            CSimpleSocket &client = *clients[k];
            if (client.IsSocketValid())
                client.Close();
            delete clients[k];
        }
        delete []clients;
    }

    server_thread.join();
    delete pThData;
    return 0;
}
