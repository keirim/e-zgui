#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QMessageBox>
#include <QFileDialog>
#include <QMimeDatabase>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QProgressBar>
#include <QNetworkReply>
#include <QHttpMultiPart>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStyle>
#include <QUrlQuery>
#include <QDesktopServices>
#include <QClipboard>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QListWidget>
#include <QStatusBar>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_settings("E-Z Uploader", "Settings")
{
    setWindowTitle("E-Z Uploader");
    setMinimumSize(800, 600);
    
    // Load stylesheet
    QFile styleFile(":/styles/style.qss");
    styleFile.open(QFile::ReadOnly);
    setStyleSheet(styleFile.readAll());
    
    setupUi();
    
    // Initialize API key state
    m_apiKey = m_settings.value("api_key").toString();
    if (!m_apiKey.isEmpty()) {
        validateApiKey(m_apiKey);
    } else {
        updateUiForValidation(false);
        m_dropArea->setEnabled(false);
    }
}

void MainWindow::setupUi()
{
    auto* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    
    auto* mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setSpacing(20);
    mainLayout->setContentsMargins(30, 30, 30, 30);
    
    // Create menu bar
    auto* menuBar = new QMenuBar(this);
    auto* fileMenu = menuBar->addMenu("File");
    auto* settingsMenu = menuBar->addMenu("Settings");
    
    // Add actions to file menu
    m_clearHistoryAction = fileMenu->addAction("Clear Upload History");
    connect(m_clearHistoryAction, &QAction::triggered, this, &MainWindow::clearHistory);
    
    // Add actions to settings menu
    m_autoCopyAction = settingsMenu->addAction("Auto-Copy URL on Upload");
    m_autoCopyAction->setCheckable(true);
    m_autoCopyAction->setChecked(m_settings.value("auto_copy", true).toBool());
    connect(m_autoCopyAction, &QAction::triggered, [this](bool checked) {
        m_settings.setValue("auto_copy", checked);
    });
    
    setMenuBar(menuBar);
    
    // Create header
    auto* headerWidget = new QWidget(this);
    auto* headerLayout = new QHBoxLayout(headerWidget);
    headerLayout->setContentsMargins(0, 0, 0, 0);
    
    // Status label to show if API key is set
    m_statusLabel = new QLabel(this);
    m_statusLabel->setObjectName("statusLabel");
    headerLayout->addWidget(m_statusLabel);
    
    // Logout button (hidden by default)
    m_logoutButton = new QPushButton("Logout", this);
    m_logoutButton->setObjectName("logoutButton");
    m_logoutButton->hide();
    headerLayout->addWidget(m_logoutButton, 0, Qt::AlignRight);
    
    mainLayout->addWidget(headerWidget);
    
    // Create a container for the API key input (hidden by default)
    m_apiKeyWidget = new QWidget(this);
    auto* apiKeyLayout = new QHBoxLayout(m_apiKeyWidget);
    apiKeyLayout->setSpacing(10);
    apiKeyLayout->setContentsMargins(0, 0, 0, 0);
    
    m_apiKeyInput = new QLineEdit(m_apiKeyWidget);
    m_apiKeyInput->setPlaceholderText("Enter API Key");
    m_apiKeyInput->setEchoMode(QLineEdit::Password);
    apiKeyLayout->addWidget(m_apiKeyInput);
    
    m_saveButton = new QPushButton("Save", m_apiKeyWidget);
    m_saveButton->setFixedWidth(100);
    apiKeyLayout->addWidget(m_saveButton);
    
    mainLayout->addWidget(m_apiKeyWidget);
    m_apiKeyWidget->hide();  // Hidden by default
    
    // Main content area
    auto* contentWidget = new QWidget(this);
    auto* contentLayout = new QHBoxLayout(contentWidget);
    contentLayout->setSpacing(20);
    
    // Upload area (left side)
    auto* uploadWidget = new QWidget(this);
    auto* uploadLayout = new QVBoxLayout(uploadWidget);
    uploadLayout->setSpacing(10);
    
    m_dropArea = new QWidget(this);
    m_dropArea->setObjectName("dropArea");
    m_dropArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_dropArea->setMinimumSize(300, 300);
    
    auto* dropAreaLayout = new QVBoxLayout(m_dropArea);
    dropAreaLayout->setAlignment(Qt::AlignCenter);
    
    m_dropLabel = new QLabel("Drag and drop files here\nor", this);
    m_dropLabel->setAlignment(Qt::AlignCenter);
    dropAreaLayout->addWidget(m_dropLabel);
    
    m_selectButton = new QPushButton("Select Files", this);
    m_selectButton->setObjectName("selectButton");
    dropAreaLayout->addWidget(m_selectButton, 0, Qt::AlignCenter);
    
    uploadLayout->addWidget(m_dropArea);
    
    m_progressBar = new QProgressBar(this);
    m_progressBar->setObjectName("progressBar");
    m_progressBar->hide();
    uploadLayout->addWidget(m_progressBar);
    
    contentLayout->addWidget(uploadWidget);
    
    // Preview panel (right side)
    setupPreviewPanel();
    contentLayout->addWidget(m_previewPanel);
    
    mainLayout->addWidget(contentWidget);
    
    // Add history list
    m_historyList = new QListWidget(this);
    m_historyList->setMaximumHeight(150);
    m_historyList->setHidden(true);
    mainLayout->addWidget(m_historyList);
    
    connect(m_historyList, &QListWidget::itemDoubleClicked, this, &MainWindow::onHistoryItemDoubleClicked);
    
    // Load history
    loadHistory();
    
    // Connect signals
    connect(m_saveButton, &QPushButton::clicked, this, &MainWindow::saveApiKey);
    connect(m_logoutButton, &QPushButton::clicked, this, &MainWindow::logout);
    connect(m_selectButton, &QPushButton::clicked, this, &MainWindow::handleFileSelection);
    
    // Enable drag and drop
    setAcceptDrops(true);
}

