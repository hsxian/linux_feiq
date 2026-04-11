#ifndef FELLOW_H
#define FELLOW_H

#include <string>
using namespace std;

#define MYSELF_IP "127.0.0.1"

class Fellow
{
public:
    string getIp() const;
    string getName() const;
    string getHost() const;
    string getMac() const;
    bool isOnLine() const;
    string version() const;

    void setIp(const string &value);

    void setName(const string &value);

    void setHost(const string &value);

    void setMac(const string &value);

    void setOnLine(bool value);

    void setVersion(const string &value);

    void setPcName(const string &value);

    bool update(const Fellow &fellow);

    bool operator == (const Fellow &fellow);

    bool isSame(const Fellow &fellow);

    string toString() const;

    ulong getId() const;
    void setId(ulong newId);

private:
    ulong id;
    string mIp;
    string mPcName;
    string mName;
    string mHost;
    string mMac;
    bool mOnLine = false;
    string mVersion;
};

#endif // FELLOW_H
