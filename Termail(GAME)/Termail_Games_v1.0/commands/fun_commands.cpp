#include "commands.h"
#include "terminal/terminal.h"
#include "utils/utils.h"

#include <QDateTime>
#include <QRandomGenerator>
#include <QThread>
#include <QCoreApplication>
#include <QRegularExpression>
#include <cmath>
#include <QProcess>
#include <QUuid>

void CommandRegistry::registerFunCommands(TerminalWidget* ui) {

    // calc
    registerCommand("calc", [ui](const QStringList& args) {
        if (args.isEmpty()) { ui->appendOutput("用法: calc <表达式>\n", ERROR_COLOR); return; }
        QString expr = args.join(' ');
        QRegularExpression valid("^[0-9+\\-*/%()\\s\\.]+$");
        if (!valid.match(expr).hasMatch() || expr.contains("()")) {
            ui->appendOutput("表达式不合法\n", ERROR_COLOR);
            return;
        }
        // 简单计算
        QProcess proc;
        proc.start("sh", QStringList() << "-c" << "echo $(( " + expr + " ))");
        proc.waitForFinished(3000);
        QString result = proc.readAllStandardOutput().trimmed();
        if (!result.isEmpty()) ui->appendOutput(QString("%1 = %2\n").arg(expr, result), PROMPT_COLOR);
        else ui->appendOutput("计算失败\n", ERROR_COLOR);
    }, "计算器", "calc <表达式>");

    registerCommand("bc", [ui](const QStringList& args) {
        ui->executeInternalCommand("calc " + args.join(' '));
    }, "计算器", "bc");

    // cowsay
    registerCommand("cowsay", [ui](const QStringList& args) {
        QString text = args.isEmpty() ? "Moo!" : args.join(' ');
        if (text.contains('\n')) {
            ui->appendOutput("cowsay只支持单行\n", ERROR_COLOR);
            return;
        }
        QString top = QString("_").repeated(text.size() + 2);
        QString bot = QString("-").repeated(text.size() + 2);
        ui->appendOutput(QString(
            " %1\n< %2 >\n %3\n"
            "        \\   ^__^\n"
            "         \\  (oo)\\_______\n"
            "            (__)\\       )\\/\\\n"
            "                ||----w |\n"
            "                ||     ||\n"
        ).arg(top, text, bot), ACCENT_COLOR);
    }, "牛说", "cowsay [文本]");

    // fortune
    registerCommand("fortune", [ui](const QStringList&) {
        QStringList fortunes = {
            "今天会是美好的一天！",
            "代码就是诗。",
            "保持好奇心。",
            "bug是日常伴侣。",
            "sudo make me a sandwich",
            "游戏是程序员的浪漫。",
            "Termail Games 陪你度过每一天。"
        };
        ui->appendOutput(fortunes[QRandomGenerator::global()->bounded(fortunes.size())] + "\n", ACCENT_COLOR);
    }, "随机格言", "fortune");

    // banner / figlet
    registerCommand("banner", [ui](const QStringList& args) {
        if (args.isEmpty()) return;
        QString text = args.join(' ').toUpper();
        QString line = QString("=").repeated(text.size() + 4);
        ui->appendOutput(line + "\n| " + text + " |\n" + line + "\n", ACCENT_COLOR);
    }, "横幅文字", "banner <文本>");

    registerCommand("figlet", [ui](const QStringList& args) {
        ui->executeInternalCommand("banner " + args.join(' '));
    }, "大字", "figlet");

    // cmatrix
    registerCommand("cmatrix", [ui](const QStringList&) {
        QString chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789@#$%&";
        for (int i = 0; i < 10; ++i) {
            QString line;
            for (int j = 0; j < 40; ++j)
                line += chars[QRandomGenerator::global()->bounded(chars.size())];
            ui->appendOutput(line + "\n", PROMPT_COLOR);
            QThread::msleep(50);
            QCoreApplication::processEvents();
        }
    }, "矩阵效果", "cmatrix");

    // sl
    registerCommand("sl", [ui](const QStringList&) {
        for (int i = 0; i < 3; ++i) {
            ui->appendOutput(QString(" ").repeated(i) + "🚂 ===== 💨💨💨\n", PROMPT_COLOR);
            QThread::msleep(200);
            QCoreApplication::processEvents();
        }
    }, "蒸汽火车", "sl");

    // lolcat
    registerCommand("lolcat", [ui](const QStringList& args) {
        QString text = args.isEmpty() ? "Hello, Rainbow!" : args.join(' ');
        QStringList colors = {"#ff0000", "#ff8800", "#ffff00", "#00ff00", "#0088ff", "#8800ff"};
        for (int i = 0; i < text.size(); ++i)
            ui->appendOutput(QString(text[i]), QColor(colors[i % colors.size()]));
        ui->appendOutput("\n");
    }, "彩虹输出", "lolcat [文本]");

    // ascii
    registerCommand("ascii", [ui](const QStringList& args) {
        QMap<QString, QString> arts;
        arts["cat"] = "  /\\_/\\\n ( o.o )\n  > ^ <\n";
        arts["heart"] = "  ♥ ♥ ♥\n ♥   ♥   ♥\n  ♥   ♥\n    ♥\n";
        arts["dog"] = "  / \\__\n (    @\\___\n /         O\n/   (_____/\n/_____/   U\n";
        if (!args.isEmpty() && arts.contains(args[0].toLower()))
            ui->appendOutput(arts[args[0].toLower()], PROMPT_COLOR);
        else
            ui->appendOutput("可用: " + QStringList(arts.keys()).join(", ") + "\n", INFO_COLOR);
    }, "ASCII艺术", "ascii [名称]");

    // morse
    registerCommand("morse", [ui](const QStringList& args) {
        QMap<QChar, QString> code;
        code['A']=".-"; code['B']="-..."; code['C']="-.-."; code['D']="-..";
        code['E']="."; code['F']="..-."; code['G']="--."; code['H']="....";
        code['I']=".."; code['J']=".---"; code['K']="-.-"; code['L']=".-..";
        code['M']="--"; code['N']="-."; code['O']="---"; code['P']=".--.";
        code['Q']="--.-"; code['R']=".-."; code['S']="..."; code['T']="-";
        code['U']="..-"; code['V']="...-"; code['W']=".--"; code['X']="-..-";
        code['Y']="-.--"; code['Z']="--..";
        code['0']="-----"; code['1']=".----"; code['2']="..---"; code['3']="...--";
        code['4']="....-"; code['5']="....."; code['6']="-...."; code['7']="--...";
        code['8']="---.."; code['9']="----."; code[' ']="/";

        QString text = args.isEmpty() ? "SOS" : args.join(' ').toUpper();
        QStringList result;
        for (const QChar& ch : text)
            result.append(code.contains(ch) ? code[ch] : QString(ch));
        ui->appendOutput(result.join(' ') + "\n", ACCENT_COLOR);
    }, "摩斯电码", "morse [文本]");

    // prime
    registerCommand("prime", [ui](const QStringList& args) {
        int n = args.isEmpty() ? 100 : args[0].toInt();
        if (n > 1000000) { ui->appendOutput("上限1000000\n", ERROR_COLOR); return; }
        QVector<bool> isPrime(n + 1, true);
        isPrime[0] = isPrime[1] = false;
        for (int i = 2; i * i <= n; ++i)
            if (isPrime[i])
                for (int j = i * i; j <= n; j += i) isPrime[j] = false;
        int count = 0;
        for (int i = 2; i <= n; ++i)
            if (isPrime[i]) {
                ui->appendOutput(QString("%1 ").arg(i, 5));
                if (++count % 10 == 0) ui->appendOutput("\n");
            }
        ui->appendOutput(QString("\n共 %1 个\n").arg(count), INFO_COLOR);
    }, "素数", "prime [上限]");

    // factor
    registerCommand("factor", [ui](const QStringList& args) {
        if (args.isEmpty()) return;
        qint64 n = args[0].toLongLong();
        qint64 num = n;
        int d = 2;
        QStringList factors;
        while (d * d <= num) {
            while (num % d == 0) { factors.append(QString::number(d)); num /= d; }
            d++;
        }
        if (num > 1) factors.append(QString::number(num));
        ui->appendOutput(QString("%1: %2\n").arg(n).arg(factors.join(' ')), PROMPT_COLOR);
    }, "因数分解", "factor <数字>");

    // fibonacci
    registerCommand("fibonacci", [ui](const QStringList& args) {
        int n = args.isEmpty() ? 10 : args[0].toInt();
        qint64 a = 0, b = 1;
        QStringList fibs;
        for (int i = 0; i < n; ++i) {
            fibs.append(QString::number(a));
            qint64 c = a + b; a = b; b = c;
        }
        ui->appendOutput(fibs.join(' ') + "\n");
    }, "斐波那契", "fibonacci [项数]");

    // pi / e / phi
    registerCommand("pi", [ui](const QStringList& args) {
        int d = args.isEmpty() ? 10 : args[0].toInt();
        ui->appendOutput(QString::number(M_PI, 'f', d) + "\n", PROMPT_COLOR);
    }, "圆周率", "pi [小数位]");

    registerCommand("e", [ui](const QStringList& args) {
        int d = args.isEmpty() ? 10 : args[0].toInt();
        ui->appendOutput(QString::number(M_E, 'f', d) + "\n", PROMPT_COLOR);
    }, "自然常数", "e [小数位]");

    registerCommand("phi", [ui](const QStringList& args) {
        int d = args.isEmpty() ? 10 : args[0].toInt();
        ui->appendOutput(QString::number((1.0 + std::sqrt(5.0)) / 2.0, 'f', d) + "\n", PROMPT_COLOR);
    }, "黄金比例", "phi [小数位]");

    // hex / bin / oct
    registerCommand("hex", [ui](const QStringList& args) {
        if (!args.isEmpty()) { bool ok; int n = args[0].toInt(&ok);
            if (ok) ui->appendOutput(QString("0x%1\n").arg(n, 0, 16).toUpper(), PROMPT_COLOR); }
    }, "十六进制", "hex <数字>");

    registerCommand("bin", [ui](const QStringList& args) {
        if (!args.isEmpty()) { bool ok; int n = args[0].toInt(&ok);
            if (ok) ui->appendOutput(QString("0b%1\n").arg(n, 0, 2), PROMPT_COLOR); }
    }, "二进制", "bin <数字>");

    registerCommand("oct", [ui](const QStringList& args) {
        if (!args.isEmpty()) { bool ok; int n = args[0].toInt(&ok);
            if (ok) ui->appendOutput(QString("0o%1\n").arg(n, 0, 8), PROMPT_COLOR); }
    }, "八进制", "oct <数字>");

    // random
    registerCommand("random", [ui](const QStringList& args) {
        if (args.isEmpty())
            ui->appendOutput(QString::number(QRandomGenerator::global()->generateDouble()) + "\n", PROMPT_COLOR);
        else if (args.size() == 1)
            ui->appendOutput(QString::number(QRandomGenerator::global()->bounded(args[0].toInt() + 1)) + "\n", PROMPT_COLOR);
        else
            ui->appendOutput(QString::number(QRandomGenerator::global()->bounded(args[0].toInt(), args[1].toInt() + 1)) + "\n", PROMPT_COLOR);
    }, "随机数", "random [最小] [最大]");

    // uuid
    registerCommand("uuid", [ui](const QStringList&) {
        ui->appendOutput(QUuid::createUuid().toString(QUuid::WithoutBraces) + "\n", PROMPT_COLOR);
    }, "UUID", "uuid");
}