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

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_settings("E-Z Uploader", "Settings")
{
    setWindowTitle("E-Z Uploader");
    resize(800, 600);
    
    // Load and apply stylesheet
    QFile styleFile(":/styles/style.qss");
    styleFile.open(QFile::ReadOnly);
    setStyleSheet(styleFile.readAll());
    
    setupUi();
    loadApiKey();
}

void MainWindow::setupUi()
{
    auto* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    
    auto* mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setSpacing(20);
    mainLayout->setContentsMargins(30, 30, 30, 30);
    
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
    m_previewPanel->setObjectName("previewPanel");
    m_previewPanel->setMinimumWidth(300);
    m_previewPanel->hide();
    
    auto* previewLayout = new QVBoxLayout(m_previewPanel);
    previewLayout->setSpacing(15);
    
    // Preview image
    m_previewImage = new QLabel(this);
    m_previewImage->setObjectName("previewImage");
    m_previewImage->setMinimumSize(280, 280);
    m_previewImage->setMaximumSize(280, 280);
    m_previewImage->setScaledContents(true);
    m_previewImage->setAlignment(Qt::AlignCenter);
    previewLayout->addWidget(m_previewImage, 0, Qt::AlignCenter);
    
    // File info
    m_fileNameLabel = new QLabel(this);
    m_fileNameLabel->setObjectName("fileNameLabel");
    m_fileNameLabel->setWordWrap(true);
    previewLayout->addWidget(m_fileNameLabel);
    
    m_fileSizeLabel = new QLabel(this);
    m_fileSizeLabel->setObjectName("fileSizeLabel");
    previewLayout->addWidget(m_fileSizeLabel);
    
    // Buttons
    auto* buttonLayout = new QVBoxLayout();
    buttonLayout->setSpacing(10);
    
    m_copyUrlButton = new QPushButton("Copy URL", this);
    m_copyUrlButton->setObjectName("copyUrlButton");
    buttonLayout->addWidget(m_copyUrlButton);
    
    m_openImageButton = new QPushButton("Open in Browser", this);
    m_openImageButton->setObjectName("openImageButton");
    buttonLayout->addWidget(m_openImageButton);
    
    m_deleteButton = new QPushButton("Delete File", this);
    m_deleteButton->setObjectName("deleteButton");
    buttonLayout->addWidget(m_deleteButton);
    
    previewLayout->addLayout(buttonLayout);
    
    // Connect signals
    connect(m_copyUrlButton, &QPushButton::clicked, this, &MainWindow::copyUrl);
    connect(m_openImageButton, &QPushButton::clicked, this, &MainWindow::openImageUrl);
    connect(m_deleteButton, &QPushButton::clicked, this, &MainWindow::openDeleteUrl);
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
            m_settings.setValue("api_key", m_apiKeyInput->text().trimmed());
            m_apiKey = m_apiKeyInput->text().trimmed();
            m_apiKeyInput->clear();
            updateUiForValidation(true, "API Key validated successfully!");
        } else {
            updateUiForValidation(false, "Invalid API Key");
        }
    } else {
        updateUiForValidation(false, "Failed to validate API Key: " + reply->errorString());
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
    if (hasValidApiKey()) {
        m_statusLabel->setText("✓ API Key Configured");
        m_statusLabel->setStyleSheet("color: #00ff00;");
        m_apiKeyWidget->hide();
        m_dropArea->setEnabled(true);
    } else {
        m_statusLabel->setText("✗ API Key Not Configured");
        m_statusLabel->setStyleSheet("color: #ff4444;");
        m_apiKeyWidget->show();
        m_dropArea->setEnabled(false);
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
    qDebug() << "Starting upload for file:" << filePath;
    
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Failed to open file:" << file.errorString();
        QMessageBox::warning(this, "Error", "Failed to open file: " + file.errorString());
        return;
    }
    
    qDebug() << "File opened successfully. Size:" << file.size() << "bytes";
    
    QHttpMultiPart* multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    
    // Add file part
    QHttpPart filePart;
    filePart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("application/octet-stream"));
    filePart.setHeader(QNetworkRequest::ContentDispositionHeader, 
                      QVariant(QString("form-data; name=\"file\"; filename=\"%1\"")
                              .arg(QFileInfo(filePath).fileName())));
    filePart.setBodyDevice(&file);
    multiPart->append(filePart);
    
    // Create and send request
    QUrl url("https://api.e-z.host/files");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader,
                     QString("multipart/form-data; boundary=%1").arg(multiPart->boundary().data()));
    request.setRawHeader("Key", m_apiKey.toUtf8());
    
    qDebug() << "Sending request to:" << url.toString();
    qDebug() << "Headers:";
    qDebug() << "- Content-Type:" << request.header(QNetworkRequest::ContentTypeHeader).toString();
    qDebug() << "- Key:" << m_apiKey.left(4) + "..." << "(truncated for security)";
    
    m_currentUpload = m_networkManager.post(request, multiPart);
    m_currentUpload->setProperty("filePath", filePath);
    multiPart->setParent(m_currentUpload);
    file.setParent(m_currentUpload);
    
    connect(m_currentUpload, &QNetworkReply::uploadProgress,
            this, &MainWindow::uploadProgress);
    connect(m_currentUpload, &QNetworkReply::finished,
            this, &MainWindow::uploadFinished);
    
    m_dropArea->setEnabled(false);
    m_progressBar->setValue(0);
    m_progressBar->show();
}

void MainWindow::uploadProgress(qint64 bytesSent, qint64 bytesTotal)
{
    if (bytesTotal > 0) {
        int progress = static_cast<int>((bytesSent * 100) / bytesTotal);
        m_progressBar->setValue(progress);
        qDebug() << "Upload progress:" << bytesSent << "/" << bytesTotal << "bytes (" << progress << "%)";
    }
}

