#include "history.h"
#include "defer.h"
#include "sqlitehelper.h"
#include <errno.h>
#include <iostream>
#include <string.h>
#include <unistd.h>
#include <sqlite3.h>

#define FELLOW_TABLE "fellow"
#define MESSAGE_TABLE "message"

History::History()
{

}

History::~History()
{
    if(mHelper)
    {
        delete mHelper;
    }
}

bool History::init(const string &dbPath)
{
    unInit();

    //检查数据库文件是否存在，若不存在，需要创建表
    bool needCreateTable = false;
    auto ret = access(dbPath.c_str(), F_OK);
    if (ret == -1)
    {
        cout << "db may be new created:" << strerror(errno) << endl;
        needCreateTable = true;
    }

    //打开数据库
    ret = sqlite3_open(dbPath.c_str(), &mDb);
    Defer closeDbIfErr
    {
        [this, ret]()
        {
            if (ret && mDb != nullptr)
            {
                cerr << "init failed, close db now" << endl;
                sqlite3_close(mDb); //除非内存不够，否则open总是会分配mDb的内存，总是需要close
                mDb = nullptr;
            }
        }};

    CHECK_SQLITE_RET(ret, "open sqlite", false);

    mHelper = new SqliteHelper(mDb);

    //创建表
    // if (needCreateTable)
    {
        //当表不存在时，需要创建表
        mHelper->createTable(FELLOW_TABLE, "id integer,ip text, name text, mac text, primary key(id)");
        mHelper->createTable(MESSAGE_TABLE, "id integer, sender integer, receiver integer, time integer, eventType integer, content blob, primary key(id)");
    }

    return !ret;
}

void History::unInit()
{
    if (mDb != nullptr)
    {
        sqlite3_close(mDb);
        mDb = nullptr;
    }
}

long long History::add(const HistoryRecord &record)
{
    long long ret;

    //先更新好友信息
    auto senderFellowId = ensureFindFellowId(*record.sender);
    auto receiverFellowId = ensureFindFellowId(*record.receiver);
    Parcel parcel;
    record.what->writeTo(parcel);
    auto buf = parcel.raw();
    //再增加记录
    ret = mHelper->insert(MESSAGE_TABLE, "null,?,?,?,?,?", [ &](sqlite3_stmt * stmt)
    {
        sqlite3_bind_int64(stmt, 1, senderFellowId);
        sqlite3_bind_int64(stmt, 2, receiverFellowId);
        sqlite3_bind_int64(stmt, 3, timeToLong(record.time));
        sqlite3_bind_int64(stmt, 4, (int64_t)record.eventType);

        auto bindRet = sqlite3_bind_blob(stmt, 5, buf.data(), buf.size(), nullptr);
        CHECK_SQLITE(bindRet, "bind content to blob");
    });

    return ret;
}

long long History::add(const shared_ptr<Fellow> &sender, const shared_ptr<Fellow> &receiver, const shared_ptr<Content> &what)
{
    HistoryRecord record;
    record.sender = sender;
    record.receiver = receiver;
    record.what = what;
    record.eventType = ViewEventType::MESSAGE;
    record.time = what->getSendTime();
    return add(record);
}

void History::modify(const string &selection, const vector<string> &args, const ViewEventType &type)
{
    mHelper->modifyInt64(MESSAGE_TABLE, "eventType", (int64_t)type, selection, args);
}

void parseFellow(sqlite3_stmt *stmt, Fellow &fellow)
{
    fellow.setId(sqlite3_column_int(stmt, 0));
    const char *ipStr = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
    const char *nameStr = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
    const char *macStr = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3));

    if (ipStr)
        fellow.setIp(ipStr);
    if (nameStr)
        fellow.setName(nameStr);
    if (macStr)
        fellow.setMac(macStr);
}

vector<Fellow> History::queryFellows(const string &selection, const vector<string> &args)
{
    vector<Fellow> result;
    mHelper->queryList(FELLOW_TABLE, "id,ip,name,mac", selection, args, [&](sqlite3_stmt * stmt)
    {
        Fellow fellow;
        parseFellow(stmt, fellow);
        result.push_back(fellow);
    });

    return result;
}



vector<HistoryRecord> History::query(const string &selection, const vector<string> &args)
{
    vector<HistoryRecord> result;
    mHelper->queryList(MESSAGE_TABLE, "id,sender,receiver,time,eventType,content", selection + " order by time", args, [&](sqlite3_stmt * stmt)
    {
        HistoryRecord record;
        record.id = sqlite3_column_int(stmt, 0);
        auto senderFellowId = sqlite3_column_int(stmt, 1);
        auto receiverFellowId = sqlite3_column_int(stmt, 2);
        auto time = sqlite3_column_int64(stmt, 3);
        record.eventType = (ViewEventType)sqlite3_column_int(stmt, 4);
        auto contentData = sqlite3_column_blob(stmt, 5);
        auto contentLen = sqlite3_column_bytes(stmt, 5);

        auto senderFellow = getFellow(senderFellowId);
        auto receiverFellow = getFellow(receiverFellowId);
        record.sender = shared_ptr<Fellow>(std::move(senderFellow));
        record.receiver = shared_ptr<Fellow>(std::move(receiverFellow));

        record.time = longToTime(time);

        Parcel parcel;
        parcel.fillWith(contentData, contentLen);
        parcel.resetForRead();
        record.what = ContentParcelFactory::createFromParcel(parcel);

        result.push_back(record);
    });
    return result;
}

unique_ptr<Fellow> History::getFellow(int id)
{
    unique_ptr<Fellow> fellow = make_unique<Fellow>();
    fellow->setId(id);
    auto stmt = mHelper->query(FELLOW_TABLE, "id,ip,name,mac", "id = ?", {to_string(id)}, [&](sqlite3_stmt * stmt)
    {
        parseFellow(stmt, *fellow);
    });
    return fellow;
}
int History::ensureFindFellowId(const Fellow &fellow)
{
    auto fellowId = findFellowId(fellow.getIp());
    if (fellowId < 0)
    {
        fellowId = mHelper->insert(FELLOW_TABLE, "null,?,?,?", [ fellow ](sqlite3_stmt * stmt)
        {
            sqlite3_bind_text(stmt, 1, fellow.getIp().c_str(), -1, nullptr);
            sqlite3_bind_text(stmt, 2, fellow.getName().c_str(), -1, nullptr);
            sqlite3_bind_text(stmt, 3, fellow.getMac().c_str(), -1, nullptr);
        });
    }

    return fellowId;
}
int History::findFellowId(const string &ip)
{
    int fellowId = -1;
    auto stmt = mHelper->query(FELLOW_TABLE, "id", "ip = ?", {ip}, [&](sqlite3_stmt * stmt)
    {
        fellowId = sqlite3_column_int(stmt, 0);
    });
    return fellowId;
}
