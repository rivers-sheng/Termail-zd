#ifndef CUSTOM_COMMANDS_DIALOG_H
#define CUSTOM_COMMANDS_DIALOG_H

#include <QDialog>
#include <QListWidget>
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QJsonObject>
#include <QMap>

class TerminalUI;

class CustomCommandsDialog : public QDialog {
    Q_OBJECT

public:
    explicit CustomCommandsDialog(TerminalUI* terminal, QWidget* parent = nullptr);
    ~CustomCommandsDialog();

private slots:
    void onSelectCommand();
    void onAddCommand();
    void onUpdateCommand();
    void onDeleteCommand();
    void onSaveCommands();
    void onCloseDialog();

private:
    void setupUI();
    void loadCommands();
    void clearFields();

    TerminalUI* term;

    // UI组件
    QListWidget* commandList;
    QLineEdit* nameEdit;
    QTextEdit* commandEdit;
    QLineEdit* descEdit;
    QPushButton* addBtn;
    QPushButton* updateBtn;
    QPushButton* deleteBtn;
    QPushButton* saveBtn;
    QPushButton* closeBtn;
};

#endif // CUSTOM_COMMANDS_DIALOG_H