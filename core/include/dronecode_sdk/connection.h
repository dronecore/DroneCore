namespace dronecode_sdk {

class Connection {
public:
    /** @brief Default UDP bind IP (accepts any incoming connections). */
    static constexpr auto DEFAULT_UDP_BIND_IP = "0.0.0.0";
    /** @brief Default UDP bind port (same port as used by MAVROS). */
    static constexpr int DEFAULT_UDP_PORT = 14540;
    /** @brief Default TCP remote IP (localhost). */
    static constexpr auto DEFAULT_TCP_REMOTE_IP = "127.0.0.1";
    /** @brief Default TCP remote port. */
    static constexpr int DEFAULT_TCP_REMOTE_PORT = 5760;
    /** @brief Default serial baudrate. */
    static constexpr int DEFAULT_SERIAL_BAUDRATE = 57600;

    /**
     * @brief Constructor.
     */
    DronecodeSDK();

    /**
     * @brief Destructor.
     *
     * Disconnects all connected vehicles and releases all resources.
     */
    ~DronecodeSDK();

    /**
     * @brief Adds Connection via URL
     *
     * Supports connection: Serial, TCP or UDP.
     * Connection URL format should be:
     * - UDP - udp://[Bind_host][:Bind_port]
     * - TCP - tcp://[Remote_host][:Remote_port]
     * - Serial - serial://Dev_Node[:Baudrate]
     *
     * @param connection_url connection URL string.
     * @return The result of adding the connection.
     */
    ConnectionResult add_any_connection(const std::string &connection_url);

    /**
     * @brief Adds a UDP connection to the specified port number.
     *
     * Any incoming connections are accepted (0.0.0.0).
     *
     * @param local_port The local UDP port to listen to (defaults to 14540, the same as MAVROS).
     * @return The result of adding the connection.
     */
    ConnectionResult add_udp_connection(int local_port = DEFAULT_UDP_PORT);

    /**
     * @brief Adds a UDP connection to the specified port number and local interface.
     *
     * To accept only local connections of the machine, use 127.0.0.1.
     * For any incoming connections, use 0.0.0.0.
     *
     * @param local_ip The local UDP IP address to listen to.
     * @param local_port The local UDP port to listen to (defaults to 14540, the same as MAVROS).
     * @return The result of adding the connection.
     */
    ConnectionResult add_udp_connection(const std::string &local_ip,
                                        int local_port = DEFAULT_UDP_PORT);

    /**
     * @brief Adds a TCP connection with a specific port number on localhost.
     *
     * @param remote_port The TCP port to connect to (defaults to 5760).
     * @return The result of adding the connection.
     */
    ConnectionResult add_tcp_connection(int remote_port = DEFAULT_TCP_REMOTE_PORT);

    /**
     * @brief Adds a TCP connection with a specific IP address and port number.
     *
     * @param remote_ip Remote IP address to connect to.
     * @param remote_port The TCP port to connect to (defaults to 5760).
     * @return The result of adding the connection.
     */
    ConnectionResult add_tcp_connection(const std::string &remote_ip,
                                        int remote_port = DEFAULT_TCP_REMOTE_PORT);

    /**
     * @brief Adds a serial connection with a specific port (COM or UART dev node) and baudrate as
     * specified.
     *
     *
     * @param dev_path COM or UART dev node name/path (e.g. "/dev/ttyS0", or "COM3" on Windows).
     * @param baudrate Baudrate of the serial port (defaults to 57600).
     * @return The result of adding the connection.
     */
    ConnectionResult add_serial_connection(const std::string &dev_path,
                                           int baudrate = DEFAULT_SERIAL_BAUDRATE);

  /**
     * @brief Returns `true` if exactly one system is currently connected.
     *
     * Connected means we are receiving heartbeats from this system.
     * It means the same as "discovered" and "not timed out".
     *
     * If multiple systems have connected, this will return `false`.
     *
     * @return `true` if exactly one system is connected.
     */
    bool is_connected() const;

    /**
     * @brief Returns `true` if a system is currently connected.
     *
     * Connected means we are receiving heartbeats from this system.
     * It means the same as "discovered" and "not timed out".
     *
     * @param uuid UUID of system to check.
     * @return `true` if system is connected.
     */
    bool is_connected(uint64_t uuid) const;


private:
    /* @private. */
    std::unique_ptr<ConnectionImpl> _impl;

    // Non-copyable
    Connection(const Connection &) = delete;
    const Connection &operator=(const Connection &) = delete;
  
};

} // namespace dronecode_sdk