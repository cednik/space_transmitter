#pragma once

#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiAP.h>

#include <list>

class LineServer {
    struct Client {
        Client(WiFiClient client, std::string buffer = "")
            : client(client), buffer(buffer)
        {}
        WiFiClient client;
        std::string buffer;
    };
    typedef std::list<Client>::iterator pclient_t;

    LineServer(const LineServer&) = delete;
    void operator = (const LineServer&) = delete;

public:
    typedef std::function<void(std::string msg, WiFiClient& client)> Callback;

    LineServer(uint16_t port = 0, Callback onReceive = nullptr)
        : m_server(port), m_onReceive(onReceive)
    {}
    void begin() { m_server.begin(); }
    void begin(uint16_t port){ m_server.begin(port); }
    void begin(uint16_t port, Callback onReceive) {
        m_server.begin(port);
        m_onReceive = onReceive;
    }
    void process() {
        WiFiClient client = m_server.available();
        if (client)
            m_clients.emplace_back(client, "");
        for (auto p_client = m_clients.begin(); p_client != m_clients.end();) {
            while (p_client->client.available()) {
                char c = p_client->client.read();
                if (c == '\n') {
                    _onReceive(p_client);
                    p_client->buffer.clear();
                } else
                    p_client->buffer += c;
            }
            if (p_client->client.connected()) {
                ++p_client;
            } else {
                if (!p_client->buffer.empty()) {
                    debug("Received incomplete packet:");
                    _onReceive(p_client);
                }
                p_client->client.stop();
                auto next = std::next(p_client);
                m_clients.erase(p_client);
                p_client = next;
            }
        }
    }
private:
    void _onReceive(pclient_t pclient) {
        if (m_onReceive)
            m_onReceive(pclient->buffer, pclient->client);
    }
    WiFiServer m_server;
    Callback m_onReceive;
    std::list<Client> m_clients;
};

size_t sendLine(const IPAddress& addr, uint16_t port, const std::string& line) {
    WiFiClient client;
    if (!client.connect(addr, port))
        return false;
    size_t written = client.print((line + '\n').c_str());
    client.flush();
    client.stop();
    return written;
}