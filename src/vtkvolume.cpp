/*
 * Copyright © 2018 Martino Pilia <martino.pilia@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <iostream>
#include <fstream>
#include <cstdio>
#include <cstring>
#include <vector>
#include <string>
#include <limits>

#include "vtkvolume.h"


/*!
 * \brief Create an empty volume.
 */
VTKVolume::VTKVolume()
{
}


/*!
 * \brief Create a volume from file.
 * \param filename File to be loaded.
 */
VTKVolume::VTKVolume(const std::string& filename)
{
    load_volume(filename);
}


/*!
 * \brief Destructor.
 */
VTKVolume::~VTKVolume()
{
}


/*!
 * \brief Check whether the architecture is little endian.
 * \return `true` if little endian, `false` if big endian.
 */
static bool is_little_endian()
{
    union {
        unsigned long l;
        unsigned char c[sizeof(unsigned long)];
    } x;
    x.l = 1;
    return x.c[0] == 1;
}


/*!
 * \brief Invert the endianness of the input value.
 * \param p Pointer to the first byte of an input value of size `N`.
 */
template<size_t N>
void swap_byte_order(unsigned char* p)
{
    unsigned char tmp;
    for (size_t i = 0; i < N / 2; ++i) {
        tmp = p[i];
        p[i] = p[N-i-1];
        p[N-i-1] = tmp;
    }
}


/*!
 * \brief Check if a VTK file is binary.
 * \param header Header lines of the file.
 * \return `true` if binary.
 */
static bool is_binary(const std::vector<std::string> &header)
{
    for (auto it = header.begin(); it != header.end(); ++it) {
        if (it->substr(0, 6) == "BINARY") {
            return true;
        }
        else if (it->substr(0, 5) == "ASCII") {
            return false;
        }
    }
    throw VTKReadError("Cannot read file format.");
}


/*!
 * \brief Read VTK data from file.
 * \param file Stream from the open file.
 * \param image_data Array of `unisgned char` to store the resulting data of type `T`.
 * \param binary Whether the VTK file is binary.
 * \param element_count Number of elements to read.
 * \return The range of the read data.
 */
template<typename T>
static void read_data(std::ifstream& file, const bool binary, const size_t element_count, std::vector<unsigned char>& image_data, std::pair<double, double>& range) {

    std::vector<T> data;
    data.resize(element_count);

    if (binary) {
        file.read(reinterpret_cast<char*>(data.data()), static_cast<std::streamsize>(sizeof (T) * element_count));
        if (is_little_endian() && sizeof (T) > 1) {
            for (size_t i = 0; i < element_count; ++i) {
                swap_byte_order<sizeof (T)>(reinterpret_cast<unsigned char*>(data.data() + i));
            }
        }
    }
    else {
        T voxel;
        for (size_t i = 0; i < element_count; ++i) {
            file >> voxel;
            data.push_back(voxel);
        }
    }

    // Find range
    range = {std::numeric_limits<double>::max(), std::numeric_limits<double>::min()};
    for (size_t i = 0; i < element_count; ++i) {
        if (data[i] > range.second) {
            range.second = data[i];
        }
        if (data[i] < range.first) {
            range.first = data[i];
        }
    }

    size_t size = data.size() * sizeof (data[0]);
    image_data.clear();
    image_data.resize(size);
    std::memcpy(image_data.data(), reinterpret_cast<char*>(data.data()), size);
}


/*!
 * \brief Cast the data to `unsigned char`, and normalise its range to [0,255].
 * \param data Input data of type `T`.
 * \param range Range of the input data.
 * \param normal_data Array to store the resulting normalised data.
 * \param element_count Number of input elements.
 */
template<typename T>
void cast_and_normalise(std::vector<unsigned char>& data, const std::pair<double, double>& range, std::vector<unsigned char>& normal_data, const size_t element_count) {
    auto *p = reinterpret_cast<T*>(data.data());

    // Normalise and cast
    normal_data.clear();
    normal_data.resize(element_count);
    for (size_t i = 0; i < element_count; ++i) {
        normal_data[i] = static_cast<unsigned char>(255 * (static_cast<double>(p[i]) - range.first) / (range.second - range.first));
    }
}


