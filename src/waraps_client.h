#ifndef waraps_client_H
#define waraps_client_H

#include <mqtt/async_client.h>
#include <nlohmann/json.hpp>

#define DEFAULT_HEARTBEAT_INTERVAL 1000

/**
 * A class to handle MQTT communication with the WARA PS MQTT broker.
 * Built to WARA PS message specifications as a L2 unit.
 * For more info see https://api.docs.waraps.org/
 *
 * Author: Janne Schyffert
 */
class waraps_client
{
private:
    const std::chrono::milliseconds heartbeat_interval =
        std::chrono::milliseconds(DEFAULT_HEARTBEAT_INTERVAL);
    const std::string UNIT_NAME, SERVER_ADDRESS;

    std::string static generate_uuid();
    static std::string generate_full_topic(const std::string& topic) ;
    std::string generate_heartbeat_message() const;
    bool handle_message(const mqtt::const_message_ptr& msg);
    void cmd_pong(nlohmann::json msg_payload);
    void cmd_stop(nlohmann::json msg_payload);

    // Commands get a separate function and map to allow different command callbacks as well as user-defined command callbacks
    void handle_command(nlohmann::json msg_payload);

    std::string uuid = generate_uuid();
    std::thread heartbeat_thread, consume_thread;
    mqtt::async_client client;
    std::shared_ptr<bool> is_running = std::make_shared<bool>(false);
    std::map<std::string, std::function<void(waraps_client *, nlohmann::json)>> message_callbacks{
        {"exec/command", &waraps_client::handle_command}};
    std::map<std::string, std::function<void(waraps_client *, nlohmann::json)>> command_callbacks{
        {"stop", &waraps_client::cmd_stop},
        {"pong", &waraps_client::cmd_pong}};

public:
    waraps_client(std::string name, std::string server_address);

    /**
     * Deleted constructors to disallow copying and moving of the client
     */
    waraps_client(waraps_client &&) = delete;
    waraps_client(const waraps_client &) = delete;
    waraps_client &operator=(const waraps_client &) = delete;
    waraps_client &operator=(waraps_client &&) = delete;

    ~waraps_client();

    /**
     * Check if the client is running and consuming messages
     * @return true if the client is running, false otherwise
     */
    bool running() const;

    /**
     * Start main thread and heartbeat thread and begin consuming messages from the MQTT server.
     * Will not block the current thread.
     */
    void start();

    /**
     * Stop consuming messages and disable the heartbeat thread, allowing the client to be destroyed.
     */
    void stop();

    /**
     * Publish a message to the MQTT server asynchronously.
     * @param topic The topic to publish the message to, will be added to WARA PS topic prefix
     * @param payload The message to publish as a JSON string
     */
    bool publish_message_async(const std::string& topic, const std::string& payload);

    /**
     * Set an asyncronous callback function to be called when a message is recieved on the specified topic.
     * Disallows setting the "command" topic callback, see set_command_callback for that.
     * @param topic the topic to listen on, will be added to WARA PS topic prefix.
     * @param callback the function to call when a message is recieved on the specified topic, takes waraps_client as a parameter to allow for state changes if needed.
     * @throws std::invalid_argument if the topic is "exec/command"
     */
    void set_message_callback(const std::string& topic, std::function<void(waraps_client *, nlohmann::json)> callback);

    /**
     * Set an asyncronous callback function to be called when a message is recieved on the specified topic.
     * Disallows setting the "command" topic callback, see set_command_callback for that.
     * @param topic the topic to listen on, will be added to WARA PS topic prefix.
     * @param callback the function to call when a message is recieved on the specified topic
     * @throws std::invalid_argument if the topic is "exec/command"
     */
    void set_message_callback(std::string topic, std::function<void(nlohmann::json)> callback);
};

#endif