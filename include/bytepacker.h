#include <cstdio>
#include <utility>
#include <array>
#include <list>
#include <string>

#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif

namespace bytepacker {
    constexpr bool is_big_endian() {
        std::uint16_t temp = 0x1234;
        std::uint8_t* tempChar = (std::uint8_t*)&temp; 
        return *tempChar == 0x12;
    }

    template<size_t len>
    constexpr std::uint8_t* reverse_raw_buffer(std::uint8_t*);

    namespace detail {
        template<typename T>
        auto buffer_cast(T nonce, std::uint8_t* buffer) {
            return *(T*)buffer;
        }

        template<typename T>
        auto buffer_cast(T* nonce, std::uint8_t* buffer) {
            return (T*)buffer;
        }

        auto buffer_cast(std::string nonce, std::uint8_t* buffer) {
            return std::string((char*)buffer);
        }

        template<size_t buffer_len, size_t... Is>
        constexpr std::uint8_t* reverse_raw_buffer_step(std::index_sequence<Is...>, std::uint8_t* buffer) {
            int l[] = { (std::swap(buffer[Is], buffer[buffer_len - Is - 1u]),0)... };
            return buffer;
        }

        template<size_t len>
        constexpr std::uint8_t* flip_byte_order_if_little_endian(std::uint8_t* buffer) {
            if(!is_big_endian()) {
                return reverse_raw_buffer<len>(buffer);
            }

            return buffer;
        }
    }

    template<size_t sz>
    struct array_buffer;

    template<size_t len>
    constexpr std::uint8_t* reverse_raw_buffer(std::uint8_t* buffer) {
        return detail::reverse_raw_buffer_step<len>(std::make_index_sequence<len/2u>{}, buffer);
    }

    template<typename T>
    void fill_raw_buffer(std::uint8_t* buffer, const T* data, size_t offset = 0) {
        std::uint8_t* stream = (std::uint8_t*)data;

        if(is_big_endian()) {
            size_t i = 0;
            while(data[i] != (std::uint8_t)0) {
                buffer[i+offset] = stream[i];
                i++;
            }
        } else {
            // count first
            size_t count = 0;
            while(data[count] != (std::uint8_t)0) {
                count++;
            }

            while(count+1 != 0) {
                buffer[count+offset] = stream[count];
                count--;
            }
        }
    }

    template<size_t len>
    constexpr std::uint8_t* (* const network_byte_order)(std::uint8_t*) = detail::flip_byte_order_if_little_endian<len>;

    template<size_t len>
    constexpr std::uint8_t* host_byte_order(std::uint8_t* buffer) {
        return detail::flip_byte_order_if_little_endian<len>(buffer);
    }

    //!< Special implementation for array_buffers because I didn't think things through 
    template<size_t len>
    constexpr array_buffer<len> host_byte_order(array_buffer<len>&& buffer) {
        // smart-buffers are null-terminated, so drop the last element from the swap list...
        detail::flip_byte_order_if_little_endian<len-1u>(buffer.data());
        return buffer;
    }

    namespace detail {
        template<size_t start, size_t len, size_t size, size_t... Is>
        constexpr auto unpack(std::index_sequence<Is...> ls, std::uint8_t(&buffer)[size]) -> array_buffer<(sizeof...(Is))+1u> {
            return { (std::uint8_t)(*(buffer + Is + start))..., (std::uint8_t)0 };
        }

        template<size_t start, size_t len, size_t size, size_t... Is>
        constexpr auto pack(std::index_sequence<Is...> ls, std::uint8_t(&buffer)[size], const std::uint8_t* data) {
            std::fill(buffer + start, buffer + start + sizeof...(Is), 0);
            int l = ((*(buffer + Is + start) = data[Is]), ...);
            return sizeof...(Is);
        }

        // Packing with Element = T
        template<size_t Idx, size_t start, size_t len, size_t size, template <typename, typename> typename Container, typename T, typename AllocT>
        constexpr size_t pack_container(std::uint8_t(&buffer)[size], const Container<T, AllocT>& container) {
            constexpr size_t new_start = start + (Idx * len);
            if constexpr (new_start < size) {
                if (Idx < container.size()) {
                    constexpr size_t seq_count = min(len, size - new_start);
                    const std::uint8_t* data_to_pack = network_byte_order<seq_count>((std::uint8_t*)&(*std::next(container.begin(), Idx)));
                    return pack<new_start, len, size>(std::make_index_sequence<seq_count>{}, buffer, data_to_pack)
                        + pack_container<Idx + 1u, start, len>(buffer, container);
                }
            }

            return 0;
        }