void MainWindow::setupPreviewPanel()
{
    m_previewPanel = new QWidget(this);
    auto* previewLayout = new QVBoxLayout(m_previewPanel);
    previewLayout->setSpacing(10);
    
    // Preview image
    m_previewImage = new QLabel(this);
    m_previewImage->setMinimumSize(200, 200);
    m_previewImage->setMaximumSize(400, 400);
    m_previewImage->setAlignment(Qt::AlignCenter);
    m_previewImage->setStyleSheet("QLabel { background-color: #2b2b2b; border-radius: 5px; }");
    previewLayout->addWidget(m_previewImage);
    
    // File info
    auto* infoWidget = new QWidget(this);
    auto* infoLayout = new QHBoxLayout(infoWidget);
    infoLayout->setSpacing(10);
    
    m_fileNameLabel = new QLabel(this);
    m_fileNameLabel->setStyleSheet("QLabel { color: #e0e0e0; }");
    infoLayout->addWidget(m_fileNameLabel);
    
    m_fileSizeLabel = new QLabel(this);
    m_fileSizeLabel->setStyleSheet("QLabel { color: #808080; }");
    infoLayout->addWidget(m_fileSizeLabel);
    
    previewLayout->addWidget(infoWidget);
    
    // Action buttons
    auto* buttonWidget = new QWidget(this);
    auto* buttonLayout = new QHBoxLayout(buttonWidget);
    buttonLayout->setSpacing(10);
    
    m_copyUrlButton = new QPushButton("Copy URL", this);
    m_copyUrlButton->setIcon(QIcon::fromTheme("edit-copy"));
    m_copyUrlButton->setCursor(Qt::PointingHandCursor);
    buttonLayout->addWidget(m_copyUrlButton);
    
    m_openImageButton = new QPushButton("Open", this);
    m_openImageButton->setIcon(QIcon::fromTheme("document-open"));
    m_openImageButton->setCursor(Qt::PointingHandCursor);
    buttonLayout->addWidget(m_openImageButton);
    
    m_deleteButton = new QPushButton("Delete", this);
    m_deleteButton->setIcon(QIcon::fromTheme("edit-delete"));
    m_deleteButton->setCursor(Qt::PointingHandCursor);
    buttonLayout->addWidget(m_deleteButton);
    
    previewLayout->addWidget(buttonWidget);
    
    // Connect button signals
    connect(m_copyUrlButton, &QPushButton::clicked, this, &MainWindow::copyUrl);
    connect(m_openImageButton, &QPushButton::clicked, this, &MainWindow::openImageUrl);
    connect(m_deleteButton, &QPushButton::clicked, this, &MainWindow::openDeleteUrl);
    
    // Initially hide the panel
    m_previewPanel->hide();
}

