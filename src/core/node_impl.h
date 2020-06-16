#pragma once

#include "global_include.h"
#include "mavlink_address.h"
#include "mavlink_include.h"
#include "mavlink_parameters.h"
#include "mavlink_commands.h"
#include "mavlink_message_handler.h"
#include "mavlink_mission_transfer.h"
#include "timeout_handler.h"
#include "call_every_handler.h"
#include "thread_pool.h"
#include "timesync.h"
#include "interface_base.h"
#include <cstdint>
#include <functional>
#include <atomic>
#include <vector>
#include <unordered_set>
#include <map>
#include <thread>
#include <mutex>
#include <future>

namespace mavsdk {

class MavsdkImpl;
class PluginImplBase;

// This class is the impl of Node. this is to hide the private methods
// and functionality from the public library API.
class NodeImpl : public Sender {
public:
    explicit NodeImpl(
            MavsdkImpl& parent, uint8_t system_id, uint8_t component_id);
    ~NodeImpl();

    void process_mavlink_message(mavlink_message_t& message);

    typedef std::function<void(const mavlink_message_t)> mavlink_message_handler_t;

    void register_mavlink_message_handler(
        uint16_t msg_id, mavlink_message_handler_t callback, const void* cookie);

    void unregister_mavlink_message_handler(uint16_t msg_id, const void* cookie);
    void unregister_all_mavlink_message_handlers(const void* cookie);

    void register_timeout_handler(std::function<void()> callback, double duration_s, void** cookie);
    void refresh_timeout_handler(const void* cookie);
    void unregister_timeout_handler(const void* cookie);

    void add_call_every(std::function<void()> callback, float interval_s, void** cookie);
    void change_call_every(float interval_s, const void* cookie);
    void reset_call_every(const void* cookie);
    void remove_call_every(const void* cookie);

    uint8_t get_system_id();
    uint8_t get_component_id();

    uint8_t get_own_system_id();
    uint8_t get_own_component_id();

    bool send_message(mavlink_message_t& message) override;

    typedef std::function<void(MAVLinkCommands::Result, float)> commandResultCallback;

    MAVLinkCommands::Result send_command(MAVLinkCommands::CommandLong& command);
    MAVLinkCommands::Result send_command(MAVLinkCommands::CommandInt& command);

    void send_command_async(
        MAVLinkCommands::CommandLong& command, const commandResultCallback callback);
    void send_command_async(
        MAVLinkCommands::CommandInt& command, const commandResultCallback callback);

    MAVLinkCommands::Result set_msg_rate(
        uint16_t message_id, double rate_hz);

    void set_msg_rate_async(
        uint16_t message_id,
        double rate_hz,
        commandResultCallback callback);

    // TODO refactor
    uint8_t get_own_mav_type() const;

    // TODO refactor
    Time& get_time() { return _time; };
    AutopilotTime& get_autopilot_time() { return _autopilot_time; };

    void register_plugin(PluginImplBase* plugin_impl);
    void unregister_plugin(PluginImplBase* plugin_impl);

    void call_user_callback(const std::function<void()>& func);

    void intercept_incoming_messages(std::function<bool(mavlink_message_t&)> callback);
    void intercept_outgoing_messages(std::function<bool(mavlink_message_t&)> callback);

    bool is_connected();
    void process_heartbeat(const mavlink_message_t& message);

    // Non-copyable
    NodeImpl(const NodeImpl&) = delete;
    const NodeImpl& operator=(const NodeImpl&) = delete;

    MAVLinkAddress target_address{};

    uint8_t get_type() const;
    uint8_t get_autopilot() const;
    uint8_t get_base_mode() const;
    uint8_t get_system_status() const;

    bool is_autopilot() const;

    MAVLinkCommands::CommandLong
    make_command_msg_rate(uint16_t message_id, double rate_hz);

    MAVLinkParameters::Result set_param_float(const std::string& name, float value);
    MAVLinkParameters::Result set_param_int(const std::string& name, int32_t value);
    MAVLinkParameters::Result set_param_ext_float(const std::string& name, float value);
    MAVLinkParameters::Result set_param_ext_int(const std::string& name, int32_t value);

    typedef std::function<void(MAVLinkParameters::Result result)> success_t;
    void set_param_float_async(
            const std::string& name, float value, success_t callback, const void* cookie);
    void set_param_int_async(
            const std::string& name, int32_t value, success_t callback, const void* cookie);
    void set_param_ext_float_async(
            const std::string& name, float value, success_t callback, const void* cookie);
    void set_param_ext_int_async(
            const std::string& name, int32_t value, success_t callback, const void* cookie);

    typedef std::function<void(MAVLinkParameters::Result result, float value)>
        get_param_float_callback_t;
    typedef std::function<void(MAVLinkParameters::Result result, int32_t value)>
        get_param_int_callback_t;

    static void receive_float_param(
        MAVLinkParameters::Result result,
        MAVLinkParameters::ParamValue value,
        get_param_float_callback_t callback);
    static void receive_int_param(
        MAVLinkParameters::Result result,
        MAVLinkParameters::ParamValue value,
        get_param_int_callback_t callback);

    std::pair<MAVLinkParameters::Result, float> get_param_float(const std::string& name);
    std::pair<MAVLinkParameters::Result, int> get_param_int(const std::string& name);
    std::pair<MAVLinkParameters::Result, float> get_param_ext_float(const std::string& name);
    std::pair<MAVLinkParameters::Result, int> get_param_ext_int(const std::string& name);

    // These methods can be used to cache a parameter when a system connects. For that
    // the callback can just be set to nullptr.
    void get_param_float_async(
        const std::string& name, get_param_float_callback_t callback, const void* cookie);
    void get_param_int_async(
        const std::string& name, get_param_int_callback_t callback, const void* cookie);
    void get_param_ext_float_async(
        const std::string& name, get_param_float_callback_t callback, const void* cookie);
    void get_param_ext_int_async(
        const std::string& name, get_param_int_callback_t callback, const void* cookie);

private:
    MavsdkImpl& _parent;
    std::atomic<bool> _should_exit{false};
    MAVLinkAddress node_address{};

    void heartbeats_timed_out();

    void system_thread();

    Time _time{};
    AutopilotTime _autopilot_time{};

    // Needs to be before anything else because they can depend on it.
    MAVLinkMessageHandler _message_handler{};

    commandResultCallback _command_result_callback{nullptr};

    std::thread* _system_thread{nullptr};

    static constexpr double _HEARTBEAT_TIMEOUT_S = 3.0;

    std::mutex _connection_mutex{};
    bool _connected{false};
    void* _heartbeat_timeout_cookie = nullptr;

    static constexpr double _HEARTBEAT_SEND_INTERVAL_S = 1.0;

    MAVLinkParameters _params;
    MAVLinkCommands _commands;

    Timesync _timesync;

    TimeoutHandler _timeout_handler;
    CallEveryHandler _call_every_handler;

    std::mutex _plugin_impls_mutex{};
    std::vector<PluginImplBase*> _plugin_impls{};

    ThreadPool _thread_pool{3};

    mavlink_heartbeat_t _heartbeat;

    std::vector<InterfaceBase> _interfaces;
};

} // namespace mavsdk
