#ifndef HISTORY_H
#define HISTORY_H

#include "content.h"
#include "fellow.h"
#include "post.h"
#include <memory>
#include <unordered_map>
#include <vector>
#include "ifeiqview.h"

struct sqlite3;
class SqliteHelper;
using namespace std;

struct HistoryRecord
{
    ulong id;
    ViewEventType eventType;
    time_point<system_clock, milliseconds> time;
    shared_ptr<Fellow> sender;
    shared_ptr<Fellow> receiver;
    shared_ptr<Content> what;
};

//日志记录
//完成代码
//调试代码
//加入model，合并model功能
//加入engine自动记录
//按好友、日期查询最近记录
//更新文件path
/**
 * @brief The History class 以Content为单位，记录和查询聊天记录
 * 还只是个半成品~
 */
class History
{
public:
    History();
    ~History();

public:
    bool init(const string &dbPath);
    void unInit();

public:
    long long add(const HistoryRecord &record);
    long long add(const shared_ptr<Fellow> &sender, const shared_ptr<Fellow> &receiver, const shared_ptr<Content> &what);
    void modify(const string &selection, const vector<string> &args, const ViewEventType &type);
    vector<HistoryRecord> query(const string &selection, const vector<string> &args);
    vector<Fellow> queryFellows(const string &selection, const vector<string> &args);
    int findFellowId(const string &ip);
    //确保找到好友id
    int ensureFindFellowId(const Fellow &fellow);

private:
    unique_ptr<Fellow> getFellow(int id);


private:
    sqlite3 *mDb = nullptr;
    SqliteHelper *mHelper = nullptr;
};

#endif // HISTORY_H