void MainWindow::copyUrl()
{
    if (!m_currentImageUrl.isEmpty()) {
        QClipboard* clipboard = QGuiApplication::clipboard();
        clipboard->setText(m_currentImageUrl);
        statusBar()->showMessage("URL copied to clipboard!", 3000);
    }
}

void MainWindow::openImageUrl()
{
    if (!m_currentImageUrl.isEmpty()) {
        QDesktopServices::openUrl(QUrl(m_currentImageUrl));
    }
}

void MainWindow::openDeleteUrl()
{
    if (!m_currentDeleteUrl.isEmpty()) {
        QDesktopServices::openUrl(QUrl(m_currentDeleteUrl));
    }
}

void MainWindow::updatePreviewPanel(const QString& imageUrl, const QString& rawUrl, const QString& deleteUrl)
{
    // Store URLs
    m_currentImageUrl = imageUrl;
    m_currentRawUrl = rawUrl;
    m_currentDeleteUrl = deleteUrl;

    // Update file info from the current upload
    if (m_currentUpload) {
        QString filePath = m_currentUpload->property("filePath").toString();
        QFileInfo fileInfo(filePath);
        m_fileNameLabel->setText(fileInfo.fileName());
        QString size = QString::number(fileInfo.size() / 1024.0 / 1024.0, 'f', 2) + " MB";
        m_fileSizeLabel->setText(size);
    }

    // Show preview panel
    m_previewPanel->show();

    // Update buttons
    m_copyUrlButton->setEnabled(true);
    m_openImageButton->setEnabled(true);
    m_deleteButton->setEnabled(true);

    // If it's an image URL, download and show preview
    if (imageUrl.contains(".png") || imageUrl.contains(".jpg") || 
        imageUrl.contains(".jpeg") || imageUrl.contains(".gif") ||
        imageUrl.contains(".webp")) {
        downloadPreviewImage();
    } else {
        // Use default icon for non-image files
        QPixmap defaultIcon = QIcon::fromTheme("text-x-generic").pixmap(64, 64);
        m_previewImage->setPixmap(defaultIcon);
    }
}

void MainWindow::downloadPreviewImage()
{
    if (m_currentImageUrl.isEmpty()) return;
    
    QNetworkRequest request;
    request.setUrl(QUrl(m_currentImageUrl));
    QNetworkReply* reply = m_networkManager.get(request);
    
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray data = reply->readAll();
            QPixmap pixmap;
            if (pixmap.loadFromData(data)) {
                // Scale pixmap to fit the label while maintaining aspect ratio
                pixmap = pixmap.scaled(m_previewImage->size(), 
                                     Qt::KeepAspectRatio, 
                                     Qt::SmoothTransformation);
                m_previewImage->setPixmap(pixmap);
            }
        } else {
            // Show error icon if preview fails
            QPixmap errorIcon = QIcon::fromTheme("dialog-error").pixmap(64, 64);
            m_previewImage->setPixmap(errorIcon);
        }
    });
}

QString MainWindow::getMimeType(const QString& filePath)
{
    QString extension = QFileInfo(filePath).suffix().toLower();
    
    // Image types
    if (extension == "jpg" || extension == "jpeg") return "image/jpeg";
    if (extension == "png") return "image/png";
    if (extension == "gif") return "image/gif";
    if (extension == "webp") return "image/webp";
    if (extension == "bmp") return "image/bmp";
    
    // Video types
    if (extension == "mp4") return "video/mp4";
    if (extension == "webm") return "video/webm";
    if (extension == "avi") return "video/x-msvideo";
    
    // Audio types
    if (extension == "mp3") return "audio/mpeg";
    if (extension == "wav") return "audio/wav";
    if (extension == "ogg") return "audio/ogg";
    
    // Document types
    if (extension == "pdf") return "application/pdf";
    if (extension == "txt") return "text/plain";
    
    // Default
    return "application/octet-stream";
}

