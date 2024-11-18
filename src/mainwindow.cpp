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
    checkAndPromptApiKey();
}

void MainWindow::setupUi()
{
    auto* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    
    auto* mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setSpacing(20);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    
    // Status label to show if API key is set
    m_statusLabel = new QLabel(this);
    m_statusLabel->setObjectName("statusLabel");
    mainLayout->addWidget(m_statusLabel);
    
    // Create a container for the API key input (hidden by default)
    m_apiKeyWidget = new QWidget(this);
    auto* apiKeyLayout = new QHBoxLayout(m_apiKeyWidget);
    apiKeyLayout->setSpacing(10);
    
    m_apiKeyInput = new QLineEdit(m_apiKeyWidget);
    m_apiKeyInput->setPlaceholderText("Enter API Key");
    m_apiKeyInput->setEchoMode(QLineEdit::Password);
    apiKeyLayout->addWidget(m_apiKeyInput);
    
    m_saveButton = new QPushButton("Save", m_apiKeyWidget);
    apiKeyLayout->addWidget(m_saveButton);
    
    mainLayout->addWidget(m_apiKeyWidget);
    m_apiKeyWidget->hide();  // Hidden by default
    
    // Drop area
    m_dropArea = new QWidget(this);
    m_dropArea->setObjectName("dropArea");
    m_dropArea->setMinimumHeight(200);
    auto* dropLayout = new QVBoxLayout(m_dropArea);
    
    m_dropLabel = new QLabel("Drag and drop files here\nor", this);
    m_dropLabel->setAlignment(Qt::AlignCenter);
    dropLayout->addWidget(m_dropLabel);
    
    m_selectButton = new QPushButton("Select Files", this);
    m_selectButton->setFixedWidth(200);
    connect(m_selectButton, &QPushButton::clicked, this, &MainWindow::handleFileSelection);
    
    auto* buttonContainer = new QWidget(this);
    auto* buttonLayout = new QHBoxLayout(buttonContainer);
    buttonLayout->addWidget(m_selectButton);
    dropLayout->addWidget(buttonContainer);
    
    mainLayout->addWidget(m_dropArea);
    
    // Progress bar
    m_progressBar = new QProgressBar(this);
    m_progressBar->setMinimum(0);
    m_progressBar->setMaximum(100);
    m_progressBar->hide();
    mainLayout->addWidget(m_progressBar);
    
    // Connect the save button
    connect(m_saveButton, &QPushButton::clicked, this, &MainWindow::saveApiKey);
    
    // Enable drag and drop
    setAcceptDrops(true);
}

void MainWindow::loadApiKey()
{
    m_apiKey = m_settings.value("api_key").toString();
}

bool MainWindow::hasValidApiKey() const
{
    return !m_apiKey.isEmpty();
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

void MainWindow::saveApiKey()
{
    QString newKey = m_apiKeyInput->text().trimmed();
    if (newKey.isEmpty()) {
        QMessageBox::warning(this, "Invalid API Key", "Please enter a valid API key.");
        return;
    }
    
    m_settings.setValue("api_key", newKey);
    m_apiKey = newKey;
    m_apiKeyInput->clear();
    
    checkAndPromptApiKey();
    QMessageBox::information(this, "Success", "API key has been saved successfully.");
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
    
    QString filePath = QFileDialog::getOpenFileName(this, "Select File");
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
    QFile* file = new QFile(filePath);
    if (!file->open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, "Error", "Could not open file for upload.");
        file->deleteLater();
        return;
    }
    
    QHttpMultiPart* multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    
    QHttpPart filePart;
    filePart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("multipart/form-data"));
    filePart.setHeader(QNetworkRequest::ContentDispositionHeader,
                      QVariant(QString("form-data; name=\"file\"; filename=\"%1\"")
                              .arg(QFileInfo(filePath).fileName())));
    filePart.setBodyDevice(file);
    file->setParent(multiPart); // File will be deleted with multiPart
    multiPart->append(filePart);
    
    QNetworkRequest request(QUrl("https://api.e-z.host/files"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "multipart/form-data");
    request.setRawHeader("key", m_apiKey.toUtf8());
    
    m_currentUpload = m_networkManager.post(request, multiPart);
    multiPart->setParent(m_currentUpload); // multiPart will be deleted with reply
    
    m_progressBar->setValue(0);
    m_progressBar->show();
    m_selectButton->setEnabled(false);
    
    connect(m_currentUpload, &QNetworkReply::uploadProgress,
            this, &MainWindow::uploadProgress);
    connect(m_currentUpload, &QNetworkReply::finished,
            this, &MainWindow::uploadFinished);
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
    m_progressBar->hide();
    m_selectButton->setEnabled(true);
    
    if (m_currentUpload->error() == QNetworkReply::NoError) {
        QByteArray response = m_currentUpload->readAll();
        showUploadResult(response);
    } else {
        QMessageBox::warning(this, "Upload Error",
                           "Failed to upload file: " + m_currentUpload->errorString());
    }
    
    m_currentUpload->deleteLater();
    m_currentUpload = nullptr;
}

void MainWindow::showUploadResult(const QByteArray& response)
{
    QJsonDocument doc = QJsonDocument::fromJson(response);
    QJsonObject obj = doc.object();
    
    if (obj["success"].toBool()) {
        QString message = QString("File uploaded successfully!\n\n"
                                "Image URL: %1\n"
                                "Raw URL: %2\n"
                                "Deletion URL: %3")
                             .arg(obj["imageUrl"].toString())
                             .arg(obj["rawUrl"].toString())
                             .arg(obj["deletionUrl"].toString());
        
        QMessageBox::information(this, "Upload Success", message);
    } else {
        QMessageBox::warning(this, "Upload Error",
                           obj["message"].toString("Unknown error occurred"));
    }
}
