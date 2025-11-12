#pragma once
#include <QWidget>

class QLineEdit;
class QPushButton;
class QGridLayout;
class QHBoxLayout;

class Keypad : public QWidget
{
    Q_OBJECT
public:
    explicit Keypad(QWidget *parent = nullptr);
    ~Keypad() override;

private slots:
    void onDigit();
    void onOperator();
    void onDot();
    void onEqual();
    void onClear();
    void onBackspace();
    void onParenLeft();
    void onParenRight();
    void onToggleTheme();

private:
    void appendText(const QString &s, bool beep = true);
    void applyLightTheme();
    void applyDarkTheme();
    void speak(const QString& zh);     // ← 用 Windows 語音把文字講出來

    QLineEdit   *display_;
    QGridLayout *grid_;
    QHBoxLayout *topbar_;
    bool darkMode_ = false;
};
