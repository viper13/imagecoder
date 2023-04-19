#ifndef CODER_H
#define CODER_H

#include <string>

int compress(const std::string &file_name_in, const std::string &file_name_out);

int decompress(const std::string &file_name_in, const std::string &file_name_out);

#endif // CODER_H
