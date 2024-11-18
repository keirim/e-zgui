#pragma once

#include <QMainWindow>
#include <QSettings>
#include <QNetworkAccessManager>
#include <QMimeData>
#include <QUrlQuery>
#include <QMessageBox>
#include <QNetworkReply>
#include <QListWidget>
#include <QListWidgetItem>
#include <QAction>
#include <QStatusBar>

class QLineEdit;
class QPushButton;
class QLabel;
class QProgressBar;
class QWidget;

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
    void copyUrl();
    void openDeleteUrl();
    void openImageUrl();
    void downloadPreviewImage();
    void previewImageDownloaded(QNetworkReply* reply);

private:
    void setupUi();
    void createApiKeyPrompt();
    void loadHistory();
    void addToHistory(const QString& fileName, const QString& imageUrl, 
                     const QString& rawUrl, const QString& deleteUrl);
    void clearHistory();
    void onHistoryItemDoubleClicked(QListWidgetItem* item);
    bool hasValidApiKey() const;
    void loadApiKey();
    void updateDropAreaStyle(bool isDragOver = false);
    void showUploadResult(const QByteArray& response);
    void validateApiKey(const QString& key);
    void updateUiForValidation(bool isValid, const QString& message = QString());
    void setupPreviewPanel();
    void updatePreviewPanel(const QString& imageUrl, const QString& rawUrl, const QString& deleteUrl);
    void clearPreviewPanel();
    bool isImageFile(const QString& filePath) const;
    QString getMimeType(const QString& filePath);

    static bool isValidFileType(const QString& filePath);
    static bool isFileSizeValid(const QString& filePath);

    QSettings m_settings;
    QString m_apiKey;
    QNetworkAccessManager m_networkManager;
    QNetworkReply* m_currentUpload = nullptr;

    // Upload URLs
    QString m_currentImageUrl;
    QString m_currentRawUrl;
    QString m_currentDeleteUrl;

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

    // Preview Panel Elements
    QWidget* m_previewPanel = nullptr;
    QLabel* m_previewImage = nullptr;
    QLabel* m_fileNameLabel = nullptr;
    QLabel* m_fileSizeLabel = nullptr;
    QPushButton* m_copyUrlButton = nullptr;
    QPushButton* m_openImageButton = nullptr;
    QPushButton* m_deleteButton = nullptr;

    // History and Settings
    QListWidget* m_historyList;
    QAction* m_clearHistoryAction;
    QAction* m_autoCopyAction;
};
