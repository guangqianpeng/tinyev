//
// Created by frank on 17-8-31.
//

#ifndef TINYEV_NONCOPYABLE_H
#define TINYEV_NONCOPYABLE_H

namespace tinyev
{

class noncopyable
{
protected:
	noncopyable() = default;
	~noncopyable() = default;
private:
	noncopyable(const noncopyable&) = delete;
	void operator=(const noncopyable&) = delete;
};

}

#endif //TINYEV_NONCOPYABLE_H
