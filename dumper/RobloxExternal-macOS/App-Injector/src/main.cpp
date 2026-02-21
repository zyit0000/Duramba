#include <iostream>
#include <vector>
#include <fstream>
#include <filesystem>

// Your project headers
#include "src/memory/memory.hpp"
#include "src/macho/macho.hpp"
#include "src/scanner/scanner.hpp"
#include "roblox/offsets.hpp"
#include "roblox/math.hpp"
#include "roblox/string.hpp"

namespace fs = std::filesystem;

void save_print_offset(uintptr_t offset) {
    const char* home = std::getenv("HOME");
    if (!home) return;

    fs::path doc_path = fs::path(home) / "Documents" / "offsets.txt";
    std::string entry = "PRINT_FUNCTION = 0x" + std::to_string(offset);

    // Check for duplicates before writing
    if (fs::exists(doc_path)) {
        std::ifstream infile(doc_path);
        std::string line;
        while (std::getline(infile, line)) {
            if (line.find(entry) != std::string::npos) return; 
        }
    }

    std::ofstream outfile(doc_path, std::ios::app);
    if (outfile.is_open()) {
        outfile << entry << " (Found: " << __DATE__ << ")" << std::endl;
        std::cout << "[+] Saved Print Offset to Documents/offsets.txt" << std::endl;
    }
}

void find_print_offset(task_t task, vm_address_t image_base) {
    std::cout << "--- Scanning for Print Function (Intel) ---" << std::endl;

    // Intel x86_64 AOB Signature
    const std::vector<uint8_t> print_sig = { 
        0x55, 0x48, 0x89, 0xE5, 0x41, 0x57, 0x41, 0x56, 0x41, 0x55, 0x41, 0x54, 0x53, 0x48, 0x81, 0xEC 
    };

    // Use your project's macho utility to find the code section
    auto text_section = macho::get_section(task, image_base, "__TEXT", "__text");
    if (!text_section) {
        std::cout << "[-] Could not find __TEXT section" << std::endl;
        return;
    }

    // Read the section using your project's memory utility
    std::vector<uint8_t> buffer(text_section->size);
    if (!memory::read_bytes(task, text_section->address, buffer.data(), text_section->size)) {
        std::cout << "[-] Failed to read __TEXT memory" << std::endl;
        return;
    }

    // Search for the pattern
    for (size_t i = 0; i <= buffer.size() - print_sig.size(); ++i) {
        if (std::memcmp(&buffer[i], print_sig.data(), print_sig.size()) == 0) {
            uintptr_t absolute_addr = text_section->address + i;
            uintptr_t relative_offset = absolute_addr - image_base;
            
            std::cout << "[+] Found Print at Offset: 0x" << std::hex << relative_offset << std::endl;
            save_print_offset(relative_offset);
            return;
        }
    }
    std::cout << "[-] Print signature not found." << std::endl;
}

int main() {
    // This part requires actual macOS task ports to run
#ifdef __APPLE__
    task_t task = mach_task_self(); 
    vm_address_t image_base = (vm_address_t)_get_image_header(0);
    find_print_offset(task, image_base);
#else
    std::cout << "Switch to your Intel Mac to run this scanner." << std::endl;
#endif
    return 0;
}