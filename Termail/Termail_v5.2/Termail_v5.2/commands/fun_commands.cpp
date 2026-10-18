#include "commands.h"
#include "terminal/terminal.h"
#include "utils/utils.h"

#include <QDateTime>
#include <QRandomGenerator>
#include <QThread>
#include <cmath>

void CommandRegistry::registerFunCommands(TerminalWidget* ui) {

    // calc（纯命令行）
    registerCommand("calc", [ui](const QStringList& args) {
        if (args.isEmpty()) { ui->appendOutput("用法: calc <表达式>  如 calc 3+5*2\n", ERROR_COLOR); return; }
        QString expr = args.join(' ');
        CommandHelper::runShell("echo $(( " + expr + " ))", ui->currentDir, ui);
    }, "计算器", "calc <表达式>");

    // bc
    registerCommand("bc", [ui](const QStringList& args) {
        ui->executeInternalCommand("calc " + args.join(' '));
    }, "计算器", "bc <表达式>");

    // expr
    registerCommand("expr", [ui](const QStringList& args) {
        ui->executeInternalCommand("calc " + args.join(' '));
    }, "计算表达式", "expr <表达式>");

    // convert
    registerCommand("convert", [ui](const QStringList& args) {
        if (args.size() < 4) return;
        double val = args[0].toDouble();
        QString from = args[1].toLower(), to = args[3].toLower();
        QMap<QString, QMap<QString, double>> conv;
        conv["km"]["miles"] = 0.621371; conv["miles"]["km"] = 1.60934;
        conv["kg"]["lbs"] = 2.20462; conv["lbs"]["kg"] = 0.453592;
        conv["m"]["ft"] = 3.28084; conv["ft"]["m"] = 0.3048;
        if (conv.contains(from) && conv[from].contains(to))
            ui->appendOutput(QString("%1 %2 = %3 %4\n").arg(val).arg(from).arg(val * conv[from][to], 0, 'f', 4).arg(to));
        else if (from == "c" && to == "f")
            ui->appendOutput(QString("%1°C = %2°F\n").arg(val).arg(val * 9 / 5 + 32, 0, 'f', 2));
        else if (from == "f" && to == "c")
            ui->appendOutput(QString("%1°F = %2°C\n").arg(val).arg((val - 32) * 5 / 9, 0, 'f', 2));
        else ui->appendOutput("不支持的转换\n", ERROR_COLOR);
    }, "单位转换", "convert <值> <单位1> to <单位2>");

    registerCommand("units", [ui](const QStringList& args) {
        ui->executeInternalCommand("convert " + args.join(' '));
    }, "单位转换", "units");

    // hex / bin / oct
    registerCommand("hex", [ui](const QStringList& args) {
        if (!args.isEmpty()) {
            bool ok; int n = args[0].toInt(&ok);
            if (ok) ui->appendOutput(QString("0x%1\n").arg(n, 0, 16).toUpper());
        }
    }, "十六进制", "hex <数字>");

    registerCommand("bin", [ui](const QStringList& args) {
        if (!args.isEmpty()) {
            bool ok; int n = args[0].toInt(&ok);
            if (ok) ui->appendOutput(QString("0b%1\n").arg(n, 0, 2));
        }
    }, "二进制", "bin <数字>");

    registerCommand("oct", [ui](const QStringList& args) {
        if (!args.isEmpty()) {
            bool ok; int n = args[0].toInt(&ok);
            if (ok) ui->appendOutput(QString("0o%1\n").arg(n, 0, 8));
        }
    }, "八进制", "oct <数字>");

    // cowsay
    registerCommand("cowsay", [ui](const QStringList& args) {
        QString text = args.isEmpty() ? "Moo!" : args.join(' ');
        QString top = QString("_").repeated(text.size() + 2);
        QString bot = QString("-").repeated(text.size() + 2);
        ui->appendOutput(QString(
            " %1\n< %2 >\n %3\n"
            "        \\   ^__^\n"
            "         \\  (oo)\\_______\n"
            "            (__)\\       )\\/\\\n"
            "                ||----w |\n"
            "                ||     ||\n"
        ).arg(top, text, bot));
    }, "牛说", "cowsay [文本]");

    // fortune
    registerCommand("fortune", [ui](const QStringList&) {
        QStringList fortunes = {
            "今天会是美好的一天！", "代码就是诗。", "保持好奇心。",
            "bug是日常伴侣。", "sudo make me a sandwich",
            "程序员最怕两件事：自己代码跑不起来，别人代码跑起来了"
        };
        ui->appendOutput(fortunes[QRandomGenerator::global()->bounded(fortunes.size())] + "\n", ACCENT_COLOR);
    }, "随机格言", "fortune");

    // banner / figlet
    registerCommand("banner", [ui](const QStringList& args) {
        if (args.isEmpty()) return;
        QString text = args.join(' ').toUpper();
        QString line = QString("█").repeated(text.size() + 4);
        ui->appendOutput(line + "\n" + QString("█ %1 █\n").arg(text) + line + "\n", ACCENT_COLOR);
    }, "横幅文字", "banner <文本>");

    registerCommand("figlet", [ui](const QStringList& args) {
        ui->executeInternalCommand("banner " + args.join(' '));
    }, "大字", "figlet <文本>");

    // cmatrix
    registerCommand("cmatrix", [ui](const QStringList&) {
        QString chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789@#$%&";
        for (int i = 0; i < 10; ++i) {
            QString line;
            for (int j = 0; j < 40; ++j)
                line += chars[QRandomGenerator::global()->bounded(chars.size())];
            ui->appendOutput(line + "\n", PROMPT_COLOR);
            QThread::msleep(50);
        }
    }, "矩阵效果", "cmatrix");

    // sl
    registerCommand("sl", [ui](const QStringList&) {
        for (int i = 0; i < 3; ++i) {
            ui->appendOutput(QString(" ").repeated(i) + "🚂 ===== 💨💨💨\n");
            QThread::msleep(200);
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
        arts["heart"] = "  ❤️ ❤️ ❤️\n ❤️   ❤️   ❤️\n  ❤️   ❤️\n    ❤️\n";
        arts["dog"] = "  / \\__\n (    @\\___\n /         O\n/   (_____/\n/_____/   U\n";
        if (!args.isEmpty() && arts.contains(args[0].toLower()))
            ui->appendOutput(arts[args[0].toLower()]);
        else
            ui->appendOutput("可用: " + QStringList(arts.keys()).join(", ") + "\n");
    }, "ASCII艺术", "ascii [名称]");

    // morse
    registerCommand("morse", [ui](const QStringList& args) {
        QMap<QChar, QString> code;
        code['A']=".-"; code['B']="-..."; code['C']="-.-."; code['S']="..."; code['O']="---"; code[' ']="/";
        QString text = args.isEmpty() ? "SOS" : args.join(' ').toUpper();
        QStringList result;
        for (const QChar& ch : text)
            result.append(code.contains(ch) ? code[ch] : QString(ch));
        ui->appendOutput(result.join(' ') + "\n", ACCENT_COLOR);
    }, "摩斯电码", "morse [文本]");

    // prime
    registerCommand("prime", [ui](const QStringList& args) {
        int n = args.isEmpty() ? 100 : args[0].toInt();
        auto isPrime = [](int x) {
            if (x < 2) return false;
            for (int i = 2; i * i <= x; ++i) if (x % i == 0) return false;
            return true;
        };
        int count = 0;
        for (int i = 2; i <= n; ++i)
            if (isPrime(i)) { ui->appendOutput(QString("%1 ").arg(i, 5)); if (++count % 10 == 0) ui->appendOutput("\n"); }
        ui->appendOutput(QString("\n共 %1 个素数\n").arg(count), INFO_COLOR);
    }, "素数", "prime [上限]");

    // factor
    registerCommand("factor", [ui](const QStringList& args) {
        if (args.isEmpty()) return;
        int n = args[0].toInt(), num = n, d = 2;
        QStringList factors;
        while (d * d <= num) {
            while (num % d == 0) { factors.append(QString::number(d)); num /= d; }
            d++;
        }
        if (num > 1) factors.append(QString::number(num));
        ui->appendOutput(QString("%1: %2\n").arg(n).arg(factors.join(' ')));
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
        ui->appendOutput(QString::number(M_PI, 'f', d) + "\n");
    }, "圆周率", "pi [小数位]");

    registerCommand("e", [ui](const QStringList& args) {
        int d = args.isEmpty() ? 10 : args[0].toInt();
        ui->appendOutput(QString::number(M_E, 'f', d) + "\n");
    }, "自然常数", "e [小数位]");

    registerCommand("phi", [ui](const QStringList& args) {
        int d = args.isEmpty() ? 10 : args[0].toInt();
        ui->appendOutput(QString::number((1.0 + std::sqrt(5.0)) / 2.0, 'f', d) + "\n");
    }, "黄金比例", "phi [小数位]");
}