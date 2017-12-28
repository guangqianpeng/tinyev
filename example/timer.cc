//
// Created by frank on 17-11-17.
//

#include <cstdio>

#include <tinyev/Logger.h>
#include <tinyev/EventLoop.h>

using namespace tinyev;

int main()
{
    EventLoop loop;

    loop.runEvery(1s, [](){
       INFO("run every 500ms");
    });
    loop.runAfter(5s, [&](){
        INFO("end after 1min");
        loop.quit();
    });
    loop.runAt(Clock::nowAfter(15min), [&](){
        INFO("run 15min later");
    });
    INFO("start loop");
    loop.loop();
}