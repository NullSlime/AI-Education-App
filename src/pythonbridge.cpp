#include "pythonbridge.h"
#include <QJsonDocument>
#include <QDir>
#include <QStandardPaths>
#include <QDebug>

PythonBridge::PythonBridge(QObject *parent)
    : QObject(parent)
    , process(new QProcess(this))
{
    pythonPath = findPythonExecutable();
    
    connect(process, &QProcess::readyReadStandardOutput, 
            this, &PythonBridge::onReadyReadStandardOutput);
    connect(process, &QProcess::readyReadStandardError, 
            this, &PythonBridge::onReadyReadStandardError);
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &PythonBridge::onProcessFinished);
}

PythonBridge::~PythonBridge()
{
    if (process->state() == QProcess::Running) {
        process->terminate();
        process->waitForFinished(3000);
    }
}

QString PythonBridge::findPythonExecutable()
{
    // Try common Python 3.12 locations
    QStringList candidates;
    
#ifdef Q_OS_WIN
    candidates << "python3.12.exe" << "python.exe" << "py.exe";
#else
    candidates << "python3.12" << "python3" << "python";
#endif
    
    for (const QString &candidate : candidates) {
        QString path = QStandardPaths::findExecutable(candidate);
        if (!path.isEmpty()) {
            qDebug() << "Found Python:" << path;
            return path;
        }
    }
    
    return "python3"; // Fallback
}

QString PythonBridge::getPythonScriptPath(const QString &scriptName)
{
    QString scriptDir = QDir::currentPath() + "/python";
    return scriptDir + "/" + scriptName;
}

void PythonBridge::runScript(const QString &scriptName, const QJsonObject &args)
{
    if (process->state() == QProcess::Running) {
        qWarning() << "Process already running";
        return;
    }
    
    QString scriptPath = getPythonScriptPath(scriptName);
    QJsonDocument doc(args);
    QString jsonArgs = QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
    
    qDebug() << "Running:" << pythonPath << scriptPath;
    qDebug() << "Args:" << jsonArgs;
    
    QStringList arguments;
    arguments << scriptPath << jsonArgs;
    
    process->start(pythonPath, arguments);
    
    if (!process->waitForStarted()) {
        emit errorReceived("Pythonプロセスの起動に失敗しました: " + process->errorString());
    }
}

void PythonBridge::terminate()
{
    if (process->state() == QProcess::Running) {
        process->terminate();
    }
}

bool PythonBridge::isRunning() const
{
    return process->state() == QProcess::Running;
}

void PythonBridge::onReadyReadStandardOutput()
{
    QString output = QString::fromUtf8(process->readAllStandardOutput()).trimmed();
    if (!output.isEmpty()) {
        qDebug() << "Python output:" << output;
        emit outputReceived(output);
    }
}

void PythonBridge::onReadyReadStandardError()
{
    QString error = QString::fromUtf8(process->readAllStandardError()).trimmed();
    if (!error.isEmpty()) {
        qWarning() << "Python error:" << error;
        emit errorReceived(error);
    }
}

void PythonBridge::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    qDebug() << "Process finished with code:" << exitCode;
    
    if (exitStatus == QProcess::CrashExit) {
        emit errorReceived("Pythonプロセスがクラッシュしました");
    }
    
    emit processFinished(exitCode);
}