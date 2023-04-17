#include "filesmodel.h"

//#include "compressor/compressor.h"

#include <QDirIterator>
#include <QFile>

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

    int width; // bitmap width in pixels
    int height; // bitmap height in pixels
    unsigned char * data; // Pointer to bitmap data. data[j * width + i] is color of pixel in row j and column i.
};

#pragma pack(pop)

void check_color_header(BMPColorHeader &bmp_color_header) {
    BMPColorHeader expected_color_header;
    if(expected_color_header.red_mask != bmp_color_header.red_mask ||
        expected_color_header.blue_mask != bmp_color_header.blue_mask ||
        expected_color_header.green_mask != bmp_color_header.green_mask ||
        expected_color_header.alpha_mask != bmp_color_header.alpha_mask) {
        throw std::runtime_error("Unexpected color mask format! The program expects the pixel data to be in the BGRA format");
    }
    if(expected_color_header.color_space_type != bmp_color_header.color_space_type) {
        throw std::runtime_error("Unexpected color space type! The program expects sRGB values");
    }
}

uint32_t make_stride_aligned(uint32_t align_stride, const uint32_t old_row_stride) {
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

        // The BMPColorHeader is used only for transparent images
        if(bmp_info_header.bit_count == 32) {
            // Check if the file has bit mask color information
            if(bmp_info_header.size >= (sizeof(BMPInfoHeader) + sizeof(BMPColorHeader))) {
                inp.read((char*)&bmp_color_header, sizeof(bmp_color_header));
                // Check if the pixel data is stored as BGRA and if the color space type is sRGB
                check_color_header(bmp_color_header);
            } else {
                std::cerr << "Warning! The file \"" << file_name << "\" does not seem to contain bit mask information\n";
                throw std::runtime_error("Error! Unrecognized file format.");
            }
        }

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
            uint32_t row_stride{ 0 };
            row_stride = bmp_info_header.width * bmp_info_header.bit_count / 8;
            uint32_t new_stride = make_stride_aligned(4, row_stride);
            std::vector<uint8_t> padding_row(new_stride - row_stride);

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


#define MAX_BITMAP_WIDTH 90000 // 30000 * 3  <=> max bitmap width * every byte coded as 3 bits

int compress(const std::string &file_name)
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

    auto writeToRow = [&row_data, &buffer, &buffer_it, &is_row_empty]()
    {
        char byte = static_cast<char>(buffer);
        if (is_row_empty && byte != 0)
        {
            is_row_empty = false;
        }
        row_data += byte;
        //std::cerr << "Buffer before: " << buffer << std::endl;
        buffer = buffer >> 8; // possible issue
        buffer_it -= 8;
        //std::cerr << "Buffer after: " << buffer << std::endl;
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

    const auto data = read_bmp(file_name);
    std::cout << "Data size: " << data->width << " X " << data->height << std::endl;
    for (int i = 0; i < data->width; ++i)
    {
        is_row_empty = true;
        row_data.clear();
        buffer = 0;
        buffer_it = 0;

        bool current_color_white = false;
        int count = 0;
        for (int j = 0; j < data->height; ++j)
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
                    writeToBuffer(Values::Black, count);
                    current_color_white = true;
                    count = 1;
                }
            }
            else
            {// black
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
            // zero 16 bit
            out_file_data.append((const char*)&row_size, 2);
        }
        else
        {
            row_size = row_data.size();
            out_file_data.append((const char*)&row_size, 2);
            out_file_data.append(row_data);
        }
        std::cerr << "row " << i << " size: " << row_size << " is empty: " << is_row_empty << std::endl;
    }
    /// TODO: write to file
    return 0;
}

int decompress(const std::string &file_name)
{
    return 0;
}


/**********************************************************************************************************/


FilesModel::FilesModel(QObject *)
{

}

void FilesModel::update()
{
    beginResetModel();
    QDirIterator it(path, QStringList(), QDir::Files, QDirIterator::NoIteratorFlags);
    while (it.hasNext())
    {
        QFileInfo fileInfo(it.next());
        std::cout << fileInfo.fileName().toStdString() << " " << fileInfo.size() << "B" << std::endl;
        modelData.push_back({fileInfo.fileName(), fileInfo.filePath().toStdString(), "", fileInfo.size(), FileStatus::Processing});
    }
    endResetModel();
}

QVariant FilesModel::data(const QModelIndex &index, int role) const
{
    if (index.row() >= modelData.size()
            || role >= Params::Last)
    {
        return QVariant();
    }

    const auto& item = modelData.at(index.row());
    switch (role) {
        case Params::Name : return item.name;
        case Params::Extention : return item.ext;
        case Params::Size : return item.size;
        case Params::Status : return item.status;
    }
    return QVariant();
}

void FilesModel::setPath(const QString &value)
{
     if (value != path)
     {
         path = value;
         emit pathChanged();
         update();
     }
}

void FilesModel::compressFile(const int index) const
{
    if (index >= modelData.size())
    {
        //TODO: notify Error
        return;
    }
    try {
        const auto data = compress(modelData[index].fullName);
        data;
    } catch (const std::exception& e) {
        std::cerr << "Compresing failed: " << e.what() << std::endl;
    }

}

int FilesModel::rowCount(const QModelIndex &parent) const
{
    return static_cast<int>(modelData.size());
}

QHash<int, QByteArray> FilesModel::roleNames() const
{
    return {
        {static_cast<int>(Params::Name), "name"},
        {static_cast<int>(Params::Extention), "ext"},
        {static_cast<int>(Params::Size), "size"},
        {static_cast<int>(Params::Status), "status"}
    };
}
