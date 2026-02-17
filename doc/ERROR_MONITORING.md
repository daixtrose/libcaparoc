# CAPAROC Error Monitoring and Status Strategy

## Overview

The CAPAROC system provides comprehensive status and error monitoring through MODBUS registers in the 0x6000-0x60FF range. This document explains how to leverage these registers for effective system monitoring and error detection.

## System-Level Monitoring

### Global Status Register (0x6000)

The global status byte provides a high-level overview of system health:

- **Bit 0: Undervoltage** - Input voltage is below acceptable threshold
- **Bit 1: Overvoltage** - Input voltage is above acceptable threshold
- **Bit 2: Cumulative Channel Error** - At least one channel has an error condition
- **Bit 3: Cumulative 80% Warning** - At least one channel is operating above 80% of nominal current
- **Bit 4: System Current Too High** - Total system current exceeds safe operating limits

**Usage:**
```bash
# Check system-level status
build/bin/caparoc_commander --get-system-status

# Or get comprehensive status including all channels
build/bin/caparoc_commander --print-status
```

### System Measurements

Key system measurements for monitoring:

| Register | Address | Description | Units |
|----------|---------|-------------|-------|
| Total System Current | 0x6001 | Sum of all channel currents | Amperes |
| Input Voltage | 0x6002 | Supply voltage to the system | Volts |
| Sum of Nominal Currents | 0x6005 | Total configured capacity | Amperes |
| Internal Temperature | 0x6009 | Device temperature | °C |

**Monitoring Strategy:**
- Monitor temperature to ensure proper cooling (typical safe range: 0-60°C)
- Check input voltage is within acceptable range (typically 11-29V DC)
- Compare total system current vs. sum of nominal currents to assess overall capacity usage
- Alert if system current exceeds 80% of configured capacity

## Channel-Level Monitoring

### Channel Status Registers (0x6010-0x604F)

Each channel has a dedicated status register with the following error bits:

- **Bit 0: 80% Warning** - Channel is operating above 80% of nominal current (pre-warning)
- **Bit 1: Overload** - Channel current exceeds nominal setting
- **Bit 2: Short Circuit** - Short circuit detected on channel
- **Bit 3: Hardware Error** - Internal hardware fault
- **Bit 4: Voltage Error** - Voltage outside acceptable range
- **Bit 5: Module Current Too High** - Module's total current exceeds limits
- **Bit 6: System Current Too High** - System total current exceeds limits

**Register Calculation:**
```
Address = 0x6010 + (module_number - 1) * 4 + (channel_number - 1)
```

**Usage:**
```bash
# Check specific channel status
build/bin/caparoc_commander --get-channel-status 2 1

# Get status for all channels
build/bin/caparoc_commander --print-status
```

### Load Current Measurements (0x6050-0x60CF)

Actual load current per channel (resolution: 100mA):

**Register Calculation:**
```
Address = 0x6050 + (module_number - 1) * 4 + (channel_number - 1)
```

**Usage:**
```bash
# Get actual load current for a channel
build/bin/caparoc_commander --get-load-current 2 1
```

## Error Detection Strategies

### 1. Continuous Health Monitoring

Poll the global status register (0x6000) periodically (e.g., every 5 seconds):

```bash
# Example monitoring script
while true; do
    build/bin/caparoc_commander --get-system-status
    sleep 5
done
```

If any error bits are set, drill down to specific channels to identify the issue.

### 2. Load vs. Capacity Analysis

Compare actual load current against nominal settings:

```bash
build/bin/caparoc_commander --print-status
```

This shows: `Actual A / Nominal A` for each channel plus status flags.

**Warning Thresholds:**
- **Green:** < 80% of nominal current
- **Yellow:** 80-100% of nominal current (80% warning bit set)
- **Red:** > 100% of nominal current (overload bit set)

### 3. Proactive Temperature Monitoring

Monitor internal temperature to prevent thermal issues:

```bash
# Check temperature
build/bin/caparoc_commander --get-system-status | grep Temperature
```

