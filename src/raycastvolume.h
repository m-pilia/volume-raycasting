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

#include <QMatrix4x4>
#include <QOpenGLExtraFunctions>
#include <QVector3D>

#include "mesh.h"

/*!
 * \brief Class for a raycasting volume.
 */
class RayCastVolume : protected QOpenGLExtraFunctions
{
public:
    RayCastVolume(void);
    virtual ~RayCastVolume();

    void load_volume(const QString &filename);
    void create_noise(void);
    void paint(void);
    std::pair<double, double> range(void);


    /*!
     * \brief Get the extent of the volume.
     * \return A vector holding the extent of the bounding box.
     *
     * The extent is normalised such that the longest side of the bounding
     * box is equal to 1.
     */
    QVector3D extent(void) {
        auto e = m_size * m_spacing;
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

private:
    GLuint m_volume_texture;
    GLuint m_noise_texture;
    Mesh m_cube_vao;
    std::pair<double, double> m_range;
    QVector3D m_origin;
    QVector3D m_spacing;
    QVector3D m_size;

    float scale_factor(void);
};
