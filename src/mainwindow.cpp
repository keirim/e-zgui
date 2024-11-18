#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QMessageBox>
#include <QSettings>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_settings("E-Z Uploader", "Settings")
{
    setWindowTitle("E-Z Uploader");
    resize(600, 400);
    
    setupUi();
    loadApiKey();
    checkAndPromptApiKey();
}

void MainWindow::setupUi()
{
    auto* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    
    auto* mainLayout = new QVBoxLayout(centralWidget);
    
    // Status label to show if API key is set
    m_statusLabel = new QLabel(this);
    mainLayout->addWidget(m_statusLabel);
    
    // Create a container for the API key input (hidden by default)
    m_apiKeyWidget = new QWidget(this);
    auto* apiKeyLayout = new QHBoxLayout(m_apiKeyWidget);
    
    m_apiKeyInput = new QLineEdit(m_apiKeyWidget);
    m_apiKeyInput->setPlaceholderText("Enter API Key");
    m_apiKeyInput->setEchoMode(QLineEdit::Password);
    apiKeyLayout->addWidget(m_apiKeyInput);
    
    m_saveButton = new QPushButton("Save", m_apiKeyWidget);
    apiKeyLayout->addWidget(m_saveButton);
    
    mainLayout->addWidget(m_apiKeyWidget);
    m_apiKeyWidget->hide();  // Hidden by default
    
    // Connect the save button
    connect(m_saveButton, &QPushButton::clicked, this, &MainWindow::saveApiKey);
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
        m_statusLabel->setText("API Key: ✓ Configured");
        m_statusLabel->setStyleSheet("color: green;");
        m_apiKeyWidget->hide();
    } else {
        m_statusLabel->setText("API Key: ✗ Not Configured");
        m_statusLabel->setStyleSheet("color: red;");
        m_apiKeyWidget->show();
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
