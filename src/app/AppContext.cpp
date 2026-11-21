#include "AppContext.h"

#include "log/LogTableModel.h"
#include "map/MapLayerManager.h"
#include "log/Logger.h"

#include <QCoreApplication>
#include <QDir>
#include <QStandardPaths>

AppContext::AppContext(QObject* parent)
    : QObject(parent)
    , m_logger(new Logger(this))
    , m_logModel(new LogTableModel(LogTableModel::DefaultMaxRows, this))
    , m_mapLayers(new MapLayerManager(this))
{
    connect(m_logger, &Logger::entryAdded, m_logModel, &LogTableModel::append);
    m_logger->installQtMessageHandler();

    // 일자별 로그 파일: <AppLocalData>/logs/yyyyMMdd.log
    const QString logDir = QDir(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation))
                               .filePath(QStringLiteral("logs"));
    const bool fileOk = m_logger->setFileLoggingEnabled(true, logDir);
    m_logger->info(QStringLiteral("%1 v%2 시작 (Qt %3)")
                       .arg(QCoreApplication::applicationName(), QCoreApplication::applicationVersion(),
                            QString::fromLatin1(qVersion())),
                   QStringLiteral("App"));
    if (fileOk)
        m_logger->info(QStringLiteral("로그 파일: %1").arg(QDir::toNativeSeparators(m_logger->currentLogFilePath())),
                       QStringLiteral("App"));
    else
        m_logger->error(QStringLiteral("로그 파일을 열 수 없습니다: %1").arg(QDir::toNativeSeparators(logDir)),
                        QStringLiteral("App"));

    m_mapLayers->loadDefaultLayers(m_projection, m_logger);
    m_mapLayers->restoreVisibility();
}

AppContext::~AppContext()
{
    Logger::uninstallQtMessageHandler();
}
