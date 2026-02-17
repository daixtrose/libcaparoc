#pragma once

#include <string>
#include <optional>
#include <cstdint>
#include <vector>
#include "caparoc/registers.hpp"

namespace caparoc {

// Forward declaration
class ModbusConnection;

// ============================================================================
// Generic Register Access Functions
// ============================================================================

/**
 * @brief Read a UINT16 register
 * 
 * @param conn MODBUS connection
 * @param address Register address
 * @return std::optional<uint16_t> Register value if successful
 */
std::optional<uint16_t> read_uint16(ModbusConnection& conn, uint16_t address);

/**
 * @brief Read a UINT32 register (2 consecutive registers)
 * 
 * @param conn MODBUS connection
 * @param address Starting register address
 * @return std::optional<uint32_t> Register value if successful
 */
std::optional<uint32_t> read_uint32(ModbusConnection& conn, uint16_t address);

/**
 * @brief Read a String32 (32-character string from 16 consecutive registers)
 * 
 * @param conn MODBUS connection
 * @param address Starting register address
 * @return std::optional<std::string> String if successful
 */
std::optional<std::string> read_string32(ModbusConnection& conn, uint16_t address);

/**
 * @brief Write a UINT16 register
 * 
 * @param conn MODBUS connection
 * @param address Register address
 * @param value Value to write
 * @return true if successful
 * @return false if failed
 */
bool write_uint16(ModbusConnection& conn, uint16_t address, uint16_t value);

/**
 * @brief Write a UINT32 register (2 consecutive registers)
 * 
 * @param conn MODBUS connection
 * @param address Starting register address
 * @param value Value to write
 * @return true if successful
 * @return false if failed
 */
bool write_uint32(ModbusConnection& conn, uint16_t address, uint32_t value);

// ============================================================================
// Control/Reset Functions (Backward Compatibility)
// ============================================================================

/**
 * @brief Reset application parameters to default settings (Power module and all Circuit Breaker modules)
 * 
 * @param conn MODBUS connection
 * @param value Any value > 0 triggers the reset
 * @return true if write successful
 * @return false if write failed
 */
bool reset_application_params_power_and_cb(ModbusConnection& conn, uint16_t value = 1);

/**
 * @brief Global channel error reset (All Circuit Breaker modules)
 * 
 * @param conn MODBUS connection
 * @param value Any value > 0 triggers the reset
 * @return true if write successful
 * @return false if write failed
 */
bool global_channel_error_reset_all_cb(ModbusConnection& conn, uint16_t value = 1);

/**
 * @brief Error counter reset (All Circuit Breaker modules)
 * 
 * @param conn MODBUS connection
 * @param value Any value > 0 triggers the reset
 * @return true if write successful
 * @return false if write failed
 */
bool error_counter_reset_all_cb(ModbusConnection& conn, uint16_t value = 1);

/**
 * @brief Reset application parameters to default settings (QUINT Power Supply)
 * 
 * @param conn MODBUS connection
 * @param value Any value > 0 triggers the reset
 * @return true if write successful
 * @return false if write failed
 */
bool reset_application_params_quint(ModbusConnection& conn, uint16_t value = 1);

// ============================================================================
// Product Information Functions
// ============================================================================

/**
 * @brief Get product name for Power Module
 * 
 * @param conn MODBUS connection
 * @return std::optional<std::string> Product name if successful
 */
std::optional<std::string> get_product_name_power_module(ModbusConnection& conn);

/**
 * @brief Get product name for a specific module
 * 
 * @param conn MODBUS connection
 * @param module_number Module number (1-16)
 * @return std::optional<std::string> Product name if successful
 */
std::optional<std::string> get_product_name_module(ModbusConnection& conn, uint8_t module_number);

/**
 * @brief Get product name for QUINT Power Supply
 * 
 * @param conn MODBUS connection
 * @return std::optional<std::string> Product name if successful
 */
std::optional<std::string> get_product_name_quint(ModbusConnection& conn);

// ============================================================================
// Utility Functions
// ============================================================================

/**
 * @brief Get register information by address
 * 
 * @param address Register address
 * @return std::string Register information as formatted string
 */
std::string get_register_info(uint16_t address);

/**
 * @brief List all available registers
 * 
 * @param filter Optional filter string (checks register name/description)
 * @return std::string Formatted list of registers
 */
std::string list_all_registers(const std::string& filter = "");

/**
 * @brief Find registers by name pattern
 * 
 * @param pattern Search pattern (case-insensitive substring match)
 * @return std::vector<RegisterInfo> List of matching registers with details
 */
std::vector<RegisterInfo> find_registers(const std::string& pattern);

/**
 * @brief Get the number of currently connected modules
 * 
 * @param conn MODBUS connection
 * @return std::optional<uint16_t> Number of connected modules if successful
 */
std::optional<uint16_t> get_number_of_connected_modules(ModbusConnection& conn);

/**
 * @brief Get the number of channels for a specific module
 * 
 * @param conn MODBUS connection
 * @param module_number Module number (1-16)
 * @return std::optional<uint16_t> Number of channels if successful
 */
std::optional<uint16_t> get_number_of_channels_for_module(ModbusConnection& conn, uint8_t module_number);

/**
 * @brief Set nominal current for a module channel
 * 
 * Validates that the module and channel numbers are within valid ranges
 * based on the currently connected modules and their channel counts.
 * 
 * @param conn MODBUS connection
 * @param module_number Module number (1-16)
 * @param channel_number Channel number (1-4)
 * @param nominal_current Nominal current in Amperes (int value)
 * @return true if write successful
 * @return false if write failed
 * @throws std::invalid_argument if module_number or channel_number are out of valid range
 */
bool set_nominal_current(ModbusConnection& conn, uint8_t module_number, uint8_t channel_number, uint16_t nominal_current);

/**
 * @brief Get nominal current for a module channel
 * 
 * @param conn MODBUS connection
 * @param module_number Module number (1-16)
 * @param channel_number Channel number (1-4)
 * @return std::optional<uint16_t> Nominal current in Amperes if successful
 */
std::optional<uint16_t> get_nominal_current(ModbusConnection& conn, uint8_t module_number, uint8_t channel_number);

/**
 * @brief Print device information (connected modules, product names, and channel counts)
 * 
 * Queries the device to get the number of connected modules, product names, and
 * the number of channels for each module. Prints formatted information to output.
 * 
 * @param conn MODBUS connection
 * @return std::string Formatted device information
 */
std::string print_device_info(ModbusConnection& conn);

// ============================================================================
// System Status and Monitoring Functions
// ============================================================================

/**
 * @brief Status bits for global status byte
 */
struct GlobalStatus {
    bool undervoltage;
    bool overvoltage;
    bool cumulative_channel_error;
    bool cumulative_80_warning;
    bool system_current_too_high;
};

/**
 * @brief Status bits for channel status
 */
struct ChannelStatus {
    bool warning_80_percent;
    bool overload;
    bool short_circuit;
    bool hardware_error;
    bool voltage_error;
    bool module_current_too_high;
    bool system_current_too_high;
};

/**
 * @brief Get global status byte (0x6000)
 * 
 * @param conn MODBUS connection
 * @return std::optional<GlobalStatus> Status if successful
 */
std::optional<GlobalStatus> get_global_status(ModbusConnection& conn);

/**
 * @brief Get total system current (0x6001)
 * 
 * @param conn MODBUS connection
 * @return std::optional<uint16_t> Current in Amperes if successful
 */
std::optional<uint16_t> get_total_system_current(ModbusConnection& conn);

/**
 * @brief Get input voltage (0x6002)
 * 
 * @param conn MODBUS connection
 * @return std::optional<uint16_t> Voltage in Volts if successful
 */
std::optional<uint16_t> get_input_voltage(ModbusConnection& conn);

/**
 * @brief Get sum of nominal currents (0x6005)
 * 
 * @param conn MODBUS connection
 * @return std::optional<uint16_t> Current in Amperes if successful
 */
std::optional<uint16_t> get_sum_of_nominal_currents(ModbusConnection& conn);

/**
 * @brief Get internal temperature (0x6009)
 * 
 * @param conn MODBUS connection
 * @return std::optional<int16_t> Temperature in °C if successful
 */
std::optional<int16_t> get_internal_temperature(ModbusConnection& conn);

/**
 * @brief Get channel status (0x6010-0x604F)
 * 
 * @param conn MODBUS connection
 * @param module_number Module number (1-16)
 * @param channel_number Channel number (1-4)
 * @return std::optional<ChannelStatus> Status if successful
 */
std::optional<ChannelStatus> get_channel_status(ModbusConnection& conn, uint8_t module_number, uint8_t channel_number);

/**
 * @brief Get actual load current for a channel (0x6050-0x60CF)
 * 
 * @param conn MODBUS connection
 * @param module_number Module number (1-16)
 * @param channel_number Channel number (1-4)
 * @return std::optional<uint16_t> Current in milliamperes (resolution 100mA) if successful
 */
std::optional<uint16_t> get_load_current(ModbusConnection& conn, uint8_t module_number, uint8_t channel_number);

/**
 * @brief Control channel on/off (0xC010-0xC04F)
 * 
 * @param conn MODBUS connection
 * @param module_number Module number (1-16)
 * @param channel_number Channel number (1-4)
 * @param on true to turn on, false to turn off
 * @return true if write successful
 * @return false if write failed
 */
bool control_channel(ModbusConnection& conn, uint8_t module_number, uint8_t channel_number, bool on);

} // namespace caparoc