void MainWindow::validateApiKey(const QString& key)
{
    QUrl url(QString("https://api.e-z.gg/paste/config"));
    QUrlQuery query;
    query.addQueryItem("key", key);
    url.setQuery(query);
    
    QNetworkRequest request(url);
    QNetworkReply* reply = m_networkManager.get(request);
    
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        validateApiKeyResponse(reply);
    });
}

void MainWindow::validateApiKeyResponse(QNetworkReply* reply)
{
    reply->deleteLater();
    
    if (reply->error() == QNetworkReply::NoError) {
        int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (statusCode == 200) {
            // Only update the stored key if we're validating a new key
            if (!m_apiKeyInput->text().isEmpty()) {
                m_apiKey = m_apiKeyInput->text().trimmed();
                m_settings.setValue("api_key", m_apiKey);
                m_apiKeyInput->clear();
            }
            updateUiForValidation(true, "API Key validated successfully!");
            m_dropArea->setEnabled(true);
        } else {
            m_apiKey.clear();
            m_settings.remove("api_key");
            updateUiForValidation(false, "Invalid API Key");
            m_dropArea->setEnabled(false);
        }
    } else {
        m_apiKey.clear();
        m_settings.remove("api_key");
        updateUiForValidation(false, "Failed to validate API Key: " + reply->errorString());
        m_dropArea->setEnabled(false);
    }
}

void MainWindow::updateUiForValidation(bool isValid, const QString& message)
{
    if (isValid) {
        m_statusLabel->setText("✓ API Key Configured");
        m_statusLabel->setStyleSheet("color: #00ff00;");
        m_apiKeyWidget->hide();
        m_dropArea->setEnabled(true);
        m_logoutButton->show();
        if (!message.isEmpty()) {
            QMessageBox::information(this, "Success", message);
        }
    } else {
        m_statusLabel->setText("✗ API Key Not Configured");
        m_statusLabel->setStyleSheet("color: #ff4444;");
        m_apiKeyWidget->show();
        m_dropArea->setEnabled(false);
        m_logoutButton->hide();
        if (!message.isEmpty()) {
            QMessageBox::warning(this, "Validation Error", message);
        }
    }
}

void MainWindow::logout()
{
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "Confirm Logout",
        "Are you sure you want to logout? This will remove your API key.",
        QMessageBox::Yes | QMessageBox::No
    );
    
    if (reply == QMessageBox::Yes) {
        m_settings.remove("api_key");
        m_apiKey.clear();
        updateUiForValidation(false);
    }
}

void MainWindow::loadApiKey()
{
    m_apiKey = m_settings.value("api_key").toString();
    if (!m_apiKey.isEmpty()) {
        validateApiKey(m_apiKey);
    } else {
        updateUiForValidation(false);
        m_dropArea->setEnabled(false);
    }
}

void MainWindow::saveApiKey()
{
    QString newKey = m_apiKeyInput->text().trimmed();
    if (newKey.isEmpty()) {
        QMessageBox::warning(this, "Invalid API Key", "Please enter a valid API key.");
        return;
    }
    
    validateApiKey(newKey);
}

void MainWindow::checkAndPromptApiKey()
{
    if (!hasValidApiKey()) {
        QMessageBox::warning(this, "API Key Required",
                           "Please enter a valid API key before uploading files.");
        m_apiKeyInput->setFocus();
    }
}

bool MainWindow::hasValidApiKey() const
{
    return !m_apiKey.isEmpty();
}

void MainWindow::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasUrls() && hasValidApiKey()) {
        event->acceptProposedAction();
        updateDropAreaStyle(true);
    }
}

void MainWindow::dragLeaveEvent(QDragLeaveEvent* event)
{
    updateDropAreaStyle(false);
    event->accept();
}

