#include "recvtextedit.h"
#include <QDate>
#include "emoji.h"
#include <QMouseEvent>
#include "feiqlib/history.h"

RecvTextEdit::RecvTextEdit(QWidget *parent)
    : QTextEdit(parent)
{
    setTextInteractionFlags(Qt::LinksAccessibleByMouse | Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
}

void RecvTextEdit::mousePressEvent(QMouseEvent *e)
{
    mPressedAnchor =  (e->button() & Qt::LeftButton) ? anchorAt(e->pos()) : "";
    QTextEdit::mousePressEvent(e);
}

void RecvTextEdit::mouseReleaseEvent(QMouseEvent *e)
{
    if (e->button() & Qt::LeftButton)
    {
        if (anchorAt(e->pos()) == mPressedAnchor && !mPressedAnchor.isEmpty())
            parseLink(mPressedAnchor);
    }
    QTextEdit::mouseReleaseEvent(e);
}

void RecvTextEdit::addFellowContent(const Content *content, long long msSinceEpoch)
{
    addContent(content, msSinceEpoch, false);
}

void RecvTextEdit::addMyContent(const Content *content, long long msSinceEpoch)
{
    addContent(content, msSinceEpoch, true);
}

void RecvTextEdit::addContent(const Content *content, long long msSinceEpoch, bool mySelf)
{
    auto align = alignment();
    // 设置对齐方式
    if (mySelf)
    {
        setAlignment(Qt::AlignRight);
    }
    else
    {
        setAlignment(Qt::AlignLeft);
    }

    drawDaySeperatorIfNewDay(msSinceEpoch);

    showHint(msSinceEpoch, mySelf);

    showContent(content, mySelf);
    append("");
    moveCursor(QTextCursor::End);

    // 重置对齐方式为默认的左对齐
    setAlignment(align);
}

void RecvTextEdit::addHistoryContent(const HistoryRecord &record)
{
    auto time = timeToLong(record.time);
    //判断消息是自己发送的还是对方发送的
    auto content = record.what.get();
    addContent(content, time, record.sender.get()->getIp() == MYSELF_IP);

    if (record.eventType == ViewEventType::SEND_TIMEO)
    {
        addWarning("发送超时:" + QString::fromStdString(content->simpleText()));
    }
}

void RecvTextEdit::showHint(long long msSinceEpoch, bool mySelf)
{
    QString name("");
    QString color("black");
    if (mySelf)
    {
        name = "我";
        color = "blue";
    }
    else
    {
        name = mFellow == nullptr ? "匿名" : mFellow->getName().c_str();
        color = "green";
    }

    QString hint = "<font color=" + color + ">" + name + " " + timeStr(msSinceEpoch) + "</font>";

    moveCursor(QTextCursor::End);
    insertHtml(hint);
    append("\n");
}

void RecvTextEdit::setCurFellow(const Fellow *fellow)
{
    if (mFellow)
        mDocs[mFellow] = document()->clone();//document将被清除或删除了，需clone

    auto it = mDocs.find(fellow);
    if (it != mDocs.end())
    {
        setDocument((*it).second);
        moveCursor(QTextCursor::End);
    }
    else
    {
        clear();
    }

    mFellow = fellow;
}

void RecvTextEdit::addWarning(const QString &warning)
{
    auto align = alignment();
    setAlignment(Qt::AlignCenter);
    auto color = textColor();
    QColor c(128, 128, 128);
    setTextColor(c);
    append(textHtmlStr( warning, c));
    append("");
    //结束当前段落，否则下一行恢复对齐方式时会将刚append的内容左对齐
    setAlignment(align);
    setTextColor(color);
}

const Fellow *RecvTextEdit::curFellow()
{
    return mFellow;
}

void RecvTextEdit::parseLink(const QString &link)
{
    QStringList parts = link.split("_");
    if (parts.count() < 3)
        return;

    auto packetNo = parts.at(0).toLongLong();
    auto fileId = parts.at(1).toLongLong();
    bool upload = parts.at(2) == "up";

    emit navigateToFileTask(packetNo, fileId, upload);
}

QString RecvTextEdit::timeStr(long long msSinceEpoch)
{
    QDateTime time;
    time.setMSecsSinceEpoch(msSinceEpoch);
    return time.toString("MM-dd HH:mm:ss");
}

void RecvTextEdit::showContent(const Content *content, bool mySelf)
{
    switch (content->type())
    {
    case ContentType::File:
        showFile(static_cast<const FileContent *>(content), mySelf);
        break;
    case ContentType::Knock:
        showKnock(static_cast<const KnockContent *>(content), mySelf);
        break;
    case ContentType::Image:
        showImage(static_cast<const ImageContent *>(content));
        break;
    case ContentType::Text:
        showText(static_cast<const TextContent *>(content));
        break;
    default:
        showUnSupport();
        break;
    }
}

void RecvTextEdit::showFile(const FileContent *content, bool fromMySelf)
{
    if (content->fileType == IPMSG_FILE_REGULAR || content->fileType == IPMSG_NOOPERATION)
    {
        stringstream ss;
        ss << "<a href=" << content->packetNo << "_" << content->fileId << "_" << (fromMySelf ? "up" : "down") << ">"
           << content->filename << "(" << content->size << ")"
           << "</a>";
        insertHtml(ss.str().c_str());
    }
    else
    {
        showUnSupport("对方发来非普通文件（可能是文件夹），收不来……");
    }
}

void RecvTextEdit::showImage(const ImageContent *content)
{
    // 尝试加载图片
    QString imagePath = QString::fromStdString(content->path);
    QImage image(imagePath);
    
    if (!image.isNull())
    {
        // 图片加载成功，插入到文本编辑框中
        QTextCursor cursor = textCursor();
        QTextDocument *doc = document();
        
        // 生成唯一的URL用于图片资源
        QString imageUrl = QString("image_%1").arg(content->fileId);
        doc->addResource(QTextDocument::ImageResource, QUrl(imageUrl), image);
        
        // 插入图片
        cursor.insertImage(imageUrl);
    }
    else
    {
        // 图片加载失败，显示错误信息
        showUnSupport("图片加载失败: " + QString::fromStdString(content->filename));
    }
}

void RecvTextEdit::showText(const TextContent *content)
{
    insertHtml(textHtmlStr(content));
}

void RecvTextEdit::showKnock(const KnockContent *content, bool mySelf)
{
    if (mySelf)
        insertHtml("[发送了一个窗口抖动]");
    else
        insertHtml("[发来窗口抖动]");
}

void RecvTextEdit::showUnSupport(const QString &text)
{
    QString t = text;
    if (t.isEmpty())
        t = "对方发来尚未支持的内容，无法显示";

    insertHtml("<font color=\"red\">" + t + "</font>");
}

void RecvTextEdit::drawDaySeperatorIfNewDay(long long sinceEpoch)
{
    QDateTime cur;
    cur.setMSecsSinceEpoch(sinceEpoch);

    if (mLastEdit > 0)
    {
        QDateTime last;
        last.setMSecsSinceEpoch(mLastEdit);

        if (last.daysTo(cur) > 0)
        {
            addWarning("-----------------------------");
        }
    }

    mLastEdit = sinceEpoch;
}

QString RecvTextEdit::textHtmlStr(const TextContent *content)
{
    auto str = QString(content->text.c_str());

    return textHtmlStr(str, QColor());
}

QString RecvTextEdit::textHtmlStr(const QString &content, const QColor &color)
{
    auto htmlStr = content.toHtmlEscaped();
    htmlStr.replace("\r\n", "<br>");
    htmlStr.replace("\r", "<br>");
    htmlStr.replace("\n", "<br>");

    for (auto i = 0; i < EMOJI_LEN; i++)
    {
        auto resName = QString(":/default/res/face/") + QString::number(i + 1) + ".gif";
        auto emojiStr = QString(g_emojis[i]).toHtmlEscaped();
        QString imgTag = "<img src=\"" + resName + "\"/>";
        htmlStr.replace(emojiStr, imgTag);
    }
    if(color != QColor::Invalid)
    {
        htmlStr = QString("<font color=%1>%2</font>").arg(color.name(), htmlStr);
    }
    return htmlStr;
}