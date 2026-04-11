#ifndef CONTENT_H
#define CONTENT_H

#include "ipmsg.h"
#include "parcelable.h"
#include "uniqueid.h"
#include "utils.h"
#include <string>

using namespace std;
class ImageContent;
enum class ContentType
{
    Text,
    Knock,
    File,
    Image,
    Id
};

/**
 * @brief 消息内容
 */
class Content : public Parcelable
{
public:
    IdType packetNo;
    void setPacketNo(string val);
    void setPacketNo(IdType val);
    Content();
    virtual ~Content();
    ContentType type() const;

protected:
    ContentType mType;
    SysTimeMs sendTime;
public:
    virtual void writeTo(Parcel &out) const override;
    virtual void readFrom(Parcel &in) override;
    SysTimeMs getSendTime() const;
    virtual string simpleText();
};

class IdContent : public Content
{
public:
    IdContent();
    IdType id;
};

class TextContent : public Content
{
public:
    TextContent();
    string text;
    string format;

public:
    virtual void writeTo(Parcel &out) const override;
    virtual void readFrom(Parcel &in) override;
    virtual string simpleText() override;
};

class FileContent : public Content
{
public:
    FileContent();
    IdType fileId;
    string filename;
    string path; //保存路径或要发送的文件的路径
    int size = 0;
    int modifyTime = 0;
    int fileType = 0;

public:
    static unique_ptr<FileContent> createFileContentToSend(const string &filePath);
    virtual void writeTo(Parcel &out) const override;
    virtual void readFrom(Parcel &in) override;
    virtual string simpleText() override;
};

class KnockContent : public Content
{
public:
    KnockContent();
    virtual string simpleText() override;
};

class ImageContent : public FileContent
{
public:
    string id;
    ImageContent();
    ImageContent &operator=(const FileContent &b);
    static unique_ptr<ImageContent> createImageContentToSend(const string &filePath);
    virtual void writeTo(Parcel &out) const override;
    virtual void readFrom(Parcel &in) override;
};

class ContentParcelFactory
{
public:
    static unique_ptr<Content> createFromParcel(Parcel &in);
};

#endif // CONTENT_H
