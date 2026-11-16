#pragma once

#include "SessionTypes.h"

#include <map>
#include <vector>

namespace Session
{

// Byte pipe between one server and its clients. core/ does not open sockets;
// a platform adapter implements this with TCP or UDP. InProcessTransport is
// the reference used by tests and by local play: every send is encoded and
// decoded so the wire format is on the path.
class ISessionTransport
{
public:
    virtual ~ISessionTransport() = default;

    virtual void SendToServer(const Envelope& message) = 0;
    virtual void SendToClient(uint32_t playerId, const Envelope& message) = 0;
    virtual void PollServer(std::vector<Envelope>& out) = 0;
    virtual void PollClient(uint32_t playerId, std::vector<Envelope>& out) = 0;
};

class InProcessTransport final : public ISessionTransport
{
public:
    void SendToServer(const Envelope& message) override;
    void SendToClient(uint32_t playerId, const Envelope& message) override;
    void PollServer(std::vector<Envelope>& out) override;
    void PollClient(uint32_t playerId, std::vector<Envelope>& out) override;

private:
    std::vector<std::vector<uint8_t>> toServer_;
    std::map<uint32_t, std::vector<std::vector<uint8_t>>> toClients_;
};

}
