#include "content.h"
#include "protocol.h"
#include <sys/stat.h>

void Content::setPacketNo(string val)
{
    packetNo = stoul(val);
}

void Content::setPacketNo(IdType val)
{
    packetNo = val;
}

Content::~Content() {}

ContentType Content::type() const
{
    return mType;
}

SysTimeMs Content::getSendTime() const
{
    return sendTime;
}

string Content::simpleText()
{
    return "***";
}

void Content::writeTo(Parcel &out) const
{
    out.write(mType);
    out.write(packetNo);
    out.write(sendTime);
}

void Content::readFrom(Parcel &in)
{
    in.read(mType);
    in.read(packetNo);
    in.read(sendTime);
}

IdContent::IdContent()
{
    mType = ContentType::Id;
}

TextContent::TextContent()
{
    mType = ContentType::Text;
}

string TextContent::simpleText()
{
    return text;
}
void TextContent::writeTo(Parcel &out) const
{
    Content::writeTo(out);
    out.writeString(text);
    out.writeString(format);
}

void TextContent::readFrom(Parcel &in)
{
    Content::readFrom(in);
    in.readString(text);
    in.readString(format);
}

FileContent::FileContent()
{
    mType = ContentType::File;
}

string FileContent::simpleText()
{
    return filename;
}

unique_ptr<FileContent> FileContent::createFileContentToSend(const string &filePath)
{
    static UniqueId mFileId;
    struct stat fInfo;
    auto ret = stat(filePath.c_str(), &fInfo);
    if (ret != 0)
        return nullptr;

    unique_ptr<FileContent> file(new FileContent());
    file->fileId = mFileId.get();
    file->path = filePath;
    file->filename = getFileNameFromPath(filePath);
    if (S_ISREG(fInfo.st_mode))
        file->fileType = IPMSG_FILE_REGULAR;
    else if (S_ISREG(fInfo.st_mode))
        file->fileType = IPMSG_FILE_DIR;
    else
        return nullptr; //先不支持其他类型
    file->size = fInfo.st_size;
    file->modifyTime = fInfo.st_mtim.tv_sec;

    return file;
}

void FileContent::writeTo(Parcel &out) const
{
    Content::writeTo(out);
    out.write(fileId);
    out.writeString(filename);
    out.writeString(path);
    out.write(fileType);
    out.write(size);
    out.write(modifyTime);
}

void FileContent::readFrom(Parcel &in)
{
    Content::readFrom(in);
    in.read(fileId);
    in.readString(filename);
    in.readString(path);
    in.read(fileType);
    in.read(size);
    in.read(modifyTime);
}

KnockContent::KnockContent()
{
    mType = ContentType::Knock;
}

string KnockContent::simpleText()
{
    return "窗口抖动";
}

ImageContent::ImageContent()
{
    mType = ContentType::Image;
}

unique_ptr<Content> ContentParcelFactory::createFromParcel(Parcel &in)
{
    //先读取父类信息
    Content content;
    auto pos = in.mark();
    content.readFrom(in);
    in.unmark(pos);

    //根据类型读取剩余数据
    Content *ptr = nullptr;
    switch (content.type())
    {
    case ContentType::Text:
        ptr = new TextContent;
        break;
    case ContentType::Knock:
        ptr = new KnockContent;
        break;
    case ContentType::File:
        ptr = new FileContent;
        break;
    case ContentType::Image:
        ptr = new ImageContent;
        break;
    case ContentType::Id:
        ptr = new IdContent;
        break;
    default:
        break;
    }

    if (ptr)
        ptr->readFrom(in);

    return unique_ptr<Content>(ptr);
}

Content::Content()
{
    sendTime = time_point_cast<milliseconds>(system_clock::now());
}