void MainWindow::dropEvent(QDropEvent* event)
{
    updateDropAreaStyle(false);
    
    if (!hasValidApiKey()) {
        QMessageBox::warning(this, "Error", "Please configure your API key first.");
        return;
    }
    
    const QList<QUrl> urls = event->mimeData()->urls();
    if (urls.isEmpty()) return;
    
    QString filePath = urls.first().toLocalFile();
    if (!isValidFileType(filePath)) {
        QMessageBox::warning(this, "Invalid File", "Please select a valid file type (image/video/audio/application).");
        return;
    }
    
    if (!isFileSizeValid(filePath)) {
        QMessageBox::warning(this, "Invalid File Size", "File size must be less than 100MB.");
        return;
    }
    
    uploadFile(filePath);
    event->acceptProposedAction();
}

void MainWindow::handleFileSelection()
{
    if (!hasValidApiKey()) {
        QMessageBox::warning(this, "Error", "Please configure your API key first.");
        return;
    }
    
    QString filePath = QFileDialog::getOpenFileName(this, "Select File",
                                                  QDir::homePath(),  // Start in home directory
                                                  "All Files (*.*)");
    if (filePath.isEmpty()) return;
    
    if (!isValidFileType(filePath)) {
        QMessageBox::warning(this, "Invalid File", "Please select a valid file type (image/video/audio/application).");
        return;
    }
    
    if (!isFileSizeValid(filePath)) {
        QMessageBox::warning(this, "Invalid File Size", "File size must be less than 100MB.");
        return;
    }
    
    uploadFile(filePath);
}

bool MainWindow::isValidFileType(const QString& filePath)
{
    QMimeDatabase db;
    QString mimeType = db.mimeTypeForFile(filePath).name();
    return mimeType.startsWith("image/") ||
           mimeType.startsWith("video/") ||
           mimeType.startsWith("audio/") ||
           mimeType.startsWith("application/");
}

bool MainWindow::isFileSizeValid(const QString& filePath)
{
    QFileInfo fileInfo(filePath);
    constexpr qint64 maxSize = 100 * 1024 * 1024; // 100MB in bytes
    return fileInfo.size() <= maxSize;
}

void MainWindow::updateDropAreaStyle(bool isDragOver)
{
    if (isDragOver) {
        m_dropArea->setProperty("class", "dragOver");
    } else {
        m_dropArea->setProperty("class", "");
    }
    m_dropArea->style()->unpolish(m_dropArea);
    m_dropArea->style()->polish(m_dropArea);
}

void MainWindow::uploadFile(const QString& filePath)
{
    if (m_apiKey.isEmpty()) {
        QMessageBox::warning(this, "Error", "Please enter a valid API key first.");
        updateUiForValidation(false);
        return;
    }
    
    QFile* file = new QFile(filePath);
    if (!file->open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, "Error", "Failed to open file: " + file->errorString());
        file->deleteLater();
        return;
    }
    
    QHttpMultiPart* multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    
    // Add file part with proper MIME type
    QHttpPart filePart;
    QString mimeType = getMimeType(filePath);
    filePart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant(mimeType));
    
    filePart.setHeader(QNetworkRequest::ContentDispositionHeader, 
                      QVariant(QString("form-data; name=\"file\"; filename=\"%1\"")
                              .arg(QFileInfo(filePath).fileName())));
    filePart.setBodyDevice(file);
    multiPart->append(filePart);
    
    // Create and send request
    QUrl url("https://api.e-z.host/files");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader,
                     QString("multipart/form-data; boundary=%1").arg(multiPart->boundary().data()));
    request.setRawHeader("key", m_apiKey.toUtf8());
    
    m_currentUpload = m_networkManager.post(request, multiPart);
    m_currentUpload->setProperty("filePath", filePath);
    multiPart->setParent(m_currentUpload);
    file->setParent(m_currentUpload);
    
    connect(m_currentUpload, &QNetworkReply::uploadProgress,
            this, &MainWindow::uploadProgress);
    connect(m_currentUpload, &QNetworkReply::finished,
            this, &MainWindow::uploadFinished);
    
    m_progressBar->setValue(0);
    m_progressBar->show();
    m_dropArea->setEnabled(false);
}

void MainWindow::uploadProgress(qint64 bytesSent, qint64 bytesTotal)
{
    if (bytesTotal > 0) {
        int progress = static_cast<int>((bytesSent * 100) / bytesTotal);
        m_progressBar->setValue(progress);
    }
}

