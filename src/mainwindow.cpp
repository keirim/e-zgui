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
    contentWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    auto* contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setSpacing(15);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    
    // Drop area
    m_dropArea = new QWidget(this);
    m_dropArea->setObjectName("dropArea");
    m_dropArea->setMinimumHeight(300);
    auto* dropLayout = new QVBoxLayout(m_dropArea);
    dropLayout->setSpacing(15);
    
    m_dropLabel = new QLabel("Drag and drop files here\nor", this);
    m_dropLabel->setAlignment(Qt::AlignCenter);
    dropLayout->addWidget(m_dropLabel);
    
    m_selectButton = new QPushButton("Select Files", this);
    m_selectButton->setFixedWidth(200);
    m_selectButton->setObjectName("selectButton");
    
    auto* buttonContainer = new QWidget(this);
    auto* buttonLayout = new QHBoxLayout(buttonContainer);
    buttonLayout->setAlignment(Qt::AlignCenter);
    buttonLayout->addWidget(m_selectButton);
    dropLayout->addWidget(buttonContainer);
    
    contentLayout->addWidget(m_dropArea);
    
    // Progress bar
    m_progressBar = new QProgressBar(this);
    m_progressBar->setMinimum(0);
    m_progressBar->setMaximum(100);
    m_progressBar->hide();
    contentLayout->addWidget(m_progressBar);
    
    mainLayout->addWidget(contentWidget);
    
    // Connect signals
    connect(m_saveButton, &QPushButton::clicked, this, &MainWindow::saveApiKey);
    connect(m_logoutButton, &QPushButton::clicked, this, &MainWindow::logout);
    connect(m_selectButton, &QPushButton::clicked, this, &MainWindow::handleFileSelection);
    
    // Enable drag and drop
    setAcceptDrops(true);
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
    
    QFile* file = new QFile(filePath);
    if (!file->open(QIODevice::ReadOnly)) {
        QString error = QString("Could not open file for upload: %1").arg(file->errorString());
        qDebug() << "Error:" << error;
        QMessageBox::warning(this, "Error", error);
        file->deleteLater();
        return;
    }

    qDebug() << "File opened successfully. Size:" << file->size() << "bytes";
    
    // Read file content
    QByteArray fileData = file->readAll();
    file->close();
    file->deleteLater();

    // Create the network request
    QNetworkRequest request(QUrl("https://api.e-z.host/files"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "multipart/form-data; boundary=boundary");
    request.setRawHeader("key", m_apiKey.toUtf8());

    // Create multipart data
    QByteArray data;
    data.append("--boundary\r\n");
    data.append("Content-Disposition: form-data; name=\"file\"; filename=\"");
    data.append(QFileInfo(filePath).fileName().toUtf8());
    data.append("\"\r\n");
    data.append("Content-Type: application/octet-stream\r\n");
    data.append("\r\n");
    data.append(fileData);
    data.append("\r\n");
    data.append("--boundary--\r\n");

    qDebug() << "Sending request to:" << request.url().toString();
    qDebug() << "Headers:";
    qDebug() << "- Content-Type:" << request.header(QNetworkRequest::ContentTypeHeader).toString();
    qDebug() << "- Key:" << m_apiKey.left(4) + "..." << "(truncated for security)";
    
    m_currentUpload = m_networkManager.post(request, data);
    
    m_progressBar->setValue(0);
    m_progressBar->show();
    m_selectButton->setEnabled(false);
    
    connect(m_currentUpload, &QNetworkReply::uploadProgress,
            this, &MainWindow::uploadProgress);
    connect(m_currentUpload, &QNetworkReply::finished,
            this, &MainWindow::uploadFinished);
    
    connect(m_currentUpload, &QNetworkReply::errorOccurred,
            this, [this](QNetworkReply::NetworkError error) {
                qDebug() << "Network error occurred:" << error;
                qDebug() << "Error string:" << m_currentUpload->errorString();
            });
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
    qDebug() << "Parsing upload response";
    
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(response, &parseError);
    
    if (parseError.error != QJsonParseError::NoError) {
        qDebug() << "JSON parse error:" << parseError.errorString();
        QMessageBox::warning(this, "Error", "Failed to parse server response");
        return;
    }
    
    QJsonObject obj = doc.object();
    
    if (obj["success"].toBool()) {
        QString imageUrl = obj["imageUrl"].toString();
        QString rawUrl = obj["rawUrl"].toString();
        QString deletionUrl = obj["deletionUrl"].toString();
        
        qDebug() << "Upload successful:";
        qDebug() << "- Image URL:" << imageUrl;
        qDebug() << "- Raw URL:" << rawUrl;
        qDebug() << "- Deletion URL:" << deletionUrl;
        
        QString message = QString("File uploaded successfully!\n\n"
                                "Image URL: %1\n"
                                "Raw URL: %2\n"
                                "Deletion URL: %3")
                             .arg(imageUrl)
                             .arg(rawUrl)
                             .arg(deletionUrl);
        
        QMessageBox::information(this, "Upload Success", message);
    } else {
        QString errorMessage = obj["message"].toString("Unknown error occurred");
        qDebug() << "Upload failed with error:" << errorMessage;
        QMessageBox::warning(this, "Upload Error", errorMessage);
    }
}
