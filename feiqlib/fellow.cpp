#include "fellow.h"
#include <memory>
#include <sstream>

string Fellow::getIp() const
{
    return mIp;
}

string Fellow::getName() const
{
    return mName.empty() ? mPcName : mName;
}

string Fellow::getHost() const
{
    return mHost;
}

string Fellow::getMac() const
{
    return mMac;
}

bool Fellow::isOnLine() const
{
    return mOnLine;
}

string Fellow::version() const
{
    return mVersion;
}

void Fellow::setIp(const string &value)
{
    mIp = value;
}

void Fellow::setName(const string &value)
{
    mName = value;
}

void Fellow::setHost(const string &value)
{
    mHost = value;
}

void Fellow::setMac(const string &value)
{
    mMac = value;
}

void Fellow::setOnLine(bool value)
{
    mOnLine = value;
}

void Fellow::setVersion(const string &value)
{
    mVersion = value;
}

void Fellow::setPcName(const string &value)
{
    mPcName = value;
}

bool Fellow::update(const Fellow &fellow)
{
    bool changed = false;

    if (!fellow.mName.empty() && mName != fellow.mName)
    {
        mName = fellow.mName;
        changed = true;
    }

    if (!fellow.mMac.empty() && mMac != fellow.mMac)
    {
        mMac = fellow.mMac;
        changed = true;
    }

    if (mOnLine != fellow.mOnLine)
    {
        mOnLine = fellow.mOnLine;
        changed = true;
    }

    return changed;
}

bool Fellow::operator ==(const Fellow &fellow)
{
    return isSame(fellow);
}

bool Fellow::isSame(const Fellow &fellow)
{
    return mIp == fellow.mIp || (!mMac.empty() && mMac == fellow.mMac);
}

string Fellow::toString() const
{
    ostringstream os;
    os << "["
       << "id=" << id
       << "ip=" << mIp
       << ",name=" << mName
       << ",host=" << mHost
       << ",pcname=" << mPcName
       << ",mac=" << mMac
       << ",online=" << mOnLine
       << ",version=" << mVersion
       << "]";
    return os.str();
}

ulong Fellow::getId() const
{
    return id;
}

void Fellow::setId(ulong newId)
{
    id = newId;
}
