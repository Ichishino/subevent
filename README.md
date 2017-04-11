Subevent
========

C++ Event Driven and Network Application Library

### Features
* Event driven App/Thread.
* Async TCP/UDP client and server.
* Async HTTP client.
* Multiple TCP connections in single thread.
* Header only.
* C++11, Linux/Windows.
* Easy to use.

### Compile Options
* `-std=c++11`
* `-pthread` or `-lpthread`

### Example
* A Simple Application
```C++
#include "subevent.hpp"

SEV_USING_NS

//---------------------------------------------------------------------------//
// MyApp
//---------------------------------------------------------------------------//

class MyApp : public NetApplication
{
protected:
    bool onInit() override
    {
        NetApplication::onInit();

        // Your initialization code here.

        return true;
    }

    void onExit() override
    {
        // Your deinitialization code here.

        NetApplication::onExit();
    }
};

//---------------------------------------------------------------------------//
// Main
//---------------------------------------------------------------------------//

SEV_IMPL_GLOBAL

int main(int, char**)
{
    MyApp app;
    return app.run();
}
```
* How to End MyApp
```C++
// call "void NetApplication::stop()"
stop();
```
* Thread
```C++
class MyThread : public NetThread
{
protected:
    bool onInit() override
    {
        NetThread::onInit();

        // Your initialization code here.

        return true;
    }

    void onExit() override
    {
        // Your deinitialization code here.

        NetThread::onExit();
    }
};
```
```C++
// Start
MyThread* myThread = new MyThread();
myThread->start();
```
```C++
// End
myThread->stop();
myThread->wait();
delete myThread;
```
* Event Send/Receive
```C++
// Declare MyEvent
// <Unique Id, ParamType1, ParamType2, ...>
typedef UserEvent<1, int, std::string> MyEvent;
```
```C++
// Send (to another thread or self)
int param1 = 20;
std::string param2 = "data";
myThread->post(new MyEvent(param1, param2));
```
```C++
// Receive
setUserEventHandler<MyEvent>([&](const MyEvent* myEvent) {
    int param1;
    std::string param2;
    myEvent->getParams(param1, param2);
});
```
* Async Task
```C++
myThread->post([&]() {
    // Executed on myThread.
});
```
* Timer
```C++
Timer* timer = new Timer();

uint32_t msec = 60000;
bool repeat = false;

// start
timer->start(msec, repeat, [&](Timer*) {
    // Called from the same thread when time out.
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
* TODO  
HTTP/2  
HTTPS  
HTTP server  
etc  
