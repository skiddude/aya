

#include <QString>
#include <QTime>


#pragma once

/**
 * Time a block of code.
 *  Note that the code will be in a new block/scope!
 *
 *  Example usage:
 *
 *  TIME_CODE
 *  (
 *      "Total Run Time: %1\n",
 *      SomeLongMethod();
 *  );
 */
#define TIME_CODE(message, code)                                               \
  {                                                                            \
    ScopedDebugTimer timer(message);                                           \
    code;                                                                      \
  }

/**
 * Outputs time between construction and destruction of object.
 */
class ScopedDebugTimer {
public:
  /**
   * Start scoped timer.
   *
   * @param   message add a %1 to the location in the message you want the time
   * printed
   */
  ScopedDebugTimer(const QString &message) : m_Message(message) {
    m_Time.start();
  }

  /**
   * Outputs time of object life.
   */
  ~ScopedDebugTimer()

      printf("%s", qPrintable(m_Message.arg(m_Time.elapsed())));
}

private : QString m_Message;
QTime m_Time;
}
;
