#include "commands.h"
#include "ui/terminal_ui.h"
#include "utils/utils.h"

#include <QThread>
#include <QCoreApplication>
#include <QDateTime>
#include <QRandomGenerator>
#include <cmath>
// ============ 趣味命令注册 ============
void CommandRegistry::registerFunCommands(TerminalUI* ui) {

    // bc
    registerCommand("bc", [ui](const QStringList& args) {
        if (args.isEmpty()) {
            ui->appendOutput("用法: bc <表达式>\n", THEME.yellow.name());
            return;
        }
        CommandHelper::runShell("echo " + args.join(' ') + " | bc -l", ui->currentDir, ui);
    }, "计算器", "bc <表达式>");

    // calc / expr
    registerCommand("calc", [ui](const QStringList& args) {
        if (args.isEmpty()) {
            ui->appendOutput("用法: calc <表达式>\n", THEME.yellow.name());
            return;
        }
        QString expr = args.join(' ');
        // 简单数学计算
        CommandHelper::runShell("python3 -c \"print(" + expr + ")\"", ui->currentDir, ui);
    }, "计算器", "calc <表达式>");

    registerCommand("expr", [ui](const QStringList& args) {
        ui->executeInternalCommand("calc " + args.join(' '));
    }, "计算表达式", "expr <表达式>");

    // convert
    registerCommand("convert", [ui](const QStringList& args) {
        if (args.size() < 4) return;
        double val = args[0].toDouble();
        QString from = args[1].toLower();
        QString to = args[3].toLower();

        QMap<QString, QMap<QString, double>> conversions;
        conversions["km"]["miles"] = 0.621371;
        conversions["miles"]["km"] = 1.60934;
        conversions["kg"]["lbs"] = 2.20462;
        conversions["lbs"]["kg"] = 0.453592;
        conversions["m"]["ft"] = 3.28084;
        conversions["ft"]["m"] = 0.3048;

        if (conversions.contains(from) && conversions[from].contains(to)) {
            double result = val * conversions[from][to];
            ui->appendOutput(QString("%1 %2 = %3 %4\n").arg(val).arg(from).arg(result, 0, 'f', 4).arg(to), THEME.green.name());
        } else if (from == "c" && to == "f") {
            double result = val * 9.0 / 5.0 + 32;
            ui->appendOutput(QString("%1 °C = %2 °F\n").arg(val).arg(result, 0, 'f', 2), THEME.green.name());
        } else if (from == "f" && to == "c") {
            double result = (val - 32) * 5.0 / 9.0;
            ui->appendOutput(QString("%1 °F = %2 °C\n").arg(val).arg(result, 0, 'f', 2), THEME.green.name());
        } else {
            ui->appendOutput("不支持的转换\n", THEME.yellow.name());
        }
    }, "单位转换", "convert <值> <单位1> to <单位2>");

    // units
    registerCommand("units", [ui](const QStringList& args) {
        ui->executeInternalCommand("convert " + args.join(' '));
    }, "单位转换", "units");

    // hex / bin / oct
    registerCommand("hex", [ui](const QStringList& args) {
        if (!args.isEmpty()) {
            bool ok;
            int n = args[0].toInt(&ok);
            if (ok) {
                ui->appendOutput(QString("0x%1\n").arg(n, 0, 16).toUpper(), THEME.green.name());
            }
        }
    }, "十六进制转换", "hex <数字>");

    registerCommand("bin", [ui](const QStringList& args) {
        if (!args.isEmpty()) {
            bool ok;
            int n = args[0].toInt(&ok);
            if (ok) {
                ui->appendOutput(QString("0b%1\n").arg(n, 0, 2), THEME.green.name());
            }
        }
    }, "二进制转换", "bin <数字>");

    registerCommand("oct", [ui](const QStringList& args) {
        if (!args.isEmpty()) {
            bool ok;
            int n = args[0].toInt(&ok);
            if (ok) {
                ui->appendOutput(QString("0o%1\n").arg(n, 0, 8), THEME.green.name());
            }
        }
    }, "八进制转换", "oct <数字>");

    // cowsay
    registerCommand("cowsay", [ui](const QStringList& args) {
        QString text = args.isEmpty() ? "Moo!" : args.join(' ');
        QString borderTop = QString("_").repeated(text.size() + 2);
        QString borderBot = QString("-").repeated(text.size() + 2);
        QString cow = QString(
            " %1\n"
            "< %2 >\n"
            " %3\n"
            "        \\   ^__^\n"
            "         \\  (oo)\\_______\n"
            "            (__)\\       )\\/\\\n"
            "                ||----w |\n"
            "                ||     ||\n"
        ).arg(borderTop, text, borderBot);
        ui->appendOutput(cow, THEME.yellow.name());
    }, "牛说", "cowsay [文本]");

    // fortune
    registerCommand("fortune", [ui](const QStringList&) {
        QStringList fortunes = {
            "今天会是美好的一天！",
            "代码就是诗。",
            "保持好奇心，保持愚蠢。",
            "bug是日常伴侣，debug是生活态度。",
            "一切皆有可能，除了Windows更新。",
            "生活就像C++，充满了未定义行为。",
            "sudo make me a sandwich",
            "程序员最怕的两件事：1. 自己的代码跑不起来 2. 别人的代码跑起来了"
        };
        int idx = QRandomGenerator::global()->bounded(fortunes.size());
        ui->appendOutput(fortunes[idx] + "\n", THEME.green.name());
    }, "随机格言", "fortune");

    // banner / figlet
    registerCommand("banner", [ui](const QStringList& args) {
        if (args.isEmpty()) return;
        QString text = args.join(' ').toUpper();
        QString line = QString("█").repeated(text.size() + 4);
        ui->appendOutput(line + "\n", THEME.cyan.name());
        ui->appendOutput(QString("█ %1 █\n").arg(text), THEME.cyan.name());
        ui->appendOutput(line + "\n", THEME.cyan.name());
    }, "横幅文字", "banner <文本>");

    registerCommand("figlet", [ui](const QStringList& args) {
        ui->executeInternalCommand("banner " + args.join(' '));
    }, "大字", "figlet <文本>");

    // cmatrix
    registerCommand("cmatrix", [ui](const QStringList&) {
        QString chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789@#$%&";
        for (int i = 0; i < 10; ++i) {
            QString line;
            for (int j = 0; j < 40; ++j) {
                line += chars[QRandomGenerator::global()->bounded(chars.size())];
            }
            ui->appendOutput(line + "\n", THEME.green.name());
            QThread::msleep(50);
            QCoreApplication::processEvents();
        }
    }, "矩阵效果", "cmatrix");

    // sl
    registerCommand("sl", [ui](const QStringList&) {
        for (int i = 0; i < 3; ++i) {
            ui->appendOutput(QString(" ").repeated(i) + "🚂 ===== 💨💨💨\n", THEME.yellow.name());
            QThread::msleep(200);
            QCoreApplication::processEvents();
        }
    }, "蒸汽火车", "sl");

    // lolcat
    registerCommand("lolcat", [ui](const QStringList& args) {
        QString text = args.isEmpty() ? "Hello, Rainbow!" : args.join(' ');
        QStringList colors = {"#ff0000", "#ff8800", "#ffff00", "#00ff00", "#0088ff", "#8800ff"};
        for (int i = 0; i < text.size(); ++i) {
            ui->appendOutput(QString(text[i]), colors[i % colors.size()]);
        }
        ui->appendOutput("\n");
    }, "彩虹输出", "lolcat [文本]");

    // ascii
    registerCommand("ascii", [ui](const QStringList& args) {
        QMap<QString, QString> arts;
        arts["cat"] = "  /\\_/\\\n ( o.o )\n  > ^ <\n";
        arts["heart"] = "  ❤️ ❤️ ❤️\n ❤️   ❤️   ❤️\n  ❤️   ❤️\n    ❤️\n";
        arts["dog"] = "  / \\__\n (    @\\___\n /         O\n/   (_____/\n/_____/   U\n";
        arts["fish"] = "      .\n  \\  |  /\n   \\ | /\n    \\|/\n~~~~~O~~~~~\n";
        arts["dragon"] = "            __====-_  _-====__\n"
                         "       _--^^^#####/     \\#####^^^--_\n"
                         "    _-^##########/ (  ) \\##########^-_\n"
                         "   -############/  |^^|  \\############-\n"
                         " _/############/   (@@)   \\############\\_\n"
                         "/#############(     ()     )#############\\\n"
                         "-------------\\              /-------------\n";

        if (!args.isEmpty() && arts.contains(args[0].toLower())) {
            ui->appendOutput(arts[args[0].toLower()]);
        } else {
            QStringList names;
            for (auto it = arts.begin(); it != arts.end(); ++it) {
                names.append(it.key());
            }
            ui->appendOutput("可用ASCII艺术: " + names.join(", ") + "\n");
        }
    }, "ASCII艺术", "ascii [名称]");

    // morse
    registerCommand("morse", [ui](const QStringList& args) {
        QMap<QChar, QString> code;
        code['A'] = ".-"; code['B'] = "-..."; code['C'] = "-.-."; code['D'] = "-..";
        code['E'] = "."; code['F'] = "..-."; code['G'] = "--."; code['H'] = "....";
        code['I'] = ".."; code['J'] = ".---"; code['K'] = "-.-"; code['L'] = ".-..";
        code['M'] = "--"; code['N'] = "-."; code['O'] = "---"; code['P'] = ".--.";
        code['Q'] = "--.-"; code['R'] = ".-."; code['S'] = "..."; code['T'] = "-";
        code['U'] = "..-"; code['V'] = "...-"; code['W'] = ".--"; code['X'] = "-..-";
        code['Y'] = "-.--"; code['Z'] = "--..";
        code['0'] = "-----"; code['1'] = ".----"; code['2'] = "..---"; code['3'] = "...--";
        code['4'] = "....-"; code['5'] = "....."; code['6'] = "-...."; code['7'] = "--...";
        code['8'] = "---.."; code['9'] = "----.";
        code[' '] = "/";

        QString text = args.isEmpty() ? "SOS" : args.join(' ').toUpper();
        QStringList result;
        for (const QChar& ch : text) {
            if (code.contains(ch)) {
                result.append(code[ch]);
            } else {
                result.append(QString(ch));
            }
        }
        ui->appendOutput(result.join(' ') + "\n", THEME.yellow.name());
    }, "摩斯电码", "morse [文本]");

    // prime
    registerCommand("prime", [ui](const QStringList& args) {
        int n = args.isEmpty() ? 100 : args[0].toInt();
        auto isPrime = [](int x) -> bool {
            if (x < 2) return false;
            for (int i = 2; i * i <= x; ++i) {
                if (x % i == 0) return false;
            }
            return true;
        };
        QList<int> primes;
        for (int i = 2; i <= n; ++i) {
            if (isPrime(i)) primes.append(i);
        }
        ui->appendOutput(QString("共 %1 个素数\n").arg(primes.size()), THEME.green.name());
        for (int i = 0; i < primes.size(); ++i) {
            ui->appendOutput(QString("%1 ").arg(primes[i], 5));
            if ((i + 1) % 10 == 0) ui->appendOutput("\n");
        }
        ui->appendOutput("\n");
    }, "素数生成", "prime [上限]");

    // factor
    registerCommand("factor", [ui](const QStringList& args) {
        if (args.isEmpty()) return;
        int n = args[0].toInt();
        QList<int> factors;
        int d = 2;
        int num = n;
        while (d * d <= num) {
            while (num % d == 0) {
                factors.append(d);
                num /= d;
            }
            d++;
        }
        if (num > 1) factors.append(num);
        QStringList strFactors;
        for (int f : factors) strFactors.append(QString::number(f));
        ui->appendOutput(QString("%1: %2\n").arg(n).arg(strFactors.join(' ')));
    }, "因数分解", "factor <数字>");

    // fibonacci
    registerCommand("fibonacci", [ui](const QStringList& args) {
        int n = args.isEmpty() ? 10 : args[0].toInt();
        qint64 a = 0, b = 1;
        QStringList fibs;
        for (int i = 0; i < n; ++i) {
            fibs.append(QString::number(a));
            qint64 c = a + b;
            a = b;
            b = c;
        }
        ui->appendOutput(fibs.join(' ') + "\n");
    }, "斐波那契数列", "fibonacci [项数]");

    // pi
    registerCommand("pi", [ui](const QStringList& args) {
        int d = args.isEmpty() ? 10 : args[0].toInt();
        ui->appendOutput(QString::number(M_PI, 'f', d) + "\n");
    }, "圆周率", "pi [小数位]");

    // e
    registerCommand("e", [ui](const QStringList& args) {
        int d = args.isEmpty() ? 10 : args[0].toInt();
        ui->appendOutput(QString::number(M_E, 'f', d) + "\n");
    }, "自然常数", "e [小数位]");

    // phi
    registerCommand("phi", [ui](const QStringList& args) {
        int d = args.isEmpty() ? 10 : args[0].toInt();
        double phi = (1.0 + std::sqrt(5.0)) / 2.0;
        ui->appendOutput(QString::number(phi, 'f', d) + "\n");
    }, "黄金比例", "phi [小数位]");
}