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

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QColorDialog>
#include <QFileDialog>
#include <QSignalBlocker>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow {parent}
    , ui {new Ui::MainWindow}
{
    ui->setupUi(this);

    // Set inital values
    ui->stepLength->valueChanged(ui->stepLength->value());
    ui->threshold_slider->valueChanged(ui->threshold_slider->value());
    ui->canvas->setBackground(Qt::black);

    // Populate list of visualisation modes
    for (const auto& mode : ui->canvas->getModes()) {
        ui->mode->addItem(mode);
    }
    ui->mode->setCurrentIndex(0);

    // Enable file drop
    setAcceptDrops(true);
}

MainWindow::~MainWindow()
{
    delete ui;
}


/*!
 * \brief Allow dragged files to enter the main window.
 * \param event Drag enter event.
 */
void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}


/*!
 * \brief Handle drop events to load volumes.
 * \param event Drop event.
 */
void MainWindow::dropEvent(QDropEvent *event)
{
    const QMimeData* mimeData = event->mimeData();

    if (mimeData->hasUrls()) {
        for (auto& url : mimeData->urls()) {
            load_volume(url.toLocalFile());
        }
    }
}


/*!
 * \brief Load a volume
 * \param path Volume file to be loaded.
 *
 * Try to load the volume. Update the UI if succesfull, or prompt an error
 * message in case of failure.
 */
void MainWindow::load_volume(const QString& path)
{
    try {
        ui->canvas->setVolume(path);

        // Update the UI
        auto range = ui->canvas->getRange();
        ui->threshold_spinbox->setMinimum(range.first);
        ui->threshold_spinbox->setMaximum(range.second);
        ui->threshold_slider->valueChanged(ui->threshold_slider->value());
    }
    catch (std::runtime_error& e) {
        QMessageBox::warning(this, tr("Error"), tr("Cannot load volume ") + path + ": " + e.what());
    }
}

/*!
 * \brief Set the ray marching step lenght.
 * \param arg1 Step length, as a fraction of the ray length.
 */
void MainWindow::on_stepLength_valueChanged(double arg1)
{
    ui->canvas->setStepLength(static_cast<GLfloat>(arg1));
}

/*!
 * \brief Load a volume from file.
 */
void MainWindow::on_loadVolume_clicked()
{
    QString path = QFileDialog::getOpenFileName(this, tr("Open volume"), ".", tr("VTK images (*.vtk)"));
    if (!path.isNull()) {
        load_volume(path);
    }
}

/*!
 * \brief MainWindow::on_threshold_spinbox_valueChanged
 * \param arg1 Threshold in image intensity value.
 *
 * The spinbox and the slider are mutually linked, so when the value is changed in one of them,
 * the change is reflected on the other.
 *
 * The spinbox holds the threshold in image intensity value, while the slider holds a percentage.
 */
void MainWindow::on_threshold_spinbox_valueChanged(double arg1)
{
    ui->canvas->setThreshold(arg1);

    QSignalBlocker(ui->threshold_slider);
    auto range = ui->threshold_spinbox->maximum() - ui->threshold_spinbox->minimum();
    ui->threshold_slider->setValue(100 * (arg1 - ui->threshold_spinbox->minimum()) / range);
}

/*!
 * \brief MainWindow::on_threshold_slider_valueChanged
 * \param value Threshold in percentage.
 *
 * \sa MainWindow::on_threshold_spinbox_valueChanged
 */
void MainWindow::on_threshold_slider_valueChanged(int value)
{
    auto range = ui->threshold_spinbox->maximum() - ui->threshold_spinbox->minimum();
    auto threshold = ui->threshold_spinbox->minimum() + value / 100.0 * range;

    ui->canvas->setThreshold(threshold);

    QSignalBlocker(ui->threshold_spinbox);
    ui->threshold_spinbox->setValue(threshold);
}

/*!
 * \brief Set the visualisation mode.
 * \param arg1 Name of the visualisation mode.
 */
void MainWindow::on_mode_currentTextChanged(const QString &mode)
{
    ui->canvas->setMode(mode);

    if ("Isosurface" == mode) {
        ui->threshold_slider->setEnabled(true);
        ui->threshold_spinbox->setEnabled(true);
        ui->threshold_label->setEnabled(true);
    }
    else {
        ui->threshold_slider->setDisabled(true);
        ui->threshold_spinbox->setDisabled(true);
        ui->threshold_label->setDisabled(true);
    }
}


/*!
 * \brief Open a dialog to choose the background colour.
 */
void MainWindow::on_background_clicked()
{
    const QColor colour = QColorDialog::getColor(ui->canvas->getBackground(), this, "Select background colour");

    if (colour.isValid()) {
        ui->canvas->setBackground(colour);
    }
}
