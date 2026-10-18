#include "custom_commands_dialog.h"
#include "ui/terminal_ui.h"
#include "utils/utils.h"
#include "config.h"

#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDir>
#include <QFile>

CustomCommandsDialog::CustomCommandsDialog(TerminalUI* terminal, QWidget* parent)
    : QDialog(parent), term(terminal) {

    setWindowTitle("自定义命令管理");
    resize(400, 350);
    setStyleSheet(QString("background-color: %1;").arg(THEME.bg.name()));

    setupUI();
    loadCommands();
}

CustomCommandsDialog::~CustomCommandsDialog() {}

void CustomCommandsDialog::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // 标题
    QLabel* titleLabel = new QLabel("自定义命令列表:", this);
    titleLabel->setStyleSheet(QString("color: %1; font-size: 14px; font-weight: bold;").arg(THEME.fg.name()));
    mainLayout->addWidget(titleLabel);

    // 命令列表
    commandList = new QListWidget(this);
    commandList->setStyleSheet(
        QString("QListWidget { background-color: %1; color: %2; font-family: 'Consolas', 'Droid Sans Mono'; font-size: 12px; border: 1px solid #555; }"
                "QListWidget::item:selected { background-color: #3a3a3a; }")
            .arg(THEME.outputBg.name(), THEME.fg.name())
    );
    mainLayout->addWidget(commandList);
    connect(commandList, &QListWidget::currentRowChanged, this, &CustomCommandsDialog::onSelectCommand);

    // 编辑区域
    QFormLayout* formLayout = new QFormLayout();

    QLabel* nameLabel = new QLabel("命令名称:", this);
    nameLabel->setStyleSheet(QString("color: %1;").arg(THEME.fg.name()));
    nameEdit = new QLineEdit(this);
    nameEdit->setStyleSheet(
        QString("QLineEdit { background-color: %1; color: %2; font-family: 'Consolas', 'Droid Sans Mono'; font-size: 12px; border: 1px solid #555; padding: 4px; }")
            .arg(THEME.outputBg.name(), THEME.fg.name())
    );
    formLayout->addRow(nameLabel, nameEdit);

    QLabel* cmdLabel = new QLabel("命令内容:", this);
    cmdLabel->setStyleSheet(QString("color: %1;").arg(THEME.fg.name()));
    commandEdit = new QTextEdit(this);
    commandEdit->setMaximumHeight(100);
    commandEdit->setStyleSheet(
        QString("QTextEdit { background-color: %1; color: %2; font-family: 'Consolas', 'Droid Sans Mono'; font-size: 12px; border: 1px solid #555; }")
            .arg(THEME.outputBg.name(), THEME.fg.name())
    );
    formLayout->addRow(cmdLabel, commandEdit);

    QLabel* descLabel = new QLabel("描述:", this);
    descLabel->setStyleSheet(QString("color: %1;").arg(THEME.fg.name()));
    descEdit = new QLineEdit(this);
    descEdit->setStyleSheet(
        QString("QLineEdit { background-color: %1; color: %2; font-family: 'Consolas', 'Droid Sans Mono'; font-size: 12px; border: 1px solid #555; padding: 4px; }")
            .arg(THEME.outputBg.name(), THEME.fg.name())
    );
    formLayout->addRow(descLabel, descEdit);

    mainLayout->addLayout(formLayout);

    // 按钮区域
    QHBoxLayout* btnLayout = new QHBoxLayout();

    QString btnStyle = "QPushButton { color: white; border: none; padding: 8px 16px; border-radius: 3px; font-weight: bold; }";

    addBtn = new QPushButton("新增", this);
    addBtn->setStyleSheet(btnStyle + "QPushButton { background-color: #4CAF50; } QPushButton:hover { background-color: #45a049; }");
    btnLayout->addWidget(addBtn);
    connect(addBtn, &QPushButton::clicked, this, &CustomCommandsDialog::onAddCommand);

    updateBtn = new QPushButton("更新", this);
    updateBtn->setStyleSheet(btnStyle + "QPushButton { background-color: #2196F3; } QPushButton:hover { background-color: #1976D2; }");
    btnLayout->addWidget(updateBtn);
    connect(updateBtn, &QPushButton::clicked, this, &CustomCommandsDialog::onUpdateCommand);

    deleteBtn = new QPushButton("删除", this);
    deleteBtn->setStyleSheet(btnStyle + "QPushButton { background-color: #f44336; } QPushButton:hover { background-color: #d32f2f; }");
    btnLayout->addWidget(deleteBtn);
    connect(deleteBtn, &QPushButton::clicked, this, &CustomCommandsDialog::onDeleteCommand);

    saveBtn = new QPushButton("保存", this);
    saveBtn->setStyleSheet(btnStyle + "QPushButton { background-color: #FF9800; } QPushButton:hover { background-color: #F57C00; }");
    btnLayout->addWidget(saveBtn);
    connect(saveBtn, &QPushButton::clicked, this, &CustomCommandsDialog::onSaveCommands);

    btnLayout->addStretch();

    closeBtn = new QPushButton("关闭", this);
    closeBtn->setStyleSheet(btnStyle + "QPushButton { background-color: #607D8B; } QPushButton:hover { background-color: #455A64; }");
    btnLayout->addWidget(closeBtn);
    connect(closeBtn, &QPushButton::clicked, this, &CustomCommandsDialog::onCloseDialog);

    mainLayout->addLayout(btnLayout);
    setLayout(mainLayout);
}

