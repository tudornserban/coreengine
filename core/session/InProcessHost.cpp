#include "InProcessHost.h"

#include "SessionCodec.h"

namespace Session
{

void InProcessTransport::SendToServer(const Envelope& message)
{
    std::vector<uint8_t> bytes = EncodeEnvelope(message);
    if (!bytes.empty())
    {
        toServer_.push_back(std::move(bytes));
    }
}

void InProcessTransport::SendToClient(uint32_t playerId, const Envelope& message)
{
    Envelope stamped = message;
    stamped.playerId = playerId;
    std::vector<uint8_t> bytes = EncodeEnvelope(stamped);
    if (!bytes.empty())
    {
        toClients_[playerId].push_back(std::move(bytes));
    }
}

void InProcessTransport::PollServer(std::vector<Envelope>& out)
{
    out.clear();
    for (const std::vector<uint8_t>& frame : toServer_)
    {
        Envelope message;
        if (DecodeEnvelope(frame, message))
        {
            out.push_back(std::move(message));
        }
    }
    toServer_.clear();
}

void InProcessTransport::PollClient(uint32_t playerId, std::vector<Envelope>& out)
{
    out.clear();
    const auto it = toClients_.find(playerId);
    if (it == toClients_.end())
    {
        return;
    }
    for (const std::vector<uint8_t>& frame : it->second)
    {
        Envelope message;
        if (DecodeEnvelope(frame, message))
        {
            out.push_back(std::move(message));
        }
    }
    it->second.clear();
}

InProcessHost::InProcessHost(std::unique_ptr<IMatchFactory> rules)
    : server_(std::move(rules))
{
}

SessionClient* InProcessHost::Connect(const std::string& name)
{
    const uint32_t playerId = server_.Connect(name);
    if (playerId == 0)
    {
        return nullptr;
    }
    auto client = std::make_unique<SessionClient>();
    client->Bind(playerId, name);
    SessionClient* raw = client.get();
    clients_.push_back(std::move(client));
    return raw;
}

void InProcessHost::Disconnect(SessionClient& client)
{
    server_.Disconnect(client.PlayerId());
    client.MarkDisconnected();
    Pump();
}

SessionClient* InProcessHost::FindClient(uint32_t playerId)
{
    for (const std::unique_ptr<SessionClient>& client : clients_)
    {
        if (client->PlayerId() == playerId)
        {
            return client.get();
        }
    }
    return nullptr;
}

void InProcessHost::Pump()
{
    for (int round = 0; round < 32; ++round)
    {
        bool any = false;
        for (const std::unique_ptr<SessionClient>& client : clients_)
        {
            const std::vector<Envelope> outbound = client->TakeOutbound();
            for (const Envelope& message : outbound)
            {
                Envelope stamped = message;
                stamped.playerId = client->PlayerId();
                transport_.SendToServer(stamped);
                any = true;
            }
        }

        std::vector<Envelope> toServer;
        transport_.PollServer(toServer);
        for (const Envelope& message : toServer)
        {
            server_.Receive(message.playerId, message);
            any = true;
        }

        const std::vector<Envelope> toClients = server_.TakeOutbound();
        for (const Envelope& message : toClients)
        {
            transport_.SendToClient(message.playerId, message);
            any = true;
        }

        for (const std::unique_ptr<SessionClient>& client : clients_)
        {
            std::vector<Envelope> incoming;
            transport_.PollClient(client->PlayerId(), incoming);
            for (const Envelope& message : incoming)
            {
                client->Receive(message);
                any = true;
            }
        }

        if (!any)
        {
            return;
        }
    }
}

void InProcessHost::AdvanceOnline()
{
    server_.AdvanceOnlineMatches();
    Pump();
}

}
