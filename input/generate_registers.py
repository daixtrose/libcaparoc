#!/usr/bin/env python3
"""
Generate C++ code for CAPAROC MODBUS registers from HTML specification
"""
import re
import html

def sanitize_name(name):
    """Convert register name to valid C++ identifier"""
    # Remove leading/trailing whitespace
    name = name.strip()
    # Convert to lowercase
    name = name.lower()
    # Replace spaces and special chars with underscores
    name = re.sub(r'[^a-z0-9]+', '_', name)
    # Remove leading/trailing underscores
    name = name.strip('_')
    # Limit length
    if len(name) > 60:
        name = name[:60].rstrip('_')
    return name

def parse_registers(html_file):
    """Parse HTML file and extract register information"""
    with open(html_file, 'r') as f:
        content = f.read()
    
    rows = re.findall(r'<tr[^>]*>(.*?)</tr>', content, re.DOTALL)
    
    registers = []
    for row in rows:
        cells = re.findall(r'<td[^>]*>(.*?)</td>', row, re.DOTALL)
        
        if len(cells) == 8:
            cells = [html.unescape(re.sub(r'<[^>]+>', '', cell).strip()) for cell in cells]
            
            if cells[0] == 'Dec Adress' or cells[0] == '[...]' or not cells[0]:
                continue
            
            try:
                dec_addr = int(cells[0])
                registers.append({
                    'dec': dec_addr,
                    'hex': cells[1],
                    'num_regs': int(cells[2]) if cells[2].isdigit() else 1,
                    'func_codes': cells[3],
                    'type': cells[4],
                    'access': cells[5],
                    'name': cells[6],
                    'description': cells[7]
                })
            except (ValueError, IndexError):
                pass
    
    return registers

def generate_register_definitions(registers):
    """Generate register constant definitions"""
    lines = []
    lines.append("// Auto-generated register definitions from CAPAROC specification")
    lines.append("// Total registers: {}".format(len(registers)))
    lines.append("")
    
    # Track used names to avoid duplicates
    used_names = set()
    
    # Group by address ranges
    current_group = None
    for reg in registers:
        addr = reg['dec']
        
        # Determine group
        if addr < 256:
            group = "Control/Reset"
        elif addr < 4096:
            group = "Product Information"
        elif addr < 49152:
            group = "Status/Measurements"
        elif addr < 53248:
            group = "Configuration"
        else:
            group = "Configuration (Extended)"
        
        if group != current_group:
            lines.append("")
            lines.append(f"// {group}")
            current_group = group
        
        const_name = sanitize_name(reg['name']).upper()
        
        # Make name unique if it's already used
        if const_name in used_names or not const_name:
            const_name = f"REG_{reg['hex'][2:].upper()}"
        
        used_names.add(const_name)
        
        lines.append(f"constexpr uint16_t {const_name} = {reg['hex']}; // {reg['access']}, {reg['type']}")
    
    return "\n".join(lines)

def generate_register_table(registers):
    """Generate register info table"""
    lines = []
    lines.append("// Register information table")
    lines.append("constexpr RegisterInfo register_table[] = {")
    
    for reg in registers:
        name_escaped = reg['name'].replace('"', '\\"')
        desc_escaped = reg['description'].replace('"', '\\"')
        
        type_enum = {
            'UINT16': 'RegisterType::UINT16',
            'UINT32': 'RegisterType::UINT32',
            'INT16': 'RegisterType::INT16',
            'String32': 'RegisterType::STRING32'
        }.get(reg['type'], 'RegisterType::UINT16')
        
        access_enum = {
            'RO': 'RegisterAccess::READ_ONLY',
            'WO': 'RegisterAccess::WRITE_ONLY',
            'RW': 'RegisterAccess::READ_WRITE'
        }.get(reg['access'], 'RegisterAccess::READ_ONLY')
        
        lines.append(f'    {{{reg["hex"]}, {reg["num_regs"]}, {type_enum}, {access_enum},')
        lines.append(f'     "{name_escaped}",')
        lines.append(f'     "{desc_escaped}"}},')
    
    lines.append("};")
    lines.append(f"constexpr size_t register_table_size = {len(registers)};")
    
    return "\n".join(lines)

if __name__ == '__main__':
    registers = parse_registers('input/registers.html')
    
    print(f"Parsed {len(registers)} registers")
    print(f"  Read-Only: {sum(1 for r in registers if r['access'] == 'RO')}")
    print(f"  Write-Only: {sum(1 for r in registers if r['access'] == 'WO')}")
    print(f"  Read-Write: {sum(1 for r in registers if r['access'] == 'RW')}")
    
    # Generate header file content
    with open('include/caparoc/registers_generated.hpp', 'w') as f:
        f.write("""#pragma once

#include <cstdint>

namespace caparoc {

// Register access types
enum class RegisterAccess {
    READ_ONLY,   // RO
    WRITE_ONLY,  // WO
    READ_WRITE   // RW
};

// Register data types
enum class RegisterType {
    UINT16,
    INT16,
    UINT32,
    INT32,
    FLOAT,
    STRING32  // 32-character string (16 registers)
};

// Register information structure
struct RegisterInfo {
    uint16_t address;
    uint16_t num_registers;
    RegisterType type;
    RegisterAccess access;
    const char* name;
    const char* description;
};

namespace registers {

""")
        f.write(generate_register_definitions(registers))
        f.write("\n\n} // namespace registers\n\n")
        f.write(generate_register_table(registers))
        f.write("\n\n} // namespace caparoc\n")
    
    print("Generated include/caparoc/registers_generated.hpp")