/*!
 * \brief Read the dimensions from a VTK header.
 * \param header Lines of the VTK header.
 */
void VTKVolume::read_dimensions(const std::vector<std::string> &header)
{
    for (auto it = header.begin(); it != header.end(); ++it) {
        if (it->substr(0, 10) == "DIMENSIONS") {
            int width, height, depth;
            if (3 != std::sscanf(it->c_str(), "%*s %d %d %d", &width, &height, &depth)) {
                throw VTKReadError("Cannot read volume dimension.");
            }
            m_size = {width, height, depth};
        }
    }
}


/*!
 * \brief Read the origin from a VTK header.
 * \param header Lines of the VTK header.
 */
void VTKVolume::read_origin(const std::vector<std::string> &header)
{
    for (auto it = header.begin(); it != header.end(); ++it) {
        if (it->substr(0, 6) == "ORIGIN") {
            float ox, oy, oz;
            if (3 != std::sscanf(it->c_str(), "%*s %f %f %f", &ox, &oy, &oz)) {
                throw VTKReadError("Cannot read volume origin.");
            }
            m_origin = {ox, oy, oz};
        }
    }
}


/*!
 * \brief Read the spacing from a VTK header.
 * \param header Lines of the VTK header.
 */
void VTKVolume::read_spacing(const std::vector<std::string> &header)
{
    for (auto it = header.begin(); it != header.end(); ++it) {
        if (it->substr(0, 7) == "SPACING") {
            float sx, sy, sz;
            if (3 != std::sscanf(it->c_str(), "%*s %f %f %f", &sx, &sy, &sz)) {
                throw VTKReadError("Cannot read volume spacing.");
            }
            m_spacing = {sx, sy, sz};
        }
    }
}


/*!
 * \brief Read the data type from a VTK header.
 * \param header Lines of the VTK header.
 *
 * For the description of VTK data types, see
 * https://www.vtk.org/wp-content/uploads/2015/04/file-formats.pdf
 */
void VTKVolume::read_data_type(const std::vector<std::string> &header)
{
    for (auto it = header.begin(); it != header.end(); ++it) {
        if (it->substr(0, 7) == "SCALARS") {
            char buffer[30];
            if (1 != std::sscanf(it->c_str(), "%*s %*s %s", buffer)) {
                throw VTKReadError("Cannot read volume data type.");
            }
            const std::string s(buffer);

            if (s == "unsigned_char") {
                m_datatype = DataType::Uint8;
            }
            else if (s == "char") {
                m_datatype = DataType::Int8;
            }
            else if (s == "unsigned_short") {
                m_datatype = DataType::Uint16;
            }
            else if (s == "short") {
                m_datatype = DataType::Int16;
            }
            else if (s == "unsigned_int") {
                m_datatype = DataType::Uint32;
            }
            else if (s == "int") {
                m_datatype = DataType::Int32;
            }
            else if (s == "unsigned_long") {
                m_datatype = DataType::Uint64;
            }
            else if (s == "long") {
                m_datatype = DataType::Int64;
            }
            else if (s == "float") {
                m_datatype = DataType::Float;
            }
            else if (s == "double") {
                m_datatype = DataType::Double;
            }
            else {
                throw VTKReadError("Unsupported volume data type.");
            }
        }
    }
}


/*!
 * \brief Load a volume from file.
 * \param filename Name of the file.
 */
