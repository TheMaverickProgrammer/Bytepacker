#include <string>
#include <cassert>
#include <iostream>

#include "../include/bytepacker.h"

int main() {
        if(is_big_endian()) { 
            std::cout << "on big endian" << std::endl;
        } else {
            std::cout << "on little endian" << std::endl;
        }

        char test[15] = { "this is a test" };
        std::cout << reverse_raw_buffer<14>((std::uint8_t*)test) << std::endl;
        
        constexpr int buffer_len = 66;
        // constexpr int buffer_len = 64;

        std::uint8_t buffer[buffer_len] = { (std::uint8_t)0 };
        fill_raw_buffer(buffer,"BOB");
        fill_raw_buffer(buffer+3,"hello");
        fill_raw_buffer(buffer+8,"DANIEL");
        fill_raw_buffer(buffer+14,"world");

        auto hello = unpack<3, 5>(buffer);
        auto world = unpack<14, 5>(buffer);

        std::list<int> enums = { 0, 1, 2, 3, 4, 9 };

        std::cout << buffer << std::endl;
        std::cout << hello.to<std::string>() << " " << world.to<std::string>() << std::endl; // hello world

        std::fill(buffer, buffer+buffer_len, 0);

        size_t total = 0;
        size_t written = 0;

        written = pack<0>(buffer, 2);
        std::cout << "wrote " << written << std::endl;
        total += written;

        written = pack<1, 6>(buffer, hello.data());
        std::cout << "wrote " << written << std::endl;
        total += written;

        written = pack_each<7>(buffer, enums);
        std::cout << "wrote " << written << " expected " << (8*enums.size()) << std::endl;
        total += written;

        buffer[total] = (std::uint8_t)0;

        std::cout << "Total: " << total << std::endl;

        print_buffer(buffer, total);

        auto hello_unpacked = unpack<1, 6>(buffer);
        std::cout << "Data: " << hello_unpacked.data() << std::endl;

        enums.clear();
        unpack_each<7>(buffer, 6, enums);

        std::cout << "Enums: ";
        
        for(auto&& e : enums) {
            std::cout << std::to_string(e) << " ";
        }

        std::cout << std::endl;

        assert(total <= buffer_len && "Total written should never exceed the buffer's capacity");

        std::cout << "Remaining buffer count: " << (buffer_len-total) << std::endl;

        return 0;
}