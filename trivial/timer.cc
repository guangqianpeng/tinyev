//
// Created by frank on 17-11-17.
//

#include <cstdio>

#include "Logger.h"
#include "EventLoop.h"

using namespace tinyev;

int main()
{
    EventLoop loop;

    loop.runEvery(1s, [](){
       INFO("run every 500ms");
    });
    loop.runAfter(1min, [&](){
        INFO("end after 1min");
        loop.quit();
    });
    INFO("start loop");
    loop.loop();
}