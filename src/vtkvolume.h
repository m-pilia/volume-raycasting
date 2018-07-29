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

#pragma once

#include <QVector3D>
#include <QMatrix4x4>

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>


class VTKReadError : public std::runtime_error {
    using std::runtime_error::runtime_error;
};


/*!
 * \brief Represent a VTK volume.
 *
 * This class allows to load a VTK volume from file.
 */
class VTKVolume {

public:

    VTKVolume(void) {}
    virtual ~VTKVolume() {}

    /*!
     * \brief Load data from file.
     * \param filename File to be loaded.
     */
    virtual void load_volume(const QString& filename);

    /*!
     * \brief Cast the data to `unsigned char` and normalise it to [0, 255];
     */
    void uint8_normalised(void);

    /*!
     * \brief Pointer to the data.
     */
    unsigned char * data() {
        return m_data.data();
    }

    /*!
     * \brief Size along the `x` axis.
     */
    size_t nx() {
        return static_cast<size_t>(m_dimensions.x());
    }

    /*!
     * \brief Size along the `y` axis.
     */
    size_t ny() {
        return static_cast<size_t>(m_dimensions.y());
    }

    /*!
     * \brief Size along the `z` axis.
     */
    size_t nz() {
        return static_cast<size_t>(m_dimensions.z());
    }

    /*!
     * \brief Range of the image, in intensity value.
     * \return A pair, holding <minimum, maximum>.
     */
    std::pair<double, double> range() {
        return m_range;
    }

    /*!
     * \brief Get the extent of the volume.
     * \return A vector holding the extent of the bounding box.
     *
     * The extent is normalised such that the longest side of the bounding
     * box is equal to 1.
     */
    QVector3D extent(void) {
        auto e = m_dimensions * m_spacing;
        return e / std::max({e.x(), e.y(), e.z()});
    }

    /*!
     * \brief Return the model matrix for the volume.
     * \param shift Shift the volume by its origin.
     * \return A matrix in homogeneous coordinates.
     *
     * The model matrix scales a two-unit side cube to the
     * extent of the volume.
     */
    QMatrix4x4 modelMatrix(bool shift = false) {
        QMatrix4x4 modelMatrix;
        if (shift) {
            modelMatrix.translate(-m_origin / scale_factor());
        }
        modelMatrix.scale(0.5f * extent());
        return modelMatrix;
    }

    /*!
     * \brief Top planes forming the AABB.
     * \param shift Shift the volume by its origin.
     * \return A vector holding the intercept of the top plane for each axis.
     */
    QVector3D top(bool shift = false) {
        auto t = extent() / 2.0;
        if (shift) {
            t -= m_origin / scale_factor();
        }
        return t;
    }

    /*!
     * \brief Bottom planes forming the AABB.
     * \param shift Shift the volume by its origin.
     * \return A vector holding the intercept of the bottom plane for each axis.
     */
    QVector3D bottom(bool shift = false) {
        auto b = -extent() / 2.0;
        if (shift) {
            b -= m_origin / scale_factor();
        }
        return b;
    }

    /*!
     * \brief Print volume info.
     */
    void print_info(void) {
        std::cout << "Dimensions: " << nx() << " " << ny() << " " << nz() << std::endl;
        std::cout << "Origin: " << m_origin[0] << " " << m_origin[1] << " " << m_origin[2] << std::endl;
        std::cout << "Spacing: " << m_spacing[0] << " " << m_spacing[1] << " " << m_spacing[2] << std::endl;
        std::cout << "Data type: " << static_cast<int>(m_datatype) << std::endl;
        std::cout << "Range: " << m_range.first << " " << m_range.second << std::endl;
    }

private:
    enum class DataType {Int8, Uint8, Int16, Uint16, Int32, Uint32, Int64, Uint64, Float, Double};

    QVector3D m_dimensions; /*!< Number of voxels for each axis. */
    QVector3D m_origin;     /*!< Origin, in voxel coordinates. */
    QVector3D m_spacing;    /*!< Spacing between voxels. */
    DataType m_datatype;    /*!< Data type. */
    std::pair<double, double> m_range; /*!< (min, max) of the original intensities, before normalisation. */
    std::vector<unsigned char> m_data; /*!< Volume data, casted to `unsigned char` and normalised to [0, 255]. */

    void read_dimensions(const std::vector<std::string> &header);
    void read_origin(const std::vector<std::string> &header);
    void read_spacing(const std::vector<std::string> &header);
    void read_data_type(const std::vector<std::string> &header);
    float scale_factor(void);
};
