#pragma once

#include <QMainWindow>
#include <QSettings>

class QLineEdit;
class QPushButton;
class QLabel;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override = default;

private slots:
    void saveApiKey();
    void checkAndPromptApiKey();

private:
    void setupUi();
    void createApiKeyPrompt();
    bool hasValidApiKey() const;
    void loadApiKey();

    QSettings m_settings;
    QString m_apiKey;
    QLineEdit* m_apiKeyInput = nullptr;
    QPushButton* m_saveButton = nullptr;
    QWidget* m_apiKeyWidget = nullptr;
    QLabel* m_statusLabel = nullptr;
};
