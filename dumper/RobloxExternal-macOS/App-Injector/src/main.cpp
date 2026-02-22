#include <iostream>
#include <vector>
#include <fstream>
#include <filesystem>
#include <cstring>
#include <sstream>       
#include <mach-o/dyld.h>  // FIXED: Added for _dyld_get_image_header

// Project headers
#include "memory/memory.hpp"
#include "macho/macho.hpp"

namespace fs = std::filesystem;

void save_offset(uintptr_t offset) {
    const char* home = std::getenv("HOME");
    if (!home) return;

    fs::path doc_path = fs::path(home) / "Documents" / "offsets.txt";
    
    std::stringstream ss;
    ss << "PRINT_FUNCTION = 0x" << std::hex << offset;
    std::string entry = ss.str();

    std::ofstream outfile(doc_path, std::ios::app);
    if (outfile.is_open()) {
        outfile << entry << " (Found: " << __DATE__ << ")" << std::endl;
        std::cout << "[+] Saved offset to Documents/offsets.txt" << std::endl;
    }
}

int main() {
    // FIXED: Changed _get_image_header to _dyld_get_image_header
    vm_address_t base = (vm_address_t)_dyld_get_image_header(0); 
    task_t task = mach_task_self();

    // Intel Signature for the function
    const std::vector<uint8_t> sig = { 
        0x55, 0x48, 0x89, 0xE5, 0x41, 0x57, 0x41, 0x56, 0x41, 0x55, 0x41, 0x54, 0x53, 0x48, 0x81, 0xEC 
    };

    auto text = macho::get_section(task, base, "__TEXT", "__text");
    if (text) {
        std::vector<uint8_t> buffer(text->size);
        if (memory::read_bytes(task, text->address, buffer.data(), text->size)) {
            for (size_t i = 0; i <= buffer.size() - sig.size(); ++i) {
                if (std::memcmp(&buffer[i], sig.data(), sig.size()) == 0) {
                    uintptr_t offset = (text->address + i) - base;
                    std::cout << "[+] Found Offset: 0x" << std::hex << offset << std::endl;
                    save_offset(offset);
                    break;
                }
            }
        }
    }
    return 0;
}