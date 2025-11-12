#include <QApplication>
#include "Keypad.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    Keypad keypad;
    keypad.show();
    return app.exec();
}
