#include <functional>
#include <cstring>
#include "info_impl.h"
#include "autopilot_interface.h"
#include "autopilot_interface_impl.h"
#include "global_include.h"

namespace mavsdk {

InfoImpl::InfoImpl(AutopilotInterface& interface) : PluginImplBase(),
    _interface(interface.impl())
{
    _interface->register_plugin(this);
}

InfoImpl::~InfoImpl()
{
    _interface->unregister_plugin(this);
}

void InfoImpl::init()
{
    using namespace std::placeholders; // for `_1`

    _interface->register_mavlink_message_handler(
        MAVLINK_MSG_ID_AUTOPILOT_VERSION,
        std::bind(&InfoImpl::process_autopilot_version, this, _1),
        this);

    _interface->register_mavlink_message_handler(
        MAVLINK_MSG_ID_FLIGHT_INFORMATION,
        std::bind(&InfoImpl::process_flight_information, this, _1),
        this);
}

void InfoImpl::deinit()
{
    _interface->unregister_all_mavlink_message_handlers(this);
}

void InfoImpl::enable()
{
    // We can't rely on System to request the autopilot_version,
    // so we do it here, anyway.
    _interface->send_autopilot_version_request();
    _interface->send_flight_information_request();

    // We're going to retry until we have the version.
    _interface->add_call_every(
        std::bind(&InfoImpl::request_version_again, this), 1.0f, &_call_every_cookie);

    // We're going to periodically ask for the flight information
    _interface->add_call_every(
        std::bind(&InfoImpl::request_flight_information, this),
        1.0f,
        &_flight_info_call_every_cookie);
}

void InfoImpl::disable()
{
    _interface->remove_call_every(_call_every_cookie);
    _interface->remove_call_every(_flight_info_call_every_cookie);

    {
        std::lock_guard<std::mutex> lock(_mutex);
        _information_received = false;
        _flight_information_received = false;
    }
}

void InfoImpl::request_version_again()
{
    {
        std::lock_guard<std::mutex> lock(_mutex);
        if (_information_received) {
            _interface->remove_call_every(_call_every_cookie);
            return;
        }
    }

    _interface->send_autopilot_version_request();
}

void InfoImpl::request_flight_information()
{
    // We will request new flight information from the autopilot only if
    // we go from an armed to disarmed state or if we haven't received any
    // information yet
    if ((_was_armed && !_interface->is_armed()) || !_flight_information_received) {
        _interface->send_flight_information_request();
    }

    _was_armed = _interface->is_armed();
}

void InfoImpl::process_autopilot_version(const mavlink_message_t& message)
{
    std::lock_guard<std::mutex> lock(_mutex);

    mavlink_autopilot_version_t autopilot_version;
    mavlink_msg_autopilot_version_decode(&message, &autopilot_version);

    _version.flight_sw_major = (autopilot_version.flight_sw_version >> (8 * 3)) & 0xFF;
    _version.flight_sw_minor = (autopilot_version.flight_sw_version >> (8 * 2)) & 0xFF;
    _version.flight_sw_patch = (autopilot_version.flight_sw_version >> (8 * 1)) & 0xFF;

    // first three bytes of flight_custom_version (little endian) describe vendor version
    _version.flight_sw_git_hash = swap_and_translate_binary_to_str(
        autopilot_version.flight_custom_version + 3,
        sizeof(autopilot_version.flight_custom_version) - 3);

    _version.flight_sw_vendor_major = autopilot_version.flight_custom_version[2];
    _version.flight_sw_vendor_minor = autopilot_version.flight_custom_version[1];
    _version.flight_sw_vendor_patch = autopilot_version.flight_custom_version[0];

    _version.os_sw_major = (autopilot_version.os_sw_version >> (8 * 3)) & 0xFF;
    _version.os_sw_minor = (autopilot_version.os_sw_version >> (8 * 2)) & 0xFF;
    _version.os_sw_patch = (autopilot_version.os_sw_version >> (8 * 1)) & 0xFF;

    // Debug() << "flight version: "
    //     << _version.flight_sw_major
    //     << "."
    //     << _version.flight_sw_minor
    //     << "."
    //     << _version.flight_sw_patch;

    // Debug() << "os version: "
    //     << _version.os_sw_major
    //     << "."
    //     << _version.os_sw_minor
    //     << "."
    //     << _version.os_sw_patch;

    _version.os_sw_git_hash = swap_and_translate_binary_to_str(
        autopilot_version.os_custom_version, sizeof(autopilot_version.os_custom_version));

    _product.vendor_id = autopilot_version.vendor_id;
    _product.vendor_name = vendor_id_str(autopilot_version.vendor_id);

    _product.product_id = autopilot_version.product_id;
    _product.product_name = product_id_str(autopilot_version.product_id);

    _identification.hardware_uid =
        translate_binary_to_str(autopilot_version.uid2, sizeof(autopilot_version.uid2));

    _information_received = true;
}

void InfoImpl::process_flight_information(const mavlink_message_t& message)
{
    std::lock_guard<std::mutex> lock(_mutex);

    mavlink_flight_information_t flight_information;
    mavlink_msg_flight_information_decode(&message, &flight_information);

    _flight_info.time_boot_ms = flight_information.time_boot_ms;
    _flight_info.flight_uid = flight_information.flight_uuid;

    _flight_information_received = true;
}

std::string InfoImpl::swap_and_translate_binary_to_str(uint8_t* binary, unsigned binary_len)
{
    std::string str(binary_len * 2, '0');

    for (unsigned i = 0; i < binary_len; ++i) {
        // One hex number occupies 2 chars.
        // The binary is in little endian, therefore we need to swap the bytes for us to read.
        snprintf(&str[i * 2], str.length() - i * 2, "%02x", binary[binary_len - 1 - i]);
    }

    return str;
}

std::string InfoImpl::translate_binary_to_str(uint8_t* binary, unsigned binary_len)
{
    std::string str(binary_len * 2 + 1, '0');

    for (unsigned i = 0; i < binary_len; ++i) {
        // One hex number occupies 2 chars.
        snprintf(&str[i * 2], str.length() - i * 2, "%02x", binary[i]);
    }

    return str;
}

std::pair<Info::Result, Info::Identification> InfoImpl::get_identification() const
{
    std::lock_guard<std::mutex> lock(_mutex);
    return std::make_pair<>(
        (_information_received ? Info::Result::Success : Info::Result::InformationNotReceivedYet),
        _identification);
}

std::pair<Info::Result, Info::Version> InfoImpl::get_version() const
{
    std::lock_guard<std::mutex> lock(_mutex);
    return std::make_pair<>(
        (_information_received ? Info::Result::Success : Info::Result::InformationNotReceivedYet),
        _version);
}

std::pair<Info::Result, Info::Product> InfoImpl::get_product() const
{
    std::lock_guard<std::mutex> lock(_mutex);
    return std::make_pair<>(
        (_information_received ? Info::Result::Success : Info::Result::InformationNotReceivedYet),
        _product);
}

std::pair<Info::Result, Info::FlightInfo> InfoImpl::get_flight_information() const
{
    std::lock_guard<std::mutex> lock(_mutex);
    return std::make_pair<>(
        (_flight_information_received ? Info::Result::Success :
                                        Info::Result::InformationNotReceivedYet),
        _flight_info);
}

const std::string InfoImpl::vendor_id_str(uint16_t vendor_id)
{
    switch (vendor_id) {
        case 0x26ac:
            return "3D Robotics Inc.";
        default:
            return "undefined";
    }
}

const std::string InfoImpl::product_id_str(uint16_t product_id)
{
    switch (product_id) {
        case 0x0010:
            return "H520";
        default:
            return "undefined";
    }
}

} // namespace mavsdk
