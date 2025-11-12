#include "Keypad.h"

#include <QLineEdit>
#include <QPushButton>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QApplication>
#include <QJSEngine>
#include <QRegularExpression>
#include <QFont>
#include <QProcess>

static QPushButton* makeBtn(const QString& text,
                            Keypad* self,
                            void (Keypad::*slot)())
{
    auto *btn = new QPushButton(text, self);
    btn->setMinimumSize(60, 48);
    btn->setCursor(Qt::PointingHandCursor);
    QObject::connect(btn, &QPushButton::clicked, self, slot);
    return btn;
}

Keypad::Keypad(QWidget *parent)
    : QWidget(parent),
    display_(new QLineEdit(this)),
    grid_(new QGridLayout),
    topbar_(new QHBoxLayout)
{
    setWindowTitle("Keypad 計算機 (主題切換 + 語音播報)");
    resize(360, 520);

    // 顯示器
    display_->setReadOnly(true);
    display_->setAlignment(Qt::AlignRight);
    QFont f = display_->font(); f.setPointSize(18); display_->setFont(f);

    // 主題切換
    auto *themeBtn = new QPushButton("切換主題", this);
    themeBtn->setCursor(Qt::PointingHandCursor);
    connect(themeBtn, &QPushButton::clicked, this, &Keypad::onToggleTheme);
    topbar_->addWidget(themeBtn);
    topbar_->addStretch();

    // 功能鍵
    grid_->addWidget(makeBtn("C",  this, &Keypad::onClear),      0, 0);
    grid_->addWidget(makeBtn("⌫",  this, &Keypad::onBackspace),  0, 1);
    grid_->addWidget(makeBtn("(",  this, &Keypad::onParenLeft),  0, 2);
    grid_->addWidget(makeBtn(")",  this, &Keypad::onParenRight), 0, 3);

    // 數字/運算子
    grid_->addWidget(makeBtn("7", this, &Keypad::onDigit), 1, 0);
    grid_->addWidget(makeBtn("8", this, &Keypad::onDigit), 1, 1);
    grid_->addWidget(makeBtn("9", this, &Keypad::onDigit), 1, 2);
    grid_->addWidget(makeBtn("+", this, &Keypad::onOperator), 1, 3);

    grid_->addWidget(makeBtn("4", this, &Keypad::onDigit), 2, 0);
    grid_->addWidget(makeBtn("5", this, &Keypad::onDigit), 2, 1);
    grid_->addWidget(makeBtn("6", this, &Keypad::onDigit), 2, 2);
    grid_->addWidget(makeBtn("-", this, &Keypad::onOperator), 2, 3);

    grid_->addWidget(makeBtn("1", this, &Keypad::onDigit), 3, 0);
    grid_->addWidget(makeBtn("2", this, &Keypad::onDigit), 3, 1);
    grid_->addWidget(makeBtn("3", this, &Keypad::onDigit), 3, 2);
    grid_->addWidget(makeBtn("*", this, &Keypad::onOperator), 3, 3);

    grid_->addWidget(makeBtn("0", this, &Keypad::onDigit), 4, 0);
    grid_->addWidget(makeBtn(".", this, &Keypad::onDot),   4, 1);
    grid_->addWidget(makeBtn("=", this, &Keypad::onEqual), 4, 2);
    grid_->addWidget(makeBtn("/", this, &Keypad::onOperator), 4, 3);

    // 版面
    auto *root = new QVBoxLayout(this);
    root->addLayout(topbar_);
    root->addWidget(display_);
    root->addLayout(grid_);
    setLayout(root);

    applyLightTheme();
}

Keypad::~Keypad() = default;

