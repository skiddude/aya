

#pragma once

// Qt Headers
#include <QString>
#include <QMutex>
#include <QObject>

// Roblox Headers
#include "Xml/Serializer.hpp"
#include "Xml/XmlElement.hpp"

namespace Aya
{
class DataModel;
}

/**
 * Serializes a DataDodel to disk either synchronously or asynchronously.
 *  Displays progress bars.
 */
class StudioSerializerHelper : public QObject
{
public:
    static bool saveAs(const QString& fileName, const QString& label, bool asynchronous, bool useBinaryFormat, Aya::DataModel* pDataModel,
        QString& outErrorMessage, bool publishAssets, char* thumbnailBuffer = nullptr, size_t thumbnailSize = NULL);

    static bool saveDebuggerData(const QString& debuggerFileName);

    static QByteArray getDataModelHash(Aya::DataModel* pDataModel);

    static bool saveAsIfModified(const QByteArray& hashToCompareWith, Aya::DataModel* pDataModel, const QString& fileName);

private:
    static void generateXML(Aya::DataModel* pDataModel, XmlElement* xml);

    static void serializeXML(const QString& fileName, XmlElement* xml, bool* result, QString* outErrorMessage);

    static void serializeXMLAsynchronous(const QString& fileName, XmlElement* xml);

    static void serializeBinary(const QString& fileName, Aya::DataModel* pDataModel, bool* outResult, QString* outErrorMessage);

    static QMutex m_Mutex;
};
