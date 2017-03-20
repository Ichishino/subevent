Subevent
========

C++ Event Driven and Network Application Library

### Features
* Event driven App/Thread.
* Async TCP/UDP api.
* Header only.
* C++11, Linux/Windows.
* Easy to use.

### Example
* A Simple Application
```C++
#include "subevent.hpp"

SEV_IMPL_GLOBAL

SEV_USING_NS

//---------------------------------------------------------------------------//
// MyApp
//---------------------------------------------------------------------------//

class MyApp : public Application
{
public:
    using Application::Application;

protected:
    bool onInit() override
    {
        Application::onInit();

        // Your initialization code here.

        return true;
    }

    void onExit() override
    {
        // Your deinitialization code here.

        Application::onExit();
    }
};

//---------------------------------------------------------------------------//
// Main
//---------------------------------------------------------------------------//

int main(int argc, char** argv)
{
    MyApp app(argc, argv);
    return app.run();
}
```
* How to End MyApp
```C++
// call "void Application::stop()"
stop();
```
* Timer
```C++
Timer* timer = new Timer();

uint32_t msec = 60000;
bool repeat = false;

// start
timer->start(msec, repeat, [&](Timer*) {
    // Called from the same thread when time out.
    // Your code here.
});
```
* Network  
Please see the files under [subevent/examples/](https://github.com/Ichishino/subevent/tree/master/examples) directory.  
To build on Linux.  
In each directory:  
```
$ cmake .
$ make
```