void VTKVolume::load_volume(const std::string& filename)
{
    // Open file
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        throw VTKReadError("Cannot open file.");
    }

    // Read VTK header
    //
    // # vtk DataFile Version x.x\n
    // comment\n
    // BINARY\n
    // DATASET STRUCTURED_POINTS\n
    // DIMENSIONS 128 128 128\n
    // ORIGIN 0.0 0.0 0.0\n
    // SPACING 1.0 1.0 1.0\n
    // POINT_DATA 2097152\n
    // SCALARS image_data unsigned_char\n
    // LOOKUP_TABLE default\n
    const size_t header_line_count = 10;
    std::vector<std::string> header;
    for (size_t i = 0; i < header_line_count; i++) {
        std::string line;
        std::getline(file, line);
        if (line.empty()) {
            file.close();
            throw VTKReadError("Cannot read header, missing line " + std::to_string(i + 1) + ".");
        }
        else {
            header.push_back(line);
        }
    }

    // Check magic number
    if (header[0].substr(0, 5) != "# vtk") {
        file.close();
        throw VTKReadError("Not a valid VTK file.");
    }

    // Read metadata
    read_data_type(header);
    read_dimensions(header);
    read_origin(header);
    read_spacing(header);

    // Read data
    size_t element_count = std::get<0>(m_size) * std::get<1>(m_size) * std::get<2>(m_size);
    const bool binary = is_binary(header);
    switch (m_datatype) {
    case VTKVolume::DataType::Int8:
        read_data<char>(file, binary, element_count, m_data, m_range);
        break;
    case VTKVolume::DataType::Uint8:
        read_data<unsigned char>(file, binary, element_count, m_data, m_range);
        break;
    case VTKVolume::DataType::Int16:
        read_data<unsigned short>(file, binary, element_count, m_data, m_range);
        break;
    case VTKVolume::DataType::Uint16:
        read_data<short>(file, binary, element_count, m_data, m_range);
        break;
    case VTKVolume::DataType::Int32:
        read_data<unsigned int>(file, binary, element_count, m_data, m_range);
        break;
    case VTKVolume::DataType::Uint32:
        read_data<int>(file, binary, element_count, m_data, m_range);
        break;
    case VTKVolume::DataType::Int64:
        read_data<unsigned long>(file, binary, element_count, m_data, m_range);
        break;
    case VTKVolume::DataType::Uint64:
        read_data<long>(file, binary, element_count, m_data, m_range);
        break;
    case VTKVolume::DataType::Float:
        read_data<float>(file, binary, element_count, m_data, m_range);
        break;
    case VTKVolume::DataType::Double:
        read_data<double>(file, binary, element_count, m_data, m_range);
        break;
    }

    file.close();
}


/*!
 * \brief Cast the data to `unsigned char` and normalise it to [0, 255].
 */
void VTKVolume::uint8_normalised(void) {
    size_t element_count = std::get<0>(m_size) * std::get<1>(m_size) * std::get<2>(m_size);
    std::vector<unsigned char> normal_data;
    normal_data.resize(element_count);

    switch (m_datatype) {
    case VTKVolume::DataType::Int8:
        cast_and_normalise<char>(m_data, m_range, normal_data, element_count);
        break;
    case VTKVolume::DataType::Uint8:
        cast_and_normalise<unsigned char>(m_data, m_range, normal_data, element_count);
        break;
    case VTKVolume::DataType::Int16:
        cast_and_normalise<short>(m_data, m_range, normal_data, element_count);
        break;
    case VTKVolume::DataType::Uint16:
        cast_and_normalise<unsigned short>(m_data, m_range, normal_data, element_count);
        break;
    case VTKVolume::DataType::Int32:
        cast_and_normalise<int>(m_data, m_range, normal_data, element_count);
        break;
    case VTKVolume::DataType::Uint32:
        cast_and_normalise<unsigned int>(m_data, m_range, normal_data, element_count);
        break;
    case VTKVolume::DataType::Int64:
        cast_and_normalise<long>(m_data, m_range, normal_data, element_count);
        break;
    case VTKVolume::DataType::Uint64:
        cast_and_normalise<unsigned long>(m_data, m_range, normal_data, element_count);
        break;
    case VTKVolume::DataType::Float:
        cast_and_normalise<float>(m_data, m_range, normal_data, element_count);
        break;
    case VTKVolume::DataType::Double:
        cast_and_normalise<double>(m_data, m_range, normal_data, element_count);
        break;
    }

    m_data = std::move(normal_data);
    m_datatype = DataType::Uint8;
}
