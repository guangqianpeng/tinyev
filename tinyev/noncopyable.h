//
// Created by frank on 17-8-31.
//

#ifndef TINYEV_NONCOPYABLE_H
#define TINYEV_NONCOPYABLE_H

namespace ev
{

class noncopyable
{
public:
    noncopyable(const noncopyable&) = delete;
    void operator=(const noncopyable&) = delete;

protected:
    noncopyable() = default;
    ~noncopyable() = default;
};

}

#endif //TINYEV_NONCOPYABLE_H