void MainWindow::uploadFinished()
{
    m_progressBar->hide();
    m_selectButton->setEnabled(true);
    
    if (m_currentUpload->error() == QNetworkReply::NoError) {
        QByteArray response = m_currentUpload->readAll();
        qDebug() << "Upload completed successfully. Response:" << response;
        showUploadResult(response);
    } else {
        QString errorString = m_currentUpload->errorString();
        int statusCode = m_currentUpload->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        
        qDebug() << "Upload failed:";
        qDebug() << "- Status code:" << statusCode;
        qDebug() << "- Error:" << errorString;
        qDebug() << "- Raw response:" << m_currentUpload->readAll();
        
        QMessageBox::warning(this, "Upload Error",
                           QString("Failed to upload file (Status: %1)\nError: %2")
                           .arg(statusCode)
                           .arg(errorString));
    }
    
    m_currentUpload->deleteLater();
    m_currentUpload = nullptr;
}

void MainWindow::showUploadResult(const QByteArray& response)
{
    qDebug() << "Raw response:" << response;
    
    QJsonDocument doc = QJsonDocument::fromJson(response);
    QJsonObject obj = doc.object();
    
    qDebug() << "Parsed JSON:" << doc.toJson();
    
    if (obj.contains("success") && obj["success"].toBool()) {
        QString imageUrl = obj["imageUrl"].toString();
        QString rawUrl = obj["rawUrl"].toString();
        QString deleteUrl = obj["deletionUrl"].toString();
        
        qDebug() << "Extracted URLs:";
        qDebug() << "- Image URL:" << imageUrl;
        qDebug() << "- Raw URL:" << rawUrl;
        qDebug() << "- Delete URL:" << deleteUrl;
        
        updatePreviewPanel(imageUrl, rawUrl, deleteUrl);
    } else {
        QString error = obj["message"].toString("Upload failed");
        qDebug() << "Upload failed:" << error;
        QMessageBox::warning(this, "Error", error);
        clearPreviewPanel();
    }
    
    m_progressBar->hide();
    m_dropArea->setEnabled(true);
}

void MainWindow::updatePreviewPanel(const QString& imageUrl, const QString& rawUrl, const QString& deleteUrl)
{
    qDebug() << "Updating preview panel with URLs:";
    qDebug() << "- Image URL:" << imageUrl;
    qDebug() << "- Raw URL:" << rawUrl;
    qDebug() << "- Delete URL:" << deleteUrl;
    
    m_currentImageUrl = imageUrl;
    m_currentRawUrl = rawUrl;
    m_currentDeleteUrl = deleteUrl;
    
    if (!m_currentUpload) {
        qDebug() << "Error: m_currentUpload is null";
        return;
    }
    
    QString filePath = m_currentUpload->property("filePath").toString();
    qDebug() << "File path from property:" << filePath;
    
    if (filePath.isEmpty()) {
        qDebug() << "Error: File path is empty";
        return;
    }
    
    QFileInfo fileInfo(filePath);
    qDebug() << "File info:";
    qDebug() << "- Name:" << fileInfo.fileName();
    qDebug() << "- Size:" << fileInfo.size() << "bytes";
    qDebug() << "- Exists:" << fileInfo.exists();
    
    m_fileNameLabel->setText(fileInfo.fileName());
    QString size = QString::number(fileInfo.size() / 1024.0 / 1024.0, 'f', 2) + " MB";
    m_fileSizeLabel->setText(size);
    
    if (isImageFile(filePath)) {
        qDebug() << "Starting preview download for image file";
        downloadPreviewImage();
    } else {
        qDebug() << "Using default icon for non-image file";
        QPixmap defaultIcon = QIcon::fromTheme("text-x-generic").pixmap(64, 64);
        m_previewImage->setPixmap(defaultIcon);
    }
    
    m_previewPanel->show();
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

void MainWindow::copyUrl()
{
    QClipboard* clipboard = QGuiApplication::clipboard();
    clipboard->setText(m_currentImageUrl);
    
    QMessageBox::information(this, "Success", "URL copied to clipboard!");
}

void MainWindow::openImageUrl()
{
    QDesktopServices::openUrl(QUrl(m_currentImageUrl));
}

void MainWindow::openDeleteUrl()
{
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "Confirm Delete",
        "Are you sure you want to delete this file? This action cannot be undone.",
        QMessageBox::Yes | QMessageBox::No
    );
    
    if (reply == QMessageBox::Yes) {
        QDesktopServices::openUrl(QUrl(m_currentDeleteUrl));
        clearPreviewPanel();
    }
}

void MainWindow::downloadPreviewImage()
{
    QNetworkRequest request{QUrl{m_currentImageUrl}};  // Fixed initialization using braces
    QNetworkReply* reply = m_networkManager.get(request);
    
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        previewImageDownloaded(reply);
    });
}

void MainWindow::previewImageDownloaded(QNetworkReply* reply)
{
    reply->deleteLater();
    
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray imageData = reply->readAll();
        QPixmap pixmap;
        pixmap.loadFromData(imageData);
        
        if (!pixmap.isNull()) {
            m_previewImage->setPixmap(pixmap.scaled(280, 280, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        }
    }
}

bool MainWindow::isImageFile(const QString& filePath) const
{
    QString extension = QFileInfo(filePath).suffix().toLower();
    return extension == "jpg" || extension == "jpeg" || 
           extension == "png" || extension == "gif" || 
           extension == "bmp" || extension == "webp";
}
