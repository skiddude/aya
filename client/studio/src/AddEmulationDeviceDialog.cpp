
#include "AddEmulationDeviceDialog.hpp"

// Qt Headers
#include <QSettings>
#include <QDir>
#include <QFileDialog>
#include <QMessageBox>
#include <QApplication>
#include <QGuiApplication>
#include <QScreen>

// Roblox Studio Headers
#include "AuthoringSettings.hpp"
#include "RobloxApplicationManager.hpp"
#include "RobloxDocManager.hpp"
#include "RobloxIDEDoc.hpp"
#include "RobloxMainWindow.hpp"
#include "StudioDeviceEmulator.hpp"
#include "UpdateUIManager.hpp"

AddEmulationDeviceDialog::AddEmulationDeviceDialog(QWidget* Parent)
    : QDialog(Parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint)
{
    setAttribute(Qt::WA_DeleteOnClose, false);

    m_UI.setupUi(this);
    this->setStyleSheet("background-color: white;");

    setFixedSize(size());
    setWindowFlags(Qt::MSWindowsFixedSizeDialogHint);

    m_UI.dpiValue->setText(QString("%1 DPI").arg(QGuiApplication::primaryScreen()->physicalDotsPerInchX()));

    connect(m_UI.cancelButton, SIGNAL(clicked()), this, SLOT(onCancel()));
    connect(m_UI.okButton, SIGNAL(clicked()), this, SLOT(submitDevice()));
}

/**
 * Callback for closing the dialog.
 *
 */
void AddEmulationDeviceDialog::closeEvent(QCloseEvent* event)
{
    QDialog::closeEvent(event);
}

void AddEmulationDeviceDialog::onCancel()
{
    accept();
}

void AddEmulationDeviceDialog::submitDevice()
{
    StudioDeviceEmulator::EmulationDevice device(m_UI.lineEditName->text(), QGuiApplication::primaryScreen()->physicalDotsPerInchX(), m_UI.spinBoxWidth->value(),
        m_UI.spinBoxHeight->value(), m_UI.checkBoxMobile->checkState());

    StudioDeviceEmulator::Instance().addDevice(device);

    accept();
}
