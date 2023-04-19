#include "coder.h"

#include <iostream>

#include <fstream>
#include <vector>
#include <iostream>
#include <memory>
#include <bitset>

#pragma pack(push, 1)
struct BMPFileHeader {
    uint16_t file_type{0x4D42};          // File type always BM which is 0x4D42
    uint32_t file_size{0};               // Size of the file (in bytes)
    uint16_t reserved1{0};               // Reserved, always 0
    uint16_t reserved2{0};               // Reserved, always 0
    uint32_t offset_data{0};             // Start position of pixel data (bytes from the beginning of the file)
};

struct BMPInfoHeader {
    uint32_t size{ 0 };                      // Size of this header (in bytes)
    int32_t width{ 0 };                      // width of bitmap in pixels
    int32_t height{ 0 };                     // width of bitmap in pixels
                                             //       (if positive, bottom-up, with origin in lower left corner)
                                             //       (if negative, top-down, with origin in upper left corner)
    uint16_t planes{ 1 };                    // No. of planes for the target device, this is always 1
    uint16_t bit_count{ 0 };                 // No. of bits per pixel
    uint32_t compression{ 0 };               // 0 or 3 - uncompressed. THIS PROGRAM CONSIDERS ONLY UNCOMPRESSED BMP images
    uint32_t size_image{ 0 };                // 0 - for uncompressed images
    int32_t x_pixels_per_meter{ 0 };
    int32_t y_pixels_per_meter{ 0 };
    uint32_t colors_used{ 0 };               // No. color indexes in the color table. Use 0 for the max number of colors allowed by bit_count
    uint32_t colors_important{ 0 };          // No. of colors used for displaying the bitmap. If 0 all colors are required
};

struct BMPColorHeader {
    uint32_t red_mask{ 0x00ff0000 };         // Bit mask for the red channel
    uint32_t green_mask{ 0x0000ff00 };       // Bit mask for the green channel
    uint32_t blue_mask{ 0x000000ff };        // Bit mask for the blue channel
    uint32_t alpha_mask{ 0xff000000 };       // Bit mask for the alpha channel
    uint32_t color_space_type{ 0x73524742 }; // Default "sRGB" (0x73524742)
    uint32_t unused[16]{ 0 };                // Unused data for sRGB color space
};

struct BMPOutFile {
    BMPOutFile()
        : width(0)
        , height(0)
        , data(nullptr)
    {}
    ~BMPOutFile()
    {
        if (data)
        {
            delete [] data;
        }
    }
    BMPOutFile( const BMPOutFile& ) = delete;
    void operator=( const BMPOutFile& ) = delete;

    unsigned int width; // bitmap width in pixels
    unsigned int height; // bitmap height in pixels
    unsigned char * data; // Pointer to bitmap data. data[j * width + i] is color of pixel in row j and column i.
};

#pragma pack(pop)

uint32_t make_stride_aligned(const uint32_t align_stride, const uint32_t old_row_stride) {
    uint32_t new_stride = old_row_stride;
    while (new_stride % align_stride != 0) {
        new_stride++;
    }
    return new_stride;
}

std::shared_ptr<BMPOutFile> read_bmp(const std::string &file_name)
{
    std::shared_ptr<BMPOutFile> data = std::make_shared<BMPOutFile>();
    std::ifstream inp{ file_name, std::ios_base::binary };
    if (inp) {
        BMPFileHeader file_header;
        BMPInfoHeader bmp_info_header;
        BMPColorHeader bmp_color_header;


        inp.read((char*)&file_header, sizeof(file_header));
        if(file_header.file_type != 0x4D42) {
            throw std::runtime_error("Error! Unrecognized file format.");
        }
        inp.read((char*)&bmp_info_header, sizeof(bmp_info_header));
        if(bmp_info_header.bit_count != 8) {
            std::cerr << "Warning! The file \"" << file_name << "\" does not supported for comppression!";
            throw std::runtime_error("Error! Unrecognized file format.");
        }
;
        // Jump to the pixel data location
        inp.seekg(file_header.offset_data, inp.beg);

        // Adjust the header fields for output.
        // Some editors will put extra info in the image file, we only save the headers and the data.
        if(bmp_info_header.bit_count == 32) {
            bmp_info_header.size = sizeof(BMPInfoHeader) + sizeof(BMPColorHeader);
            file_header.offset_data = sizeof(BMPFileHeader) + sizeof(BMPInfoHeader) + sizeof(BMPColorHeader);
        } else {
            bmp_info_header.size = sizeof(BMPInfoHeader);
            file_header.offset_data = sizeof(BMPFileHeader) + sizeof(BMPInfoHeader);
        }
        file_header.file_size = file_header.offset_data;

        if (bmp_info_header.height < 0) {
            throw std::runtime_error("The program can treat only BMP images with the origin in the bottom left corner!");
        }

        data->width = bmp_info_header.width;
        data->height = bmp_info_header.height;

        int data_size = bmp_info_header.width * bmp_info_header.height * bmp_info_header.bit_count / 8;
        data->data = new unsigned char[data_size];

        // Here we check if we need to take into account row padding
        if (bmp_info_header.width % 4 == 0) {
            inp.read((char*)data->data, data_size);
            file_header.file_size += data_size;
        }
        else {
            uint32_t row_stride{ static_cast<uint32_t>(bmp_info_header.width) };
            std::vector<uint8_t> padding_row(make_stride_aligned(4, row_stride) - row_stride);
            for (int y = 0; y < bmp_info_header.height; ++y) {
                inp.read((char*)(data->data + row_stride * y), row_stride);
                inp.read((char*)padding_row.data(), padding_row.size());
            }
            file_header.file_size += data_size + bmp_info_header.height * padding_row.size();
        }
    }
    else {
        std::cerr << "Can't open file: " << file_name << std::endl;
        throw std::runtime_error("Unable to open the input image file.");
    }
    return data;
}

