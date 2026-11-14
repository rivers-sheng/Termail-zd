#ifndef EDITOR_DIALOG_H
#define EDITOR_DIALOG_H

#include <QDialog>
#include <QTextEdit>
#include <QPlainTextEdit> // 新增
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QString>

class EditorDialog : public QDialog {
    Q_OBJECT

public:
    explicit EditorDialog(const QString& filePath, QWidget* parent = nullptr);
    ~EditorDialog();

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void closeEvent(QCloseEvent* event) override;

private slots:
    void saveFile();
    void findText();
    void replaceText();
    void toggleLineNumbers();
    void updateLineNumberAreaWidth(int newBlockCount); // 新增
    void updateLineNumberArea(const QRect &rect, int dy); // 新增

private:
    void setupUI();
    void loadFile();
    bool maybeSave();
    void updateTitle();
    void updateLineNumbers();

    QString filePath;
    QTextEdit* textEdit;
    QPlainTextEdit* lineNumberArea; // 新增：专门显示行号
    QLineEdit* searchField;
    QLabel* statusLabel;
    QPushButton* saveBtn;
    QPushButton* closeBtn;
    QPushButton* findBtn;
    QPushButton* replaceBtn;
    bool modified;
    bool lineNumbersVisible;
};

#endif