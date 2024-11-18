#pragma once

#include <QMainWindow>
#include <QSettings>
#include <QNetworkAccessManager>
#include <QMimeData>

class QLineEdit;
class QPushButton;
class QLabel;
class QProgressBar;
class QNetworkReply;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override = default;

protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragLeaveEvent(QDragLeaveEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private slots:
    void saveApiKey();
    void logout();
    void checkAndPromptApiKey();
    void handleFileSelection();
    void uploadFile(const QString& filePath);
    void uploadProgress(qint64 bytesSent, qint64 bytesTotal);
    void uploadFinished();
    void validateApiKeyResponse(QNetworkReply* reply);

private:
    void setupUi();
    void createApiKeyPrompt();
    bool hasValidApiKey() const;
    void loadApiKey();
    void updateDropAreaStyle(bool isDragOver = false);
    void showUploadResult(const QByteArray& response);
    void validateApiKey(const QString& key);
    void updateUiForValidation(bool isValid, const QString& message = QString());

    static bool isValidFileType(const QString& filePath);
    static bool isFileSizeValid(const QString& filePath);

    QSettings m_settings;
    QString m_apiKey;
    QNetworkAccessManager m_networkManager;
    QNetworkReply* m_currentUpload = nullptr;

    // UI Elements
    QLineEdit* m_apiKeyInput = nullptr;
    QPushButton* m_saveButton = nullptr;
    QPushButton* m_logoutButton = nullptr;
    QWidget* m_apiKeyWidget = nullptr;
    QLabel* m_statusLabel = nullptr;
    QWidget* m_dropArea = nullptr;
    QLabel* m_dropLabel = nullptr;
    QPushButton* m_selectButton = nullptr;
    QProgressBar* m_progressBar = nullptr;
};
