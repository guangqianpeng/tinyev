//
// Created by frank on 17-11-17.
//

#include <tinyev/Logger.h>
#include <tinyev/EventLoop.h>

using namespace ev;

int main()
{
    EventLoop loop;

    loop.runEvery(1s, [](){
        INFO("run every 1s");
    });
    loop.runAfter(10s, [&](){
        INFO("end after 10s");
        loop.quit();
    });
    loop.runAt(clock::nowAfter(15min), [&](){
        INFO("run 15min later");
    });

    INFO("start looping");
    loop.loop();
}