#include "caparoc/caparoc.hpp"
#include "caparoc/modbus_connection.hpp"
#include "caparoc/registers.hpp"
#include <format>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <thread>
#include <chrono>

namespace caparoc {

// ============================================================================
// Generic Register Access Functions
// ============================================================================

std::optional<uint16_t> read_uint16(ModbusConnection& conn, uint16_t address) {
    uint16_t value;
    if (!conn.read_register(address, value)) {
        return std::nullopt;
    }
    return value;
}

std::optional<uint32_t> read_uint32(ModbusConnection& conn, uint16_t address) {
    uint16_t values[2];
    if (!conn.read_registers(address, 2, values)) {
        return std::nullopt;
    }
    // MODBUS uses big endian: values[0] is high word, values[1] is low word
    return (static_cast<uint32_t>(values[0]) << 16) | values[1];
}

std::optional<std::string> read_string32(ModbusConnection& conn, uint16_t address) {
    uint16_t values[16];  // 32 bytes = 16 registers
    if (!conn.read_registers(address, 16, values)) {
        return std::nullopt;
    }
    
    // Convert to string (2 bytes per register, big-endian MODBUS format)
    // Each register contains 2 bytes: high byte first, then low byte
    std::string result;
    result.reserve(32);
    for (int i = 0; i < 16; ++i) {
        result += static_cast<char>((values[i] >> 8) & 0xFF);
        result += static_cast<char>(values[i] & 0xFF);
    }
    
    // Trim null bytes
    size_t end = result.find('\0');
    if (end != std::string::npos) {
        result.resize(end);
    }
    
    return result;
}

bool write_uint16(ModbusConnection& conn, uint16_t address, uint16_t value) {
    return conn.write_register(address, value);
}

bool write_uint32(ModbusConnection& conn, uint16_t address, uint32_t value) {
    uint16_t values[2];
    // MODBUS uses big endian: values[0] is high word, values[1] is low word
    values[0] = (value >> 16) & 0xFFFF;
    values[1] = value & 0xFFFF;
    return conn.write_registers(address, 2, values);
}

// ============================================================================
// Validation Utility Functions
// ============================================================================

static void validate_module_number(ModbusConnection& conn, uint8_t module_number) {
    auto num_modules_opt = get_number_of_connected_modules(conn);
    if (!num_modules_opt) {
        throw std::invalid_argument("Failed to read number of connected modules from device");
    }
    uint16_t num_modules = *num_modules_opt;
    
    if (module_number < 1 || module_number > num_modules) {
        throw std::invalid_argument(std::format(
            "Invalid module number: {}. Expected value between 1 and {} (number of connected modules)",
            module_number, num_modules
        ));
    }
}

static void validate_channel_number(ModbusConnection& conn, uint8_t module_number, uint8_t channel_number) {
    auto num_channels_opt = get_number_of_channels_for_module(conn, module_number);
    if (!num_channels_opt) {
        throw std::invalid_argument(std::format(
            "Failed to read number of channels for module {} from device",
            module_number
        ));
    }
    uint16_t num_channels = *num_channels_opt;
    
    if (channel_number < 1 || channel_number > num_channels) {
        throw std::invalid_argument(std::format(
            "Invalid channel number: {} for module {}. Expected value between 1 and {} (number of channels for this module)",
            channel_number, module_number, num_channels
        ));
    }
}

// ============================================================================
// Control/Reset Functions (Backward Compatibility)
// ============================================================================

bool reset_application_params_power_and_cb(ModbusConnection& conn, uint16_t value) {
    return write_uint16(conn, 0x0010, value);
}

bool global_channel_error_reset_all_cb(ModbusConnection& conn, uint16_t value) {
    return write_uint16(conn, 0x0011, value);
}

bool error_counter_reset_all_cb(ModbusConnection& conn, uint16_t value) {
    return write_uint16(conn, 0x0012, value);
}

bool reset_application_params_quint(ModbusConnection& conn, uint16_t value) {
    return write_uint16(conn, 0x0020, value);
}

// ============================================================================
// Product Information Functions
// ============================================================================

std::optional<std::string> get_product_name_power_module(ModbusConnection& conn) {
    return read_string32(conn, 0x1000);
}

std::optional<std::string> get_product_name_module(ModbusConnection& conn, uint8_t module_number) {
    validate_module_number(conn, module_number);
    uint16_t address = 0x1010 + (module_number - 1) * 0x10;
    return read_string32(conn, address);
}

std::optional<std::string> get_product_name_quint(ModbusConnection& conn) {
    return read_string32(conn, 0x1110);
}

// ============================================================================
// Utility Functions
// ============================================================================

std::string get_register_info(uint16_t address) {
    for (size_t i = 0; i < register_table_size; ++i) {
        if (register_table[i].address == address) {
            const auto& reg = register_table[i];
            
            std::string type_str;
            switch (reg.type) {
                case RegisterType::UINT16: type_str = "UINT16"; break;
                case RegisterType::INT16: type_str = "INT16"; break;
                case RegisterType::UINT32: type_str = "UINT32"; break;
                case RegisterType::INT32: type_str = "INT32"; break;
                case RegisterType::FLOAT: type_str = "FLOAT"; break;
                case RegisterType::STRING32: type_str = "STRING32"; break;
            }
            
            std::string access_str;
            switch (reg.access) {
                case RegisterAccess::READ_ONLY: access_str = "RO"; break;
                case RegisterAccess::WRITE_ONLY: access_str = "WO"; break;
                case RegisterAccess::READ_WRITE: access_str = "RW"; break;
            }
            
            return std::format(
                "Address: 0x{:04X} ({} dec)\n"
                "Registers: {}\n"
                "Type: {}\n"
                "Access: {}\n"
                "Name: {}\n"
                "Description: {}",
                reg.address, reg.address,
                reg.num_registers,
                type_str,
                access_str,
                reg.name,
                reg.description
            );
        }
    }
    
    return std::format("Register at address 0x{:04X} not found", address);
}

std::string list_all_registers(const std::string& filter) {
    std::ostringstream oss;
    oss << "CAPAROC MODBUS Register Map\n";
    oss << "===========================\n";
    oss << std::format("Total registers: {}\n", register_table_size);
    
    // Convert filter to lowercase for case-insensitive search
    std::string filter_lower = filter;
    std::transform(filter_lower.begin(), filter_lower.end(), filter_lower.begin(),
                   [](unsigned char c){ return std::tolower(c); });
    
    int count = 0;
    for (size_t i = 0; i < register_table_size; ++i) {
        const auto& reg = register_table[i];
        
        // Apply filter if provided
        if (!filter.empty()) {
            std::string name_lower = reg.name;
            std::transform(name_lower.begin(), name_lower.end(), name_lower.begin(),
                          [](unsigned char c){ return std::tolower(c); });
            std::string desc_lower = reg.description;
            std::transform(desc_lower.begin(), desc_lower.end(), desc_lower.begin(),
                          [](unsigned char c){ return std::tolower(c); });
            
            if (name_lower.find(filter_lower) == std::string::npos &&
                desc_lower.find(filter_lower) == std::string::npos) {
                continue;
            }
        }
        
        std::string type_str;
        switch (reg.type) {
            case RegisterType::UINT16: type_str = "UINT16"; break;
            case RegisterType::INT16: type_str = "INT16"; break;
            case RegisterType::UINT32: type_str = "UINT32"; break;
            case RegisterType::INT32: type_str = "INT32"; break;
            case RegisterType::FLOAT: type_str = "FLOAT"; break;
            case RegisterType::STRING32: type_str = "STRING32"; break;
        }
        
        std::string access_str;
        switch (reg.access) {
            case RegisterAccess::READ_ONLY: access_str = "RO"; break;
            case RegisterAccess::WRITE_ONLY: access_str = "WO"; break;
            case RegisterAccess::READ_WRITE: access_str = "RW"; break;
        }
        
        if (count == 0) {
            oss << "\n";
        }
        
        oss << std::format(
            "[0x{:04X}] {} | {} | {} regs | {}\n",
            reg.address,
            access_str,
            type_str,
            reg.num_registers,
            reg.name
        );
        
        count++;
        
        // Limit output if no filter
        if (filter.empty() && count >= 800) {
            oss << std::format("\n... and {} more registers (use filter to narrow down)\n", 
                             register_table_size - count);
            break;
        }
    }
    
    if (!filter.empty()) {
        oss << std::format("\nMatching registers: {}\n", count);
    }
    
    return oss.str();
}

std::vector<RegisterInfo> find_registers(const std::string& pattern) {
    std::vector<RegisterInfo> result;
    
    std::string pattern_lower = pattern;
    std::transform(pattern_lower.begin(), pattern_lower.end(), pattern_lower.begin(),
                   [](unsigned char c){ return std::tolower(c); });
    
    for (size_t i = 0; i < register_table_size; ++i) {
        const auto& reg = register_table[i];
        
        std::string name_lower = reg.name;
        std::transform(name_lower.begin(), name_lower.end(), name_lower.begin(),
                      [](unsigned char c){ return std::tolower(c); });
        
        if (name_lower.find(pattern_lower) != std::string::npos) {
            result.push_back(reg);
        }
    }
    
    return result;
}

std::optional<uint16_t> get_number_of_connected_modules(ModbusConnection& conn) {
    return read_uint16(conn, 0x2000);
}

std::optional<uint16_t> get_number_of_channels_for_module(ModbusConnection& conn, uint8_t module_number) {
    if (module_number < 1 || module_number > 16) {
        return std::nullopt;
    }
    // Register address: 0x2001 + (module_number - 1)
    uint16_t address = 0x2001 + (module_number - 1);
    return read_uint16(conn, address);
}

bool set_nominal_current(ModbusConnection& conn, uint8_t module_number, uint8_t channel_number, uint16_t nominal_current) {
    validate_module_number(conn, module_number);
    validate_channel_number(conn, module_number, channel_number);

    // Check if module is CAPAROC E2 12-24DC/2-10A (manual rotary dial only)
    auto product_name_opt = get_product_name_module(conn, module_number);
    if (product_name_opt && product_name_opt->find("CAPAROC E2 12-24DC/2-10A") != std::string::npos) {
        throw std::invalid_argument(
            "Module is CAPAROC E2 12-24DC/2-10A. Nominal current must be set physically via the rotary dials."
        );
    }
    
    // Calculate register address
    // Base address: 0xC050
    // Formula: 0xC050 + (module_number - 1) * 4 + (channel_number - 1)
    uint16_t address = 0xC050 + (module_number - 1) * 4 + (channel_number - 1);

    // Read device's max CAPAROC bus cycle (0x6006) to determine appropriate spacing
    uint16_t max_bus_cycle_ms = 100;  // Default fallback
    if (auto max_cycle_opt = read_uint16(conn, 0x6006)) {
        max_bus_cycle_ms = *max_cycle_opt;
    }
    // Wait for at least (bus_cycle + 50ms) to allow device to settle before critical lock/unlock sequence
    const auto wait_time = std::chrono::milliseconds(static_cast<int>(max_bus_cycle_ms) + 50);
    std::this_thread::sleep_for(wait_time);

    // Unlock nominal current parametrization (channel then global)
    const uint16_t global_lock_address = 0xC001;
    const uint16_t channel_lock_address = 0xC090 + (module_number - 1) * 4 + (channel_number - 1);
    
    constexpr auto kDelay = std::chrono::milliseconds(50);
    constexpr int kRetries = 5;

    if (!write_uint16(conn, channel_lock_address, 0)) {
        return false;
    }
    std::this_thread::sleep_for(kDelay);
    if (!write_uint16(conn, global_lock_address, 0)) {
        return false;
    }
    std::this_thread::sleep_for(kDelay);

    // Write with retry + verification to mitigate timing issues
    bool verified = false;
    for (int attempt = 0; attempt < kRetries; ++attempt) {
        if (!write_uint16(conn, address, nominal_current)) {
            std::this_thread::sleep_for(kDelay);
            continue;
        }
        std::this_thread::sleep_for(kDelay);
        auto verify = read_uint16(conn, address);
        if (verify && *verify == nominal_current) {
            verified = true;
            break;
        }
        std::this_thread::sleep_for(kDelay);
    }

    if (!verified) {
        // Write verification failed - still try to re-lock to clean up
        write_uint16(conn, global_lock_address, 1);
        write_uint16(conn, channel_lock_address, 1);
        return false;
    }

    // Re-lock nominal current parametrization
    if (!write_uint16(conn, global_lock_address, 1)) {
        return false;
    }
    std::this_thread::sleep_for(kDelay);
    if (!write_uint16(conn, channel_lock_address, 1)) {
        return false;
    }

    // Allow device to settle after write operation before next command
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    return true;
}

std::optional<uint16_t> get_nominal_current(ModbusConnection& conn, uint8_t module_number, uint8_t channel_number) {
    validate_module_number(conn, module_number);
    validate_channel_number(conn, module_number, channel_number);
    
    // Calculate register address
    // Base address: 0xC050
    // Formula: 0xC050 + (module_number - 1) * 4 + (channel_number - 1)
    uint16_t address = 0xC050 + (module_number - 1) * 4 + (channel_number - 1);
    
    return read_uint16(conn, address);
}

std::string print_device_info(ModbusConnection& conn) {
    std::ostringstream oss;
    
    // Get power module product name
    auto power_module_name = get_product_name_power_module(conn);
    if (power_module_name) {
        oss << std::format("Power Module: {}\n", *power_module_name);
    }
    
    // System Status
    oss << "\n=== System Status ===\n";
    
    auto global_status = get_global_status(conn);
    if (global_status) {
        oss << "Global Status: ";
        bool has_error = false;
        if (global_status->undervoltage) { oss << "UNDERVOLTAGE "; has_error = true; }
        if (global_status->overvoltage) { oss << "OVERVOLTAGE "; has_error = true; }
        if (global_status->cumulative_channel_error) { oss << "CHANNEL_ERROR "; has_error = true; }
        if (global_status->cumulative_80_warning) { oss << "80%_WARNING "; has_error = true; }
        if (global_status->system_current_too_high) { oss << "SYSTEM_CURRENT_HIGH "; has_error = true; }
        if (!has_error) { oss << "OK"; }
        oss << "\n";
    }
    
    auto total_current = get_total_system_current(conn);
    if (total_current) {
        oss << std::format("Total System Current: {} A\n", *total_current);
    }
    
    auto input_voltage = get_input_voltage(conn);
    if (input_voltage) {
        oss << std::format("Input Voltage: {:.2f} V\n", *input_voltage / 100.0);
    }
    
    auto sum_nominal = get_sum_of_nominal_currents(conn);
    if (sum_nominal) {
        oss << std::format("Sum of Nominal Currents: {} A\n", *sum_nominal);
    }
    
    auto temperature = get_internal_temperature(conn);
    if (temperature) {
        oss << std::format("Internal Temperature: {} °C\n", *temperature);
    }
    
    // Get number of connected modules
    auto num_modules_opt = get_number_of_connected_modules(conn);
    if (!num_modules_opt) {
        oss << "\nError: Failed to read number of connected modules\n";
        return oss.str();
    }
    
    uint16_t num_modules = *num_modules_opt;
    oss << std::format("\n=== Connected Modules: {} ===\n", num_modules);
    
    // Get information for each connected module
    for (uint16_t module = 1; module <= num_modules; ++module) {
        // Get product name for this module
        auto product_name = get_product_name_module(conn, static_cast<uint8_t>(module));
        if (!product_name) {
            oss << std::format("Module {}: Error reading product name\n", module);
            continue;
        }
        
        // Get number of channels for this module
        auto num_channels_opt = get_number_of_channels_for_module(conn, static_cast<uint8_t>(module));
        if (!num_channels_opt) {
            oss << std::format("Module {}: {} (Error reading channel count)\n", module, *product_name);
            continue;
        }
        
        uint16_t num_channels = *num_channels_opt;
        oss << std::format("Module {}: {} ({} channels)\n", module, *product_name, num_channels);
        
        // Get current and status for each channel
        for (uint16_t channel = 1; channel <= num_channels; ++channel) {
            oss << std::format("  Channel {}: ", channel);
            
            // Get nominal current
            auto nominal_current = read_uint16(conn, 0xC050 + (module - 1) * 4 + (channel - 1));
            
            // Get actual load current
            auto load = get_load_current(conn, static_cast<uint8_t>(module), static_cast<uint8_t>(channel));
            
            if (nominal_current && load) {
                double load_amps = *load / 1000.0;  // Convert mA to A
                oss << std::format("{:.1f} A / {} A", load_amps, *nominal_current);
            } else if (nominal_current) {
                oss << std::format("? A / {} A", *nominal_current);
            } else {
                oss << "Error reading currents";
            }
            
            // Get status
            auto status = get_channel_status(conn, static_cast<uint8_t>(module), static_cast<uint8_t>(channel));
            if (status) {
                oss << " [";
                bool has_error = false;
                if (status->short_circuit) { oss << "SHORT_CIRCUIT "; has_error = true; }
                if (status->overload) { oss << "OVERLOAD "; has_error = true; }
                if (status->hardware_error) { oss << "HW_ERROR "; has_error = true; }
                if (status->voltage_error) { oss << "VOLTAGE_ERROR "; has_error = true; }
                if (status->warning_80_percent) { oss << "80%_WARNING "; has_error = true; }
                if (status->module_current_too_high) { oss << "MODULE_CURRENT_HIGH "; has_error = true; }
                if (status->system_current_too_high) { oss << "SYSTEM_CURRENT_HIGH "; has_error = true; }
                if (!has_error) { oss << "OK"; }
                oss << "]";
            }
            
            oss << "\n";
        }
    }
    
    // Get QUINT power supply info
    auto quint_name = get_product_name_quint(conn);
    if (quint_name) {
        oss << std::format("\nQUINT Power Supply: {}\n", *quint_name);
    }
    
    return oss.str();
}

// ============================================================================
// System Status and Monitoring Functions
// ============================================================================

std::optional<GlobalStatus> get_global_status(ModbusConnection& conn) {
    auto val = read_uint16(conn, 0x6000);
    if (!val) {
        return std::nullopt;
    }
    
    GlobalStatus status;
    status.undervoltage = (*val & 0x01) != 0;
    status.overvoltage = (*val & 0x02) != 0;
    status.cumulative_channel_error = (*val & 0x04) != 0;
    status.cumulative_80_warning = (*val & 0x08) != 0;
    status.system_current_too_high = (*val & 0x10) != 0;
    
    return status;
}

std::optional<uint16_t> get_total_system_current(ModbusConnection& conn) {
    return read_uint16(conn, 0x6001);
}

std::optional<uint16_t> get_input_voltage(ModbusConnection& conn) {
    return read_uint16(conn, 0x6002);
}

std::optional<uint16_t> get_sum_of_nominal_currents(ModbusConnection& conn) {
    return read_uint16(conn, 0x6005);
}

std::optional<int16_t> get_internal_temperature(ModbusConnection& conn) {
    auto val = read_uint16(conn, 0x6009);
    if (!val) {
        return std::nullopt;
    }
    return static_cast<int16_t>(*val);
}

std::optional<ChannelStatus> get_channel_status(ModbusConnection& conn, uint8_t module_number, uint8_t channel_number) {
    validate_module_number(conn, module_number);
    validate_channel_number(conn, module_number, channel_number);
    
    uint16_t address = 0x6010 + (module_number - 1) * 4 + (channel_number - 1);
    auto val = read_uint16(conn, address);
    if (!val) {
        return std::nullopt;
    }
    
    ChannelStatus status;
    status.warning_80_percent = (*val & 0x01) != 0;
    status.overload = (*val & 0x02) != 0;
    status.short_circuit = (*val & 0x04) != 0;
    status.hardware_error = (*val & 0x08) != 0;
    status.voltage_error = (*val & 0x10) != 0;
    status.module_current_too_high = (*val & 0x20) != 0;
    status.system_current_too_high = (*val & 0x40) != 0;
    
    return status;
}

std::optional<uint16_t> get_load_current(ModbusConnection& conn, uint8_t module_number, uint8_t channel_number) {
    validate_module_number(conn, module_number);
    validate_channel_number(conn, module_number, channel_number);
    
    uint16_t address = 0x6050 + (module_number - 1) * 4 + (channel_number - 1);
    return read_uint16(conn, address);
}

bool control_channel(ModbusConnection& conn, uint8_t module_number, uint8_t channel_number, bool on) {
    validate_module_number(conn, module_number);
    validate_channel_number(conn, module_number, channel_number);
    
    uint16_t address = 0xC010 + (module_number - 1) * 4 + (channel_number - 1);
    return write_uint16(conn, address, on ? 1 : 0);
}

} // namespace caparoc