void MainWindow::uploadFinished()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    
    m_progressBar->hide();
    m_dropArea->setEnabled(true);
    
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray response = reply->readAll();
        showUploadResult(response);
        
        // Auto-copy URL if enabled
        if (m_autoCopyAction && m_autoCopyAction->isChecked()) {
            QClipboard* clipboard = QGuiApplication::clipboard();
            clipboard->setText(m_currentImageUrl);
            statusBar()->showMessage("URL copied to clipboard!", 3000);
        }
    } else {
        QString errorMsg = reply->errorString();
        if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).isValid()) {
            int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            errorMsg = QString("Server returned error %1: %2").arg(statusCode).arg(errorMsg);
        }
        QMessageBox::critical(this, "Upload Error", 
                            "Failed to upload file: " + errorMsg + 
                            "\nPlease check your internet connection and API key.");
    }
    
    reply->deleteLater();
}

void MainWindow::showUploadResult(const QByteArray& response)
{
    QJsonDocument doc = QJsonDocument::fromJson(response);
    if (!doc.isObject()) {
        QMessageBox::warning(this, "Error", "Invalid response from server");
        return;
    }
    
    QJsonObject obj = doc.object();
    if (!obj["success"].toBool()) {
        QString message = obj["message"].toString("Unknown error");
        QMessageBox::warning(this, "Upload Failed", message);
        return;
    }
    
    QJsonObject data = obj["data"].toObject();
    QString imageUrl = data["url"].toString();
    QString rawUrl = data["raw"].toString();
    QString deleteUrl = data["delete"].toString();
    
    updatePreviewPanel(imageUrl, rawUrl, deleteUrl);
    
    // Add to history
    QString fileName = QFileInfo(m_currentUpload->property("filePath").toString()).fileName();
    addToHistory(fileName, imageUrl, rawUrl, deleteUrl);
    
    statusBar()->showMessage("File uploaded successfully!", 3000);
}

void MainWindow::clearPreviewPanel()
{
    m_currentImageUrl.clear();
    m_currentRawUrl.clear();
    m_currentDeleteUrl.clear();
    m_previewImage->clear();
    m_fileNameLabel->clear();
    m_fileSizeLabel->clear();
    m_previewPanel->hide();
}

void MainWindow::loadHistory()
{
    QStringList history = m_settings.value("upload_history").toStringList();
    for (const QString& entry : history) {
        QJsonDocument doc = QJsonDocument::fromJson(entry.toUtf8());
        if (doc.isObject()) {
            QJsonObject obj = doc.object();
            addToHistory(obj["name"].toString(), 
                        obj["image"].toString(),
                        obj["raw"].toString(),
                        obj["delete"].toString());
        }
    }
    
    if (!m_historyList->count()) {
        m_historyList->setHidden(true);
    }
}

void MainWindow::addToHistory(const QString& fileName, const QString& imageUrl, 
                            const QString& rawUrl, const QString& deleteUrl)
{
    // Create history item
    auto* item = new QListWidgetItem(fileName);
    item->setData(Qt::UserRole, QStringList({imageUrl, rawUrl, deleteUrl}));
    
    // Add to list and show if hidden
    m_historyList->insertItem(0, item);
    m_historyList->setHidden(false);
    
    // Save to settings
    QJsonObject entry;
    entry["name"] = fileName;
    entry["image"] = imageUrl;
    entry["raw"] = rawUrl;
    entry["delete"] = deleteUrl;
    
    QStringList history = m_settings.value("upload_history").toStringList();
    history.prepend(QJsonDocument(entry).toJson(QJsonDocument::Compact));
    
    // Keep only last 20 entries
    while (history.size() > 20) {
        history.removeLast();
    }
    
    m_settings.setValue("upload_history", history);
}

void MainWindow::clearHistory()
{
    m_historyList->clear();
    m_historyList->setHidden(true);
    m_settings.remove("upload_history");
}

void MainWindow::onHistoryItemDoubleClicked(QListWidgetItem* item)
{
    QStringList urls = item->data(Qt::UserRole).toStringList();
    if (urls.size() == 3) {
        updatePreviewPanel(urls[0], urls[1], urls[2]);
    }
}
