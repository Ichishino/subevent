#ifndef SUBEVENT_APPLICATION_INL
#define SUBEVENT_APPLICATION_INL

#include <cassert>

#include <subevent/application.hpp>

#ifdef SEV_OS_WIN
#include <windows.h>
#endif

SEV_NS_BEGIN

//----------------------------------------------------------------------------//
// Application
//----------------------------------------------------------------------------//

#ifndef SEV_HEADER_ONLY
Application* Application::gApp = nullptr;
#endif

Application::Application(Thread* /* unused */)
    : Application("")
{
}

Application::Application(const std::string& name, Thread* /* unused */)
    : Thread(name)
{
    assert(gApp == nullptr);
    assert(gThread == nullptr);

    gApp = this;
    gThread = this;

    mId = std::this_thread::get_id();
#ifdef SEV_OS_WIN
    mHandle = GetCurrentThread();
#else
    mHandle = pthread_self();
#endif
}

Application::~Application()
{
    if (gApp == this)
    {
        gApp = nullptr;
    }
}

void Application::setArgs(int32_t argc, char* argv[])
{
    for (int index = 0; index < argc; ++index)
    {
        mArgs.push_back(argv[index]);
    }
}

Application* Application::getCurrent()
{
    return gApp;
}

bool Application::onInit()
{
    return Thread::onInit();
}

void Application::onExit()
{
    Thread::onExit();
}

int Application::run()
{
    // init
    if (onInit())
    {
        // run
        if (!mEventLoop.run())
        {
            mExitCode = -2;
        }
    }
    else
    {
        mExitCode = -1;
    }

    // exit
    onExit();

    return mExitCode;
}

SEV_NS_END

#endif // SUBEVENT_APPLICATION_INL
