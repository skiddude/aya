#include "boost.hpp"
#include "threadsafe.hpp"
#include "Debug.hpp"
#include "atomic.hpp"



Aya::concurrency_catcher::scoped_lock::scoped_lock(concurrency_catcher& m)
    : m(m)
{
    if (m.value.swap(concurrency_catcher::locked) != concurrency_catcher::unlocked)
    {
        AYACRASH();
    }
}

Aya::concurrency_catcher::scoped_lock::~scoped_lock()
{
    m.value.swap(concurrency_catcher::unlocked);
}

const unsigned long Aya::reentrant_concurrency_catcher::noThreadId = 4493024;

Aya::reentrant_concurrency_catcher::scoped_lock::scoped_lock(Aya::reentrant_concurrency_catcher& m)
    : m(m)
{
    const long threadId(GetCurrentThreadId());
    isChild = threadId == m.threadId;
    if (isChild)
        return; // Ah ha! I'm a recursive lock request. Just go ahead :)

    if (m.value.swap(reentrant_concurrency_catcher::locked) != reentrant_concurrency_catcher::unlocked)
    {
        AYACRASH();
    }

    // We own the lock, so assign the current thread ID
    AYAASSERT(m.threadId == reentrant_concurrency_catcher::noThreadId);
    m.threadId = threadId;
}


Aya::reentrant_concurrency_catcher::scoped_lock::~scoped_lock()
{
    if (!isChild)
    {
        m.threadId = reentrant_concurrency_catcher::noThreadId;
        m.value.swap(reentrant_concurrency_catcher::unlocked);
    }
}


Aya::readwrite_concurrency_catcher::scoped_write_request::scoped_write_request(readwrite_concurrency_catcher& m)
    : m(m)
{
    if (m.write_requested.swap(readwrite_concurrency_catcher::locked) != readwrite_concurrency_catcher::unlocked)
        AYAASSERT(false); // should be a AYACRASH();
    if (m.read_requested > 0)
        AYAASSERT(false); // should be a AYACRASH();
}
Aya::readwrite_concurrency_catcher::scoped_write_request::~scoped_write_request()
{
    if (m.write_requested.swap(readwrite_concurrency_catcher::unlocked) != readwrite_concurrency_catcher::locked)
        AYAASSERT(false); // should be a AYACRASH();
}

// Place this code around tasks that write to a DataModel
Aya::readwrite_concurrency_catcher::scoped_read_request::scoped_read_request(readwrite_concurrency_catcher& m)
    : m(m)
{
    if (m.write_requested != readwrite_concurrency_catcher::unlocked)
        AYAASSERT(false); // should be a AYACRASH();
    ++m.read_requested;
}
Aya::readwrite_concurrency_catcher::scoped_read_request::~scoped_read_request()
{
    if (--m.read_requested < 0)
        AYAASSERT(false); // should be a AYACRASH();
}