        // Unpacking with Element = T
        template<size_t Idx, size_t start, size_t len, size_t size, template <typename, typename> typename Container, typename T, typename AllocT>
        constexpr void unpack_container(std::uint8_t(&buffer)[size], size_t elements, Container<T, AllocT>& container) {
            constexpr size_t new_start = start + (Idx * len);
            if constexpr (new_start < size) {
                if (Idx < elements) {
                    constexpr size_t seq_count = min(len, size - new_start);
                    auto bytes = unpack<new_start, len, size>(std::make_index_sequence<seq_count>{}, buffer);
                    container.emplace_back(*(T*)host_byte_order<len>(bytes.data()));
                    unpack_container<Idx + 1u, start, len>(buffer, elements, container);
                }
            }
        }
    }

    // NOTE: Keep in mind array_buffer<> structures reserve +1u elements for the last zero
    template<size_t start, size_t len, size_t size>
    constexpr auto unpack(std::uint8_t(&buffer)[size]) -> array_buffer<len+1u> {
        constexpr size_t seq_count = min(len, size - start);
        return host_byte_order<len+1u>(detail::unpack<start, len>(std::make_index_sequence<seq_count>{}, buffer));
    }


    //<! Type = T
    template<size_t start, size_t size, typename T>
    constexpr size_t pack(std::uint8_t(&buffer)[size], const T& data) {
        constexpr size_t len = sizeof(T);
        constexpr size_t seq_count = min(len, size - start);
        const std::uint8_t* data_to_pack = network_byte_order<len>((std::uint8_t*)&data);
        return detail::pack<start, len>(std::make_index_sequence<seq_count>{}, buffer, data_to_pack);
    }

    //<! Type = T
    template<size_t start, size_t len, size_t size, typename T>
    constexpr size_t pack(std::uint8_t(&buffer)[size], const T& data) {
        constexpr size_t seq_count = min(len, size - start);
        const std::uint8_t* data_to_pack = network_byte_order<len>((std::uint8_t*)&data);
        return detail::pack<start, len>(std::make_index_sequence<seq_count>{}, buffer, data_to_pack);
    }

    /*<! Type = T(&)[N]
    template<size_t start, size_t size, size_t data_size, typename T>
    constexpr size_t pack(std::uint8_t(&buffer)[size], T(&data)[data_size]) {
        constexpr size_t len = data_size*sizeof(T);
        constexpr size_t seq_count = min(len, size - start);
        return detail::pack<start, len>(std::make_index_sequence<seq_count>{}, buffer, (const std::uint8_t*)&data);
    }*/

    //<! Type = T*
    template<size_t start, size_t len, size_t size, typename T>
    constexpr size_t pack(std::uint8_t(&buffer)[size], T* data) {
        constexpr size_t seq_count = min(len, size - start);
        const std::uint8_t* data_to_pack = network_byte_order<len>((std::uint8_t*)data);
        return detail::pack<start, len>(std::make_index_sequence<seq_count>{}, buffer, data_to_pack);
    }

    //<! Special case: when packing a container with elements of type T
    template<size_t start, std::uint8_t each_len, size_t size, template <typename, typename> typename Container, typename T, typename AllocT>
    constexpr size_t pack_each(char(&buffer)[size], const Container<T, AllocT>& container) {
        return detail::pack_container<0, start, each_len>(buffer, container);
    }

    //<! Special case: when packing a container with elements of type T with a restricted size
    template<size_t start, size_t size, template <typename, typename> typename Container, typename T, typename AllocT>
    constexpr size_t pack_each(std::uint8_t(&buffer)[size], const Container<T, AllocT>& container) {
        constexpr size_t each_len = sizeof(T);
        return detail::pack_container<0, start, each_len>(buffer, container);
    }

    //<! Special case: when unpacking a container with elements of type T with a restricted size
    template<size_t start, size_t each_len, size_t size, template <typename, typename> typename Container, typename T, typename AllocT>
    constexpr void unpack_each(std::uint8_t(&buffer)[size], size_t elements, Container<T, AllocT>& container) {
        detail::unpack_container<0, start, each_len>(buffer, elements, container);
    }

    //<! Special case: when unpacking a container with elements of type T
    template<size_t start, size_t size, template <typename, typename> typename Container, typename T, typename AllocT>
    constexpr void unpack_each(std::uint8_t(&buffer)[size], size_t elements, Container<T, AllocT>& container) {
        constexpr size_t each_len = sizeof(T);
        detail::unpack_container<0, start, each_len>(buffer, elements, container);
    }

    //<! array_buffer is a wrapper over std::array that acts as the final outgoing buffer
    template<size_t sz>
    struct array_buffer : std::array<std::uint8_t, sz> {
        size_t written{0};

        template<typename T>
        auto to() {
            return detail::buffer_cast(T{}, this->data());
        } 
    };
}