#include <reactor/network/rdv-socket.hh>

#include <reactor/network/resolve.hh>
#include <reactor/network/buffer.hh>
#include <reactor/network/rdv.hh>
#include <reactor/network/exception.hh>
#include <reactor/scheduler.hh>

ELLE_LOG_COMPONENT("rdv.socket");

namespace reactor
{
  namespace network
  {

    using Endpoint = boost::asio::ip::udp::endpoint;
    RDVSocket::RDVSocket()
    : _breacher("breacher", [this] { this->loop_breach();})
    , _keep_alive("keep-alive", [this]  {this->loop_keep_alive();})
    {
    }

    RDVSocket::~RDVSocket()
    {
      _breacher.terminate_now();
      _keep_alive.terminate_now();
    }

    void RDVSocket::rdv_connect(std::string const& id,
                                std::string const& rdv_host, int rdv_port,
                                DurationOpt timeout)
    {
      rdv_connect(id, resolve_udp(rdv_host, std::to_string(rdv_port)), timeout);
    }
    void RDVSocket::rdv_connect(std::string const& id, Endpoint ep, DurationOpt timeout)
    {
      _id = id;
      ELLE_TRACE_SCOPE("rdv_connect to %s as %s", ep, id);
      _server_reached.close();
      _server = ep;
      rdv::Message req;
      req.command = rdv::Command::ping;
      req.id = id;
      elle::Buffer buf = elle::serialization::json::serialize(req, false);
      auto now = boost::posix_time::second_clock::universal_time();
      while (true)
      {
        send_to(Buffer(buf), ep);
        if (reactor::wait(_server_reached, 500_ms))
          return;
        if (timeout
          && boost::posix_time::second_clock::universal_time() - now > *timeout)
          throw TimeOut();
      }
    }
    Size RDVSocket::receive_from(Buffer buffer,
        boost::asio::ip::udp::endpoint &endpoint,
        DurationOpt timeout)
    {
      while (true)
      {
        Size sz = UDPSocket::receive_from(buffer, endpoint, timeout);
        if (sz < 8)
          return sz;
        if (endpoint == _server && !_server_reached.opened())
        {
          ELLE_TRACE("message from server, open reached");
          _server_reached.open();
        }
        if (!memcmp(rdv::rdv_magic, buffer.data(), 8))
        {
          rdv::Message repl = elle::serialization::json::deserialize<rdv::Message>(
            elle::Buffer(buffer.data()+8, sz - 8), false);
          ELLE_DEBUG("got message from %s, code %s", endpoint, (int)repl.command);
          switch (repl.command)
          {
          case rdv::Command::ping:
            {
              rdv::Message reply;
              reply.id = _id;
              reply.command = rdv::Command::pong;
              reply.source_endpoint = endpoint;
              elle::Buffer buf = elle::serialization::json::serialize(reply, false);
              send_with_magik(buf, endpoint);
            }
            break;
          case rdv::Command::pong:
            {
              ELLE_DEBUG("pong from %s", repl.id);
              auto it = _contacts.find(repl.id);
              if (it != _contacts.end())
              {
                ELLE_TRACE("opening result barrier");
                it->second.result = endpoint;
                it->second.barrier->open();
              }
            }
            break;
          case rdv::Command::connect:
            {
              ELLE_TRACE("connect result tgt=%s, peer=%s", *repl.target_address,
                !!repl.target_endpoint);
              auto it = _contacts.find(*repl.target_address);
              if (it != _contacts.end() && !it->second.barrier->opened())
              {
                if (repl.target_endpoint)
                {
                  // set result but do not open barrier yet, so that
                  // contact() can retry pinging it
                  it->second.result = repl.target_endpoint;
                  // give it a ping
                  send_ping(*repl.target_endpoint);
                }
                else
                { // nothing to do, contact() will resend periodically
                }
              }
            }
            break;
          case rdv::Command::connect_requested:
            { // add to breach requests
              ELLE_ASSERT(repl.target_endpoint);
              ELLE_TRACE("connect_requested, id=%s, ep=%s",
                repl.id, *repl.target_endpoint);
              auto it = std::find_if(_breach_requests.begin(), _breach_requests.end(),
                [&](std::pair<Endpoint, int>const& b) {
                  return b.first == *repl.target_endpoint;
                });
              if (it != _breach_requests.end())
                it->second += 5;
              else
                _breach_requests.push_back(
                  std::make_pair(*repl.target_endpoint, 5));
            }
            break;
          }
        }
        else
          return sz;
      }
    }
    Endpoint RDVSocket::contact(std::string const& id,
        std::vector<Endpoint> const& endpoints,
        DurationOpt timeout)
    {
      if (_contacts.find(id) != _contacts.end())
        throw elle::Error("contact already in progress");
      {
        ContactInfo& ci = _contacts[id];
        ci.barrier.reset(new Barrier());
        ci.barrier->close();
      }
      auto now = boost::posix_time::second_clock::universal_time();
      while (true)
      {
        if (!endpoints.empty())
        { // try known endpoints
          for (auto const& ep: endpoints)
            send_ping(ep);
        }
        // try establishing link through rdv
        auto const& c = _contacts.at(id);
        if (!c.barrier->opened() && _server_reached.opened())
        {
          if (c.result)
          { // RDV gave us an enpoint, but we are not connected to it yet, ping it
            send_ping(*c.result);
          }
          else
          {
            rdv::Message req;
            req.command = rdv::Command::connect;
            req.id = _id;
            req.target_address = id;
            elle::Buffer buf = elle::serialization::json::serialize(req, false);
            send_to(buf, _server);
          }
        }
        if (reactor::wait(*_contacts.at(id).barrier, 500_ms))
        {
          auto res = *_contacts[id].result;
          _contacts.erase(id);
          return res;
        }
        if (timeout
          && boost::posix_time::second_clock::universal_time() - now > *timeout)
          throw TimeOut();
      }
    }

    void RDVSocket::send_with_magik(elle::Buffer const& b, Endpoint peer)
    {
      elle::Buffer data;
      data.append(reactor::network::rdv::rdv_magic, 8);
      data.append(b.contents(), b.size());
      send_to(Buffer(data.contents(), data.size()), peer);
    }

    void RDVSocket::send_ping(Endpoint target)
    {
      ELLE_DEBUG("send ping to %s", target);
      rdv::Message ping;
      ping.command = rdv::Command::ping;
      ping.id = _id;
      ping.source_endpoint = target;
      elle::Buffer buf = elle::serialization::json::serialize(ping, false);
      send_with_magik(buf, target);
    }

    void RDVSocket::loop_breach()
    {
      while (true)
      {
        for (int i=0; i<signed(_breach_requests.size()); ++i)
        {
          auto& b = _breach_requests[i];
          send_ping(b.first);
          if (!--b.second)
          {
            std::swap(_breach_requests[i],
                      _breach_requests[_breach_requests.size()-1]);
            _breach_requests.pop_back();
            --i;
          }
        }
        reactor::sleep(500_ms);
      }
    }

    void RDVSocket::loop_keep_alive()
    {
      reactor::wait(_server_reached);
      while (true)
      {
        send_ping(_server);
        reactor::sleep(30_sec);
      }
    }
  }
}