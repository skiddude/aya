


#include "ManageEmulationDeviceDialog.hpp"

// Qt Headers
#include <QApplication>
#include <QGuiApplication>
#include <QScreen>
#include <QDir>
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>

// Roblox Studio Headers
#include "AddEmulationDeviceDialog.hpp"
#include "AuthoringSettings.hpp"
#include "RobloxApplicationManager.hpp"
#include "RobloxDocManager.hpp"
#include "RobloxIDEDoc.hpp"
#include "RobloxMainWindow.hpp"
#include "StudioDeviceEmulator.hpp"
#include "UpdateUIManager.hpp"

ManageEmulationDeviceDialog::ManageEmulationDeviceDialog(QWidget* Parent)
    : QDialog(Parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint)
{
    setAttribute(Qt::WA_DeleteOnClose, false);

    m_UI.setupUi(this);
    this->setStyleSheet("background-color: white;");

    setFixedSize(size());
    setWindowFlags(Qt::MSWindowsFixedSizeDialogHint);

    rePopulateListWidget();

    connect(m_UI.plusButton, SIGNAL(clicked()), this, SLOT(onPlusButtonPressed()));
    connect(m_UI.minusButton, SIGNAL(clicked()), this, SLOT(onMinusButtonPressed()));
    connect(m_UI.okButton, SIGNAL(clicked()), this, SLOT(onCancel()));
}

void ManageEmulationDeviceDialog::rePopulateListWidget()
{
    m_UI.deviceListWidget->clear();


    QList<StudioDeviceEmulator::EmulationDevice> devices = StudioDeviceEmulator::Instance().getDeviceList();

    for (QList<StudioDeviceEmulator::EmulationDevice>::const_iterator iter = devices.begin(); iter != devices.end(); ++iter)
    {
        if (*iter == StudioDeviceEmulator::EmulationDevice("Default", 96, 0, 0, false))
            continue;

        QListWidgetItem* deviceItem = new QListWidgetItem(m_UI.deviceListWidget);
        deviceItem->setText((*iter).name);

        QString iconLocation = (*iter).mobile ? "EmulateMobile.png" : "EmulateComputer.png";
        deviceItem->setIcon(QIcon(QString(":/images/icons/Test/%1").arg(iconLocation)));

        QAction* action = StudioDeviceEmulator::Instance().getActionFromDevice(*iter);

        deviceItem->setData(Qt::UserRole, (int)(size_t)action);

        m_UI.deviceListWidget->addItem(deviceItem);
    }
}

void ManageEmulationDeviceDialog::onPlusButtonPressed()
{
    AddEmulationDeviceDialog dialog(NULL);
    dialog.exec();
    rePopulateListWidget();
}

void ManageEmulationDeviceDialog::onMinusButtonPressed()
{
    QList<QListWidgetItem*> selectedItems = m_UI.deviceListWidget->selectedItems();

    for (QList<QListWidgetItem*>::const_iterator iter = selectedItems.begin(); iter != selectedItems.end(); ++iter)
    {
        QAction* action = (QAction*)((*iter)->data(Qt::UserRole).toInt());
        StudioDeviceEmulator::Instance().removeDevice(action);
    }
    rePopulateListWidget();
}

void ManageEmulationDeviceDialog::closeEvent(QCloseEvent* event)
{
    QDialog::closeEvent(event);
}

void ManageEmulationDeviceDialog::onCancel()
{
    accept();
}
