#pragma once

#include "ISessionTransport.h"
#include "SessionClient.h"
#include "SessionServer.h"

#include <memory>
#include <vector>

namespace Session
{

// Reference client-server in one process. Pump moves encoded envelopes both
// ways. AdvanceOnline steps the server clock and then delivers the confirms.
// A deployed server uses the same SessionServer behind a socket transport.
class InProcessHost
{
public:
    explicit InProcessHost(std::unique_ptr<IMatchFactory> rules);

    SessionClient* Connect(const std::string& name);
    void Disconnect(SessionClient& client);
    void Pump();
    void AdvanceOnline();

    SessionServer& Server() { return server_; }
    const SessionServer& Server() const { return server_; }
    ISessionTransport& Transport() { return transport_; }

private:
    SessionClient* FindClient(uint32_t playerId);

    InProcessTransport transport_;
    SessionServer server_;
    std::vector<std::unique_ptr<SessionClient>> clients_;
};

}
