#include <QObject>
#include "input_common/sdl/sdl.h"

class JoystickEventTicker : public QObject {
    Q_OBJECT
public:
    JoystickEventTicker(QObject* pParent = nullptr) : QObject(pParent) {}

public slots:

    void launch() {
        tickNext();
    }

private slots:

    void tick() {
        InputCommon::SDL::PollEvent();
        // Continue ticking
        tickNext();
    }

private:
    void tickNext() {
        // Trigger the tick() invokation when the event loop runs next time
        QMetaObject::invokeMethod(this, "tick", Qt::QueuedConnection);
    }
};