void CustomCommandsDialog::loadCommands() {
    commandList->clear();
    for (auto it = term->customCommands.begin(); it != term->customCommands.end(); ++it) {
        QString name = it.key();
        QString desc = it.value()["desc"].toString();
        commandList->addItem(QString("%1 - %2").arg(name, desc));
    }
}

void CustomCommandsDialog::onSelectCommand() {
    int row = commandList->currentRow();
    if (row < 0) return;

    QString itemText = commandList->currentItem()->text();
    QString name = itemText.split(" - ").first();

    if (term->customCommands.contains(name)) {
        QJsonObject cmdInfo = term->customCommands[name];
        nameEdit->setText(name);
        commandEdit->setPlainText(cmdInfo["command"].toString());
        descEdit->setText(cmdInfo["desc"].toString());
    }
}

void CustomCommandsDialog::onAddCommand() {
    QString name = nameEdit->text().trimmed();
    QString command = commandEdit->toPlainText().trimmed();
    QString desc = descEdit->text().trimmed();

    if (name.isEmpty() || command.isEmpty()) {
        QMessageBox::critical(this, "错误", "命令名称和内容不能为空");
        return;
    }

    if (term->customCommands.contains(name)) {
        QMessageBox::critical(this, "错误", QString("命令 '%1' 已存在").arg(name));
        return;
    }

    QJsonObject cmdInfo;
    cmdInfo["command"] = command;
    cmdInfo["desc"] = desc;
    term->customCommands[name] = cmdInfo;

    loadCommands();
    clearFields();
}

void CustomCommandsDialog::onUpdateCommand() {
    QString name = nameEdit->text().trimmed();
    QString command = commandEdit->toPlainText().trimmed();
    QString desc = descEdit->text().trimmed();

    if (name.isEmpty() || command.isEmpty()) {
        QMessageBox::critical(this, "错误", "命令名称和内容不能为空");
        return;
    }

    QJsonObject cmdInfo;
    cmdInfo["command"] = command;
    cmdInfo["desc"] = desc;
    term->customCommands[name] = cmdInfo;

    loadCommands();
    clearFields();
}

void CustomCommandsDialog::onDeleteCommand() {
    int row = commandList->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, "提示", "请先选择一个命令");
        return;
    }

    QString itemText = commandList->currentItem()->text();
    QString name = itemText.split(" - ").first();

    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "确认删除", QString("确定要删除命令 '%1' 吗？").arg(name),
        QMessageBox::Yes | QMessageBox::No
    );

    if (reply == QMessageBox::Yes) {
        term->customCommands.remove(name);
        loadCommands();
        clearFields();
    }
}

void CustomCommandsDialog::onSaveCommands() {
    QString configPath = Utils::customCommandsPath();
    QJsonObject obj;
    for (auto it = term->customCommands.begin(); it != term->customCommands.end(); ++it) {
        obj[it.key()] = it.value();
    }

    if (Utils::saveJsonFile(configPath, obj)) {
        QMessageBox::information(this, "成功", "自定义命令已保存");
    } else {
        QMessageBox::critical(this, "错误", "保存失败");
    }
}

void CustomCommandsDialog::onCloseDialog() {
    close();
}

void CustomCommandsDialog::clearFields() {
    nameEdit->clear();
    commandEdit->clear();
    descEdit->clear();
}