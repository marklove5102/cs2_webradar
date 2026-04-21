#include "pch.hpp"

bool main()
{
    config_data_t config_data = {};
    if (!cfg::setup(config_data))
    {
        std::this_thread::sleep_for(std::chrono::seconds(5));
        return {};
    }
    LOG_INFO("config system initialization completed");

    if (!m_memory->setup())
    {
        std::this_thread::sleep_for(std::chrono::seconds(5));
        return {};
    }
    LOG_INFO("memory initialization completed");

    if (!i::setup())
    {
        std::this_thread::sleep_for(std::chrono::seconds(5));
        return {};
    }
    LOG_INFO("interfaces initialization completed");

    if (!schema::setup())
    {
        std::this_thread::sleep_for(std::chrono::seconds(5));
        return {};
    }
    LOG_INFO("schema initialization completed");

    ix::initNetSystem();
    LOG_INFO("winsock initialization completed");

    const auto ipv4_address = utils::get_ipv4_address(config_data);
    if (ipv4_address.empty())
        LOG_WARNING("failed to automatically get your ipv4 address!\n                 we will use '%s' from 'config.json'. if the local ip is wrong, please set it", config_data.m_local_ip);

    const auto formatted_address = std::format("ws://{}:22006/cs2_webradar", ipv4_address);

    static ix::WebSocket web_socket;
    static bool connected = false;
    static bool failed = false;

    web_socket.setUrl(formatted_address);
    web_socket.setOnMessageCallback([&](const ix::WebSocketMessagePtr& msg)
    {
        if (msg->type == ix::WebSocketMessageType::Open)
        {
            connected = true;
            LOG_INFO("connected to the web socket ('%s')", formatted_address.c_str());
        }
        else if (msg->type == ix::WebSocketMessageType::Error)
        {
            failed = true;
            LOG_ERROR("failed to connect to the web socket ('%s')", formatted_address.c_str());
        }
    });
    web_socket.start();

    while (!connected && !failed)
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

    if (!connected)
        return {};

    auto start = std::chrono::system_clock::now();

    for (;;)
    {
        const auto now = std::chrono::system_clock::now();

        if (now - start >= std::chrono::milliseconds(100))
        {
            start = now;

            sdk::update();
            f::run();

            web_socket.send(f::m_data.dump());
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    system("pause");

    return true;
}