**Recommended Actions:**
- **< 50°C:** Normal operation
- **50-60°C:** Verify adequate cooling
- **> 60°C:** Reduce load or improve ventilation

### 4. Voltage Monitoring

Monitor input voltage for power supply issues:

```bash
# Check input voltage
build/bin/caparoc_commander --get-system-status | grep Voltage
```

**Typical Operating Range:** 11-29V DC (check your specific module specs)

## Error Recovery Procedures

### Channel Errors

When channel errors are detected:

1. **Read channel status** to identify specific error type
2. **Check load current** to verify actual vs. nominal
3. **Inspect physical connections** for shorts or loose wiring
4. **Reset error counters** after resolving the issue

```bash
# Reset error for specific channel
build/bin/caparoc_commander --write-uint16 0x0210 1  # Module 1 Channel 1 error reset
```

### System Errors

For system-level errors (undervoltage, overvoltage):

1. **Verify power supply** output voltage and capacity
2. **Check total system current** vs. power supply rating
3. **Inspect wiring** from power supply to CAPAROC base
4. **Reduce load** if necessary

## Integration with Monitoring Systems

### Prometheus/Grafana Example

Create a script that exports metrics:

```bash
#!/bin/bash
# caparoc_exporter.sh

STATUS=$(build/bin/caparoc_commander --get-system-status)
VOLTAGE=$(echo "$STATUS" | grep "Input Voltage" | awk '{print $3}')
CURRENT=$(echo "$STATUS" | grep "Total System Current" | awk '{print $4}')
TEMP=$(echo "$STATUS" | grep "Internal Temperature" | awk '{print $3}')

echo "caparoc_input_voltage $VOLTAGE"
echo "caparoc_system_current $CURRENT"
echo "caparoc_temperature $TEMP"
```

### Alerting Rules

Set up alerts based on status registers:

1. **Critical:** Short circuit, hardware error, overvoltage, undervoltage
2. **Warning:** 80% current warning, high temperature (>50°C)
3. **Info:** System operating normally

## Best Practices

1. **Poll global status first** - Most efficient way to detect if anything is wrong
2. **Drill down on errors** - Only read individual channel status when global status indicates an issue
3. **Log historical data** - Track temperature and current trends over time
4. **Set up automated alerts** - Don't rely on manual checking
5. **Document nominal settings** - Keep track of configured current limits for each channel
6. **Regular maintenance checks** - Review error counters periodically even if no active errors

## Command Reference

```bash
# Comprehensive status display
build/bin/caparoc_commander --print-status

# System-level status only
build/bin/caparoc_commander --get-system-status

# Specific channel status
build/bin/caparoc_commander --get-channel-status MODULE CHANNEL

# Actual load current
build/bin/caparoc_commander --get-load-current MODULE CHANNEL

# Control channel on/off
build/bin/caparoc_commander --control-channel MODULE CHANNEL on|off

# Device information
build/bin/caparoc_commander --print-device-info
```

## Register Summary

| Range | Purpose | Access |
|-------|---------|--------|
| 0x6000 | Global status byte | Read-only |
| 0x6001-0x6009 | System measurements | Read-only |
| 0x6010-0x604F | Channel status (errors) | Read-only |
| 0x6050-0x60CF | Channel load currents | Read-only |
| 0xC000-0xC002 | Control/locks | Read-write |
| 0xC010-0xC04F | Channel on/off control | Read-write |
| 0xC050-0xC0CF | Nominal current settings | Read-write |

## Troubleshooting Tips

**Problem:** Can't read status registers
- **Solution:** Verify MODBUS connection and device IP address

**Problem:** All channels show errors
- **Solution:** Check input voltage and power supply capacity

**Problem:** Intermittent errors on specific channel
- **Solution:** Check wiring connections and load stability

**Problem:** Temperature rising above 50°C
- **Solution:** Verify ambient temperature, improve airflow, reduce load

**Problem:** 80% warning on multiple channels
- **Solution:** Consider adding modules to increase capacity or reduce loads
