#pragma once

#include <AsyncUdp.h>

#include <functional>
#include <string>
#include <list>
#include <mutex>
#include <queue>

class SyncTask {

    struct Packet {
        Packet(IPAddress la, uint16_t lp, IPAddress ra, uint16_t rp, const std::string& data, char cast)
            : localIP(la), localPort(lp), remoteIP(ra), remotePort(rp), data(data), cast_type(cast)
        {}
        IPAddress localIP;
        uint16_t localPort;
        IPAddress remoteIP;
        uint16_t remotePort;
        std::string data;
        char cast_type;
    };

public:
    typedef std::function<void(std::string tag, IPAddress addr, uint16_t port)> Task;

    static void begin(uint16_t port) {
        if (s_socket.listenMulticast({0xE0, 0, 0, 0}, port)) {
            s_socket.onPacket([&](AsyncUDPPacket packet) {
                // std::lock_guard<std::mutex>guard(s_mutex);
                // s_packet_que.emplace(
                //     packet.localIP(),
                //     packet.localPort(),
                //     packet.remoteIP(),
                //     packet.remotePort(),
                //     std::string(reinterpret_cast<const char*>(packet.data()), packet.length()),
                //     packet.isBroadcast()?'B':packet.isMulticast()?'M':'U'
                // );
                Serial.print("UDP Packet Type: ");
                Serial.print(packet.isBroadcast()?"Broadcast":packet.isMulticast()?"Multicast":"Unicast");
                Serial.print(", From: ");
                Serial.print(packet.remoteIP());
                Serial.print(":");
                Serial.print(packet.remotePort());
                Serial.print(", To: ");
                Serial.print(packet.localIP());
                Serial.print(":");
                Serial.print(packet.localPort());
                Serial.print(", Length: ");
                Serial.print(packet.length());
                Serial.print(", Data: ");
                Serial.write(packet.data(), packet.length());
                Serial.println();
            } );
        } else {
            debug("Can not start UDP sync server");
            trap("Can not start UDP sync server");
        }
    }
    static void create(const std::string& tag, Task task) {
        s_tasks.emplace_front(tag, task);
    }
    static void sync(uint16_t port, const std::string& tag) {
        WiFiUDP udp;
        udp.beginPacket({255, 255, 255, 255}, port);
        udp.write(reinterpret_cast<const uint8_t*>(tag.data()), tag.size());
        udp.endPacket();
    }
    static void process() {
        std::lock_guard<std::mutex>guard(s_mutex);
        if (s_packet_que.empty())
            return;
        auto& packet = s_packet_que.front();
        debug(format("Receive sync packet \"{}\" from {}:{}, cast {}", packet.data, packet.remoteIP.toString().c_str(), packet.remotePort, packet.cast_type));
        for (auto p_task = s_tasks.begin(); p_task != s_tasks.end();) {
            if (packet.data == p_task->m_tag) {
                if (p_task->m_task)
                    p_task->m_task(packet.data, packet.remoteIP, packet.remotePort);
                auto next = std::next(p_task);
                s_tasks.erase(p_task);
                p_task = next;
            } else {
                ++p_task;
            }
        }
        s_packet_que.pop();
    }
    SyncTask(const std::string& tag, Task task)
        : m_tag(tag), m_task(task)
    {}
private:
    SyncTask(const SyncTask&) = delete;
    void operator = (const SyncTask&) = delete;

    const std::string m_tag;
    Task m_task;

    static std::list<SyncTask> s_tasks;
    static AsyncUDP s_socket;
    static std::mutex s_mutex;
    static std::queue<Packet> s_packet_que;
};

std::list<SyncTask> SyncTask::s_tasks;
AsyncUDP SyncTask::s_socket;
std::mutex SyncTask::s_mutex;
std::queue<SyncTask::Packet> SyncTask::s_packet_que;