int compress(const std::string &file_name_in, const std::string &file_name_out)
{
    std::string out_file_data;

    uint16_t row_size = 0;
    std::string row_data;
    bool is_row_empty = false;

    uint16_t buffer_it = 0;
    uint16_t buffer;

    enum class Values
    {
        White,
        Black,
        White4,
        Black4
    };

    auto writeToRow = [&row_data, &buffer, &buffer_it]()
    {
        char byte = static_cast<char>(buffer);
        row_data += byte;
        buffer = buffer >> 8; // possible issue
        buffer_it -= 8;
    };
    auto writeToBuffer = [&buffer, &buffer_it, writeToRow](const Values data, const int count)
    {
        for (int it = 0; it < count; ++it)
        {
            switch (data) {
            case Values::White : {
                buffer |= (1 << buffer_it++);// true
                buffer |= (1 << buffer_it++);
                buffer &= ~(1 << buffer_it++);// false
            } break;
            case Values::Black : {
                buffer |= (1 << buffer_it++);
                buffer |= (1 << buffer_it++);
                buffer |= (1 << buffer_it++);
            } break;
            case Values::White4 : {
                buffer &= ~(1 << buffer_it++);
            } break;
            case Values::Black4 : {
                buffer |= (1 << buffer_it++);
                buffer &= ~(1 << buffer_it++);
            } break;
            }
        }
        if (buffer_it > 7)
        {
            writeToRow();
        }
    };

    const auto data = read_bmp(file_name_in);
    std::cout << "Data size: " << data->width << " X " << data->height << std::endl;
    for (int i = 0; i < data->height; ++i)
    {
        is_row_empty = true;
        row_data.clear();
        buffer = 0;
        buffer_it = 0;

        bool current_color_white = false;
        int count = 0;
        for (int j = 0; j < data->width; ++j)
        {
            if (data->data[j * data->width + i] == 0xff)
            {// white
                if (current_color_white)
                {
                    ++count;
                    if (count == 4)// change to const or macros
                    {
                        writeToBuffer(Values::White4, 1);
                        count = 0;
                    }
                }
                else
                {
                    is_row_empty = false;
                    writeToBuffer(Values::Black, count);
                    current_color_white = true;
                    count = 1;
                }
            }
            else
            {// black
                is_row_empty = false;
                if (!current_color_white)
                {
                    ++count;
                    if (count == 4)// change to const or macros
                    {
                        writeToBuffer(Values::Black4, 1);
                        count = 0;
                    }
                }
                else
                {
                    writeToBuffer(Values::White, count);
                    current_color_white = false;
                    count = 1;
                }
            }
        }
        if (buffer_it > 0)
        {
            writeToRow();
        }
        row_size = 0;
        if (is_row_empty)
        {
            std::cerr << "Empty" << std::endl;
            // zero 16 bit
            out_file_data.append((const char*)&row_size, 2);
        }
        else
        {
            row_size = row_data.size();
            out_file_data.append((const char*)&row_size, 2);
            out_file_data.append(row_data);
        }
    }

    std::ofstream onp{ file_name_out, std::ios_base::binary };
    if (onp.is_open())
    {
        /*onp.write((const char*)&data->header_size, 4);
        onp.write((const char*)&data->header, data->header_size);*/
        uint16_t width = data->width;
        uint16_t height = data->height;
        onp.write((const char*)&width, 2);
        onp.write((const char*)&height, 2);
        onp.write(out_file_data.c_str(), out_file_data.size());
        onp.flush();
        onp.close();
        std::cout << "wrote the file successfully! " << file_name_out << std::endl;
    }
    else
    {
        std::cerr << "Can't open file: " << file_name_out << std::endl;
        throw std::runtime_error("Unable to open the output image file.");
    }
    return 0;
}

