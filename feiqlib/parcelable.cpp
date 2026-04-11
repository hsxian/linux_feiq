#include "parcelable.h"

void Parcel::writeString(const string &val)
{
    writePtr(val.c_str(), val.length());
}

void Parcel::readString(string &val)
{
    auto size = nextSize();
    unique_ptr<char[]> buf(new char[size + 1]);
    readPtr(buf.get());
    buf[size] = 0;
    val = buf.get();
}

void Parcel::resetForRead()
{
    ss.seekg(0, ss.beg);
}

void Parcel::fillWith(const void *data, int len)
{
    ss.clear();
    ss.write(static_cast<const char *>(data), len);
}

streampos Parcel::mark()
{
    return ss.tellg();
}

void Parcel::unmark(streampos markPos)
{
    ss.seekg(markPos);
}

vector<char> Parcel::raw()
{
    auto size = ss.tellp();
    resetForRead();

    vector<char> buf(size);
    ss.read(buf.data(), size);

    return buf;
}

int Parcel::nextSize()
{
    auto pos = mark();
    Head head(ss);
    unmark(pos);
    return head.size;
}

Parcel::Head::Head(int size)
{
    this->size = size;
}

Parcel::Head::Head(istream &ss)
{
    read(ss);
}

void Parcel::Head::myWriteInt(ostream &os, int val)
{
    char buf[9] = {0};
    snprintf(buf, sizeof(buf), "%08d", val);
    os << buf;
}

int Parcel::Head::myReadInt(istream &is)
{
    char buf[9] = {0};
    is.read(buf, sizeof(buf) - 1);
    return stoi(buf);
}

void Parcel::Head::write(ostream &os)
{
    myWriteInt(os, size);
}

void Parcel::Head::read(istream &is)
{
    size = myReadInt(is);
}
