#ifndef ANDROIDKEYPAD_H
#define ANDROIDKEYPAD_H

// In-app keyboards for the connection fields. The system keyboard on these
// devices is a Japanese IME that composes full-width text (and on the phone
// the host field never brought it up at all), so the host, user and password
// fields are read-only and a tap opens one of these instead: digits, "." ,
// Back and クリア for the host; half-width letters, digits and a few symbols
// for the user name and password. Keys edit the field directly (so its
// textChanged handlers run as usual); tapping outside the keyboard closes it.
// Key sizes follow the field's height, so the same code fits the phone and
// the (scaled) tablet settings page. Same as the phone version's keypads.

#include <QEvent>
#include <QFrame>
#include <QGridLayout>
#include <QLineEdit>
#include <QList>
#include <QMouseEvent>
#include <QPushButton>
#include <QWidget>
#include <functional>

class AndroidKeypadBackdrop : public QWidget
{
public:
    explicit AndroidKeypadBackdrop(QWidget *parent) : QWidget(parent)
    {
        setAttribute(Qt::WA_DeleteOnClose);
        setGeometry(parent->rect());
        // The app style sheet gives every QWidget a solid background; keep
        // this full-window tap catcher see-through so the form stays visible.
        setObjectName(QStringLiteral("androidKeypadBackdrop"));
        setStyleSheet(QStringLiteral("#androidKeypadBackdrop{background:transparent;}"));
    }
protected:
    void mousePressEvent(QMouseEvent *) override { close(); }
};

class AndroidKeypadOpener : public QObject
{
public:
    AndroidKeypadOpener(QLineEdit *edit, bool ipOnly) : QObject(edit), edit(edit), ipOnly(ipOnly)
    {
        edit->setReadOnly(true);
        edit->setFocusPolicy(Qt::NoFocus);
        edit->installEventFilter(this);
    }

protected:
    bool eventFilter(QObject *obj, QEvent *ev) override
    {
        if (obj == edit && ev->type() == QEvent::MouseButtonRelease) {
            openKeypad();
            return true;
        }
        return QObject::eventFilter(obj, ev);
    }

private:
    void openKeypad()
    {
        QWidget *win = edit->window();
        AndroidKeypadBackdrop *backdrop = new AndroidKeypadBackdrop(win);
        QFrame *pad = new QFrame(backdrop);
        pad->setFrameShape(QFrame::Box);
        pad->setAutoFillBackground(true);
        const int h = qMax(24, edit->height());
        QGridLayout *grid = new QGridLayout(pad);
        grid->setContentsMargins(h / 5, h / 5, h / 5, h / 5);
        grid->setSpacing(h / 8);
        QFont f = edit->font();
        if (f.pointSizeF() > 0)
            f.setPointSizeF(f.pointSizeF() * (ipOnly ? 1.4 : 1.2));
        QLineEdit *e = edit;
        auto addKey = [&](const QString &label, int row, int col, int colSpan,
                          std::function<void()> action) -> QPushButton * {
            QPushButton *b = new QPushButton(label, pad);
            b->setFont(f);
            b->setFocusPolicy(Qt::NoFocus);
            b->setMinimumSize(ipOnly ? h * 185 / 100 : h * 120 / 100, ipOnly ? h * 110 / 100 : h);
            grid->addWidget(b, row, col, 1, colSpan);
            if (action)
                QObject::connect(b, &QPushButton::clicked, pad, action);
            return b;
        };
        auto addChar = [&](const QString &ch, int row, int col) -> QPushButton * {
            QPushButton *b = addKey(ch, row, col, 1, Q_NULLPTR);
            QObject::connect(b, &QPushButton::clicked, pad, [e, b]() { e->setText(e->text() + b->text()); });
            return b;
        };
        auto back = [e]() { if (!e->text().isEmpty()) e->setText(e->text().chopped(1)); };
        auto clear = [e]() { e->clear(); };

        if (ipOnly) {
            const char *digits[3][3] = { {"7", "8", "9"}, {"4", "5", "6"}, {"1", "2", "3"} };
            for (int r = 0; r < 3; r++)
                for (int c = 0; c < 3; c++)
                    addChar(QString::fromLatin1(digits[r][c]), r, c);
            addChar(QStringLiteral("."), 3, 0);
            addChar(QStringLiteral("0"), 3, 1);
            addKey(QStringLiteral("Back"), 3, 2, 1, back);
            addKey(QStringLiteral("クリア"), 4, 0, 3, clear);
        } else {
            const char *rows[4] = { "1234567890", "qwertyuiop", "asdfghjkl-", "zxcvbnm._@" };
            QList<QPushButton *> letters;
            for (int r = 0; r < 4; r++)
                for (int c = 0; c < 10; c++) {
                    QPushButton *b = addChar(QString(QLatin1Char(rows[r][c])), r, c);
                    if (QChar::fromLatin1(rows[r][c]).isLetter())
                        letters.append(b);
                }
            QPushButton *shift = addKey(QStringLiteral("大文字"), 4, 0, 3, Q_NULLPTR);
            shift->setCheckable(true);
            QObject::connect(shift, &QPushButton::toggled, pad, [letters](bool upper) {
                for (QPushButton *b : letters)
                    b->setText(upper ? b->text().toUpper() : b->text().toLower());
            });
            addKey(QStringLiteral("Back"), 4, 3, 3, back);
            addKey(QStringLiteral("クリア"), 4, 6, 4, clear);
        }

        // Right side of the window, at the field's height if it fits, so the
        // field stays visible to the left of the keyboard.
        pad->adjustSize();
        const QPoint pos = edit->mapTo(win, QPoint(0, 0));
        const int x = win->width() - pad->width() - 8;
        const int y = qMax(0, qMin(pos.y(), win->height() - pad->height() - 8));
        pad->move(x, y);
        backdrop->show();
        backdrop->raise();
    }

    QLineEdit *edit;
    bool ipOnly;
};

#endif // ANDROIDKEYPAD_H
