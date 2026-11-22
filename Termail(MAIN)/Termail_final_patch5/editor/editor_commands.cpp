#include "commands.h"
#include "terminal/terminal.h"
#include "editor/editor_dialog.h"

#include <QFile>
#include <QDir>
#include <QFileInfo>

namespace {

// 打开编辑器的统一入口，绕开 executeInternalCommand 的字符串解析
void openEditorFor(TerminalWidget* ui, const QString& arg) {
    if (arg.isEmpty()) {
        ui->appendOutput("文件名不能为空\n", ERROR_COLOR);
        return;
    }
    QString path = QDir(ui->currentDir).absoluteFilePath(arg);
    ui->appendOutput(QString("打开编辑器: %1\n").arg(arg), ACCENT_COLOR);
    EditorDialog* editor = new EditorDialog(path, ui);
    editor->setAttribute(Qt::WA_DeleteOnClose);
    editor->showFullScreen();
}

} // namespace

void CommandRegistry::registerEditorCommands(TerminalWidget* ui) {

    // edit - 打开编辑器
    registerCommand("edit", [ui](const QStringList& args) {
        if (args.isEmpty()) {
            ui->appendOutput("用法: edit <文件>\n", ERROR_COLOR);
            return;
        }
        openEditorFor(ui, args[0]);
    }, "图形化编辑器", "edit <文件>");

    // nano - 同 edit
    registerCommand("nano", [ui](const QStringList& args) {
        if (args.isEmpty()) {
            ui->appendOutput("用法: nano <文件>\n", ERROR_COLOR);
            return;
        }
        openEditorFor(ui, args[0]);
    }, "编辑器", "nano <文件>");

    // create - 创建并编辑
    registerCommand("create", [ui](const QStringList& args) {
        if (args.isEmpty()) {
            ui->appendOutput("用法: create <文件>\n", ERROR_COLOR);
            return;
        }
        QString path = QDir(ui->currentDir).absoluteFilePath(args[0]);
        if (!QFile::exists(path)) {
            QFile f(path);
            if (f.open(QIODevice::WriteOnly)) {
                f.close();
                ui->appendOutput(QString("已创建: %1\n").arg(args[0]), INFO_COLOR);
            } else {
                ui->appendOutput("创建失败，检查目录权限\n", ERROR_COLOR);
                return;
            }
        }
        openEditorFor(ui, args[0]);
    }, "创建并编辑", "create <文件>");
}