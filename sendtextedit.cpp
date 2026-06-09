#include "sendtextedit.h"
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QFileInfo>
#include <QDebug>
#include <QFileIconProvider>

SendTextEdit::SendTextEdit(QWidget *parent)
    : QTextEdit(parent)
{
    setAcceptDrops(true);
    installEventFilter(this);
}

void SendTextEdit::newLine()
{
    append("");
}

void SendTextEdit::dragEnterEvent(QDragEnterEvent *e)
{
    if (e->mimeData()->hasUrls())
    {
        auto urls = e->mimeData()->urls();
        for (auto url : urls)
        {
            if (QFileInfo(url.toLocalFile()).isFile())
            {
                e->accept();
                return;
            }
        }
    }
    else
    {
        QTextEdit::dragEnterEvent(e);
    }
}
void SendTextEdit::dropEvent(QDropEvent *e)
{
    if (e->mimeData()->hasUrls())
    {
        auto urls = e->mimeData()->urls();

        for (auto url : urls)
        {
            QString localFile = url.toLocalFile();
            QFileInfo fileInfo(localFile);

            // 检查是否是图片文件
            QString suffix = fileInfo.suffix().toLower();
            if (suffix == "jpg" || suffix == "jpeg" || suffix == "png" || suffix == "gif" || suffix == "bmp")
            {
                // 处理图片文件拖拽
                QImage image(localFile);
                if (!image.isNull())
                {
                    QTextCursor cursor = textCursor();
                    QTextDocument *doc = document();
                    doc->addResource(QTextDocument::ImageResource, url, image);
                    cursor.insertImage(url.toString());
                    mImages.append(fileInfo);
                }
            }
            else
            {
                //显示文件图标和文件名
                QFileIconProvider iconProvider;
                QIcon icon = iconProvider.icon(fileInfo);
                QPixmap pixmap = icon.pixmap(32, 32);

                QTextCursor cursor = textCursor();
                QTextDocument *doc = document();
                doc->addResource(QTextDocument::ImageResource, url, pixmap);

                // 插入文件图标和文件名
                cursor.insertImage(url.toString());
                cursor.insertText(" " + fileInfo.fileName());
                cursor.insertText("\n");

                // 处理其他文件
                mFiles.append(fileInfo);
            }
        }

        if (!mImages.isEmpty() || !mFiles.isEmpty())
        {
            e->accept();
        }
    }
    else
    {
        QTextEdit::dropEvent(e);
    }
}

bool SendTextEdit::eventFilter(QObject *, QEvent *e)
{
    if (e->type() == QEvent::KeyPress)
    {
        auto keyEvent = static_cast<QKeyEvent *>(e);
        auto enter = keyEvent->key() == Qt::Key_Return;
        auto ctrl  = keyEvent->modifiers() == Qt::ControlModifier;
        if (enter && ctrl)
        {
            if(!tryEmitMimeFileSignal())
                emit ctrlEnterPressed();
            return true;
        }
        else if (enter)
        {
            if(!tryEmitMimeFileSignal())
                emit enterPressed();
            return true;
        }
    }
    return false;
}

bool SendTextEdit::tryEmitMimeFileSignal()
{
    bool hasFiles = !mImages.isEmpty() || !mFiles.isEmpty();

    if (!mImages.isEmpty())
    {
        emit acceptDropImages(mImages);
        mImages.clear();
        clear();

    }
    if (!mFiles.isEmpty())
    {
        emit acceptDropFiles(mFiles);
        mFiles.clear();
        clear();
    }

    return hasFiles;
}
