# Bytepacker
Bytepacker is a header-only C++ utility for packing bytes in a **specific-range** and **size** and is use for networking where packet length is strict and packet headers must be well-defined. 

It also transforms buffers from little-endian to big-endian and vice-versa so you don't
have to.

## WARNING & LIMITATIONS
This is an experimental utility. It uses compile-time recursion to ensure the data written
never exceeds the buffer size. Because of this design feature, your compiler may 
give an error that recursion steps are too high if you have over 900 or so elements.
For most practical scenarios this is a non-issue.

Because this is experimental, it will most likely go under several API revisions
until a more fitting API is chosen.

# Saftey
Bytepacker utilities expect a uint8_t array of static size. This is important because most device-to-device protocols expect particular lengths. Bytepacker does not create or use buffers of variable lengths. For buffers that may contain string data, define a strict max length and create the buffer of that size.

# Serializing
`start` is the index position to begin reading the input buffer.
The conventional way to serialize with Bytepacker is to use the `size_t pack<start>(buffer, data)` utility function. It will return the number of bytes packed successfully. If no bytes could be packed, then `zero` is returned. You can use the bytes written and the expected
bytes written to assert that your buffer has been defined well enough to store all your data.

# Deserializing
The conventional way to deserialize with Bytepacker is to use the `auto unpack<start, sizeof(T)>(buffer)` utility function.  It returns an `array_buffer<len>` object where `len-1` is the number of bytes stored in the array. `T` is the type of object you intend to cast to after successfully reading the buffer. Read on below to learn how to cast into the final type.

# Array Buffers
Unpacking will return a type `array_buffer<len>` which acts just like `std::array<uint8_t, len>` and it also has a `.to<T>()` function to safely cast the buffer to your target object type regardless of your platform's host byte order is.

!NOTE! array buffers have `+1` elements because the last is reserved as the null byte. This is useful when using `.to<std::string>()` so you can print your buffers.
