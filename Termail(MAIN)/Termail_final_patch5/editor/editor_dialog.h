#ifndef EDITOR_DIALOG_H
#define EDITOR_DIALOG_H

#include <QDialog>
#include <QTextEdit>
#include <QPlainTextEdit>
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
    void resizeEvent(QResizeEvent* event) override;

private slots:
    void saveFile();
    void findText();
    void replaceText();
    void toggleLineNumbers();

private:
    void setupUI();
    void loadFile();
    bool maybeSave();
    void updateTitle();
    void updateLineNumbers();
    void syncLineNumberScroll();

    QString filePath;
    QTextEdit* textEdit;
    QPlainTextEdit* lineNumberArea;
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