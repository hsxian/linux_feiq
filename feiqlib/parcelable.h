#ifndef PARCELABLE_H
#define PARCELABLE_H

#include <iostream>
#include <memory.h>
#include <memory>
#include <sstream>
#include <vector>

using namespace std;

class Parcel
{
private:
    class Head
    {
    public:
        Head(int size);

        Head(istream &ss);

        int size;

        void myWriteInt(ostream &os, int val);

        int myReadInt(istream &is);

        void write(ostream &os);

        void read(istream &is);
    };

public:
    template <typename T>
    void write(const T &val)
    {
        writePtr(&val, 1);
    }

    template <typename T>
    void read(T &val)
    {
        readPtr(&val);
    }

    template <typename T>
    void writePtr(const T *ptr, int n)
    {
        Head head{(int)(n * sizeof(T))};
        head.write(ss);
        ss.write((const char *)ptr, head.size);
    }

    template <typename T>
    void readPtr(T *ptr)
    {
        Head head(ss);
        unique_ptr<char[]> buf(new char[head.size]);
        ss.read(buf.get(), head.size);
        memcpy(ptr, buf.get(), head.size);
    }

    void writeString(const string &val);

    void readString(string &val);

    void resetForRead();

    void fillWith(const void *data, int len);

public:
    streampos mark();

    void unmark(streampos markPos);

public:
    vector<char> raw();

private:
    int nextSize();

private:
    stringstream ss;
};

class Parcelable
{
public:
    virtual void writeTo(Parcel &out) const = 0;
    virtual void readFrom(Parcel &in) = 0;
};

#endif // PARCELABLE_H
