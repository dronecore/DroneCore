#include "dronecore.h"

#include "dronecore_impl.h"
#include "global_include.h"

namespace dronecore {

DroneCore::DroneCore() :
    _impl(nullptr)
{
    _impl = new DroneCoreImpl();
}

DroneCore::~DroneCore()
{
    delete _impl;
    _impl = nullptr;
}

ConnectionResult DroneCore::add_any_connection(const std::string &connection_url)
{
    return _impl->add_any_connection(connection_url);
}

ConnectionResult DroneCore::add_link_connection(const std::string &protocol,
                                                const std::string &ip,
                                                const int port)
{
    return _impl->add_link_connection(protocol, ip, port);
}

ConnectionResult DroneCore::add_udp_connection(const int local_port_number)
{
    return _impl->add_udp_connection(local_port_number);
}

ConnectionResult DroneCore::add_tcp_connection(const std::string &remote_ip,
                                               const int remote_port)
{
    return _impl->add_tcp_connection(remote_ip, remote_port);
}

ConnectionResult DroneCore::add_serial_connection(const std::string &dev_path,
                                                  const int baudrate)
{
    return _impl->add_serial_connection(dev_path, baudrate);
}

const std::vector<uint64_t> &DroneCore::device_uuids() const
{
    return _impl->get_device_uuids();
}

Device &DroneCore::device() const
{
    return _impl->get_device();
}

Device &DroneCore::device(const uint64_t uuid) const
{
    return _impl->get_device(uuid);
}

bool DroneCore::is_connected() const
{
    return _impl->is_connected();
}

bool DroneCore::is_connected(const uint64_t uuid) const
{
    return _impl->is_connected(uuid);
}

void DroneCore::register_on_discover(const event_callback_t callback)
{
    _impl->register_on_discover(callback);
}

void DroneCore::register_on_timeout(const event_callback_t callback)
{
    _impl->register_on_timeout(callback);
}

} // namespace dronecore
