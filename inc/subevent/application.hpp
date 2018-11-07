#ifndef SUBEVENT_APPLICATION_HPP
#define SUBEVENT_APPLICATION_HPP

#include <string>
#include <vector>

#include <subevent/std.hpp>
#include <subevent/thread.hpp>

SEV_NS_BEGIN

//----------------------------------------------------------------------------//
// Application
//----------------------------------------------------------------------------//

class Application : public Thread
{
public:
    SEV_DECL explicit Application(Thread* unused = nullptr);
    SEV_DECL explicit Application(
        const std::string& name, Thread* unused = nullptr);
    SEV_DECL virtual ~Application() override;

public:
    SEV_DECL int32_t run();

    SEV_DECL void setArgs(int32_t argc, char* argv[]);

    SEV_DECL const std::vector<std::string>& getArgs() const
    {
        return mArgs;
    }

    SEV_DECL static Application* getCurrent();

protected:
    SEV_DECL bool onInit() override;
    SEV_DECL void onExit() override;

private:
    static Application* gApp;

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;
    void start() = delete;
    void wait() = delete;

    std::vector<std::string> mArgs;
};

SEV_NS_END

#endif // SUBEVENT_APPLICATION_HPP