// ---- Windows 系統語音 ----
void Keypad::speak(const QString &zh)
{
#ifdef Q_OS_WIN
    // 轉義單引號，避免 PowerShell 字串截斷
    QString text = zh;
    text.replace("'", "''");

    // PowerShell 腳本：優先用 zh-TW 女聲；若無就用預設聲音
    QString script =
        "Add-Type -AssemblyName System.Speech; "
        "$s = New-Object System.Speech.Synthesis.SpeechSynthesizer; "
        "try { "
        "  $ci = [System.Globalization.CultureInfo]::GetCultureInfo('zh-TW'); "
        "  $s.SelectVoiceByHints([System.Speech.Synthesis.VoiceGender]::Female, "
        "                          [System.Speech.Synthesis.VoiceAge]::Adult, 0, $ci); "
        "} catch { } "
        "$s.Rate = 0; $s.Volume = 100; "
        "$s.Speak('" + text + "');";

    QProcess::startDetached("powershell", QStringList() << "-NoProfile" << "-Command" << script);
#else
    // 非 Windows：至少保留一個聲響（你也可改成調用 macOS `say` 或 Linux `espeak`）
    QApplication::beep();
#endif
}
// --------------------------

void Keypad::appendText(const QString &s, bool beep)
{
    display_->setText(display_->text() + s);
    if (beep) QApplication::beep();
}

void Keypad::onDigit()
{
    auto *btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;
    const QString text = btn->text();
    appendText(text);
    speak(text);                 // 按「3」就講「3」
}

void Keypad::onOperator()
{
    auto *btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;

    QString t = display_->text();
    if (t.endsWith(" + ") || t.endsWith(" - ") || t.endsWith(" * ") || t.endsWith(" / ")) {
        t.chop(3);
        display_->setText(t + " " + btn->text() + " ");
    } else {
        appendText(" " + btn->text() + " ");
    }
    // 播報中文
    speak(btn->text() == "+" ? "加" :
              btn->text() == "-" ? "減" :
              btn->text() == "*" ? "乘" : "除");
}

void Keypad::onDot()           { appendText("."); speak("點"); }
void Keypad::onEqual()
{
    speak("等於");
    QString expr = display_->text();
    QString jsExpr = expr; jsExpr.remove(QRegularExpression("\\s+"));
    if (jsExpr.isEmpty()) return;

    QJSEngine eng;
    QJSValue v = eng.evaluate(jsExpr);
    if (v.isError()) { display_->setText("Error"); speak("錯誤"); return; }

    double num = v.toNumber();
    display_->setText(QString::number(num, 'g', 12));
    speak(display_->text());   // 播報結果
}
void Keypad::onClear()         { display_->clear(); speak("清除"); }
void Keypad::onBackspace()
{
    QString t = display_->text();
    if (t.isEmpty()) { speak("退格"); return; }
    if (t.size() >= 3 && (t.endsWith(" + ") || t.endsWith(" - ") || t.endsWith(" * ") || t.endsWith(" / "))) t.chop(3);
    else t.chop(1);
    display_->setText(t);
    speak("退格");
}
void Keypad::onParenLeft()     { appendText("("); speak("左括號"); }
void Keypad::onParenRight()    { appendText(")"); speak("右括號"); }

void Keypad::onToggleTheme()
{
    darkMode_ = !darkMode_;
    if (darkMode_) applyDarkTheme(); else applyLightTheme();
    speak("切換主題");
}

void Keypad::applyLightTheme()
{
    setStyleSheet(R"(
        QWidget { background:#f7f7f7; color:#111; }
        QLineEdit { background:white; border:1px solid #cfcfcf; border-radius:8px; padding:8px; }
        QPushButton { background:#ffffff; border:1px solid #d0d0d0; border-radius:10px; padding:6px; }
        QPushButton:hover { background:#fafafa; }
        QPushButton:pressed { background:#f0f0f0; }
    )");
}

void Keypad::applyDarkTheme()
{
    setStyleSheet(R"(
        QWidget { background:#14161a; color:#eaeaea; }
        QLineEdit { background:#1e2126; color:#eaeaea; border:1px solid #2e3238; border-radius:8px; padding:8px; }
        QPushButton { background:#1b1e23; color:#eaeaea; border:1px solid #30343b; border-radius:10px; padding:6px; }
        QPushButton:hover { background:#22262c; }
        QPushButton:pressed { background:#262b32; }
    )");
}