int decompress(const std::string &file_name_in, const std::string &file_name_out)
{
    std::ifstream inp{ file_name_in, std::ios_base::binary };
    if (inp) {
        std::shared_ptr<BMPOutFile> data = std::make_shared<BMPOutFile>();
        inp.read((char*)&(data->width), 2);
        inp.read((char*)&(data->height), 2);

        auto genNextBit = [](const unsigned char* data, unsigned int &byte_it, unsigned int &bit_it) -> bool
        {
            const unsigned char byt = data[byte_it];
            bool value = static_cast<bool>(byt & (1 << bit_it));
            ++bit_it;
            if (bit_it == 8)
            {
                ++byte_it;
                bit_it = 0;
            }
            return value;
        };
        const unsigned int out_width = static_cast<unsigned int>(data->width);
        const uint32_t alligned_width = make_stride_aligned(4, out_width);
        const auto padding = alligned_width - out_width;
        const unsigned int data_size = alligned_width * data->height;
        data->data = new unsigned char[data_size];
        for (unsigned int i = 0; i < data->height; ++i)
        {
            uint16_t row_width = 0;
            inp.read((char*)&row_width, 2);

            const unsigned int row_base_it = i * alligned_width;
            if (row_width == 0)
            {// empty row
                for (unsigned int j = 0; j < out_width; ++j)
                {
                    data->data[row_base_it + j] = 0xff;
                }
            }
            else
            {
                unsigned char* row_data = new unsigned char[static_cast<unsigned int>(row_width)];
                inp.read((char*)row_data, row_width);
                unsigned int out_it = 0;
                unsigned int byte_it = 0;
                unsigned int bit_it = 0;
                while (out_it < out_width && byte_it < row_width)
                {
                    if (genNextBit(row_data, byte_it, bit_it))
                    {
                        if (genNextBit(row_data, byte_it, bit_it))
                        {
                            if (genNextBit(row_data, byte_it, bit_it))
                            {
                                data->data[row_base_it + out_it++] = 0x00;
                            }
                            else
                            {
                                data->data[row_base_it + out_it++] = 0xff;
                            }
                        }
                        else
                        {// 4 black
                            data->data[row_base_it + out_it++] = 0x00;
                            data->data[row_base_it + out_it++] = 0x00;
                            data->data[row_base_it + out_it++] = 0x00;
                            data->data[row_base_it + out_it++] = 0x00;
                        }
                    }
                    else
                    {// 4 white
                        data->data[row_base_it + out_it++] = 0xff;
                        data->data[row_base_it + out_it++] = 0xff;
                        data->data[row_base_it + out_it++] = 0xff;
                        data->data[row_base_it + out_it++] = 0xff;
                    }
                }
                if (out_it < out_width || byte_it < row_width)
                {
                    std::cerr << "Bad: row: " << i << " width(it, size): " << out_it << " " << out_width << " in row(it, size): " << byte_it << " " << row_width << std::endl;
                }
                delete [] row_data;
                row_data = nullptr;
            }
        }
        BMPFileHeader header;
        header.offset_data = sizeof(BMPFileHeader) + sizeof(BMPInfoHeader) + sizeof (BMPColorHeader);
        header.file_size = header.offset_data + data_size;

        BMPInfoHeader info;
        info.size = sizeof(BMPInfoHeader);
        info.width = data->width;
        info.height = data->height;
        info.bit_count = 8;
        info.size_image = data_size;

        BMPColorHeader colors;

        std::ofstream onp{ file_name_out, std::ios_base::binary };
        if (onp.is_open())
        {
            onp.write((const char*)&header, sizeof(BMPFileHeader));
            onp.write((const char*)&info, sizeof(BMPInfoHeader));
            onp.write((const char*)&colors, sizeof(BMPColorHeader));
            onp.write((const char*)(data->data), header.file_size);
            onp.flush();
            onp.close();
            std::cout << "wrote the file successfully! " << file_name_out << std::endl;
        }
        else
        {
            std::cerr << "Can't open file: " << file_name_out << std::endl;
            throw std::runtime_error("Unable to open the output image file.");
        }
    }
    return 0;
}

