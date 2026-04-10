#include "history.h"
#include "defer.h"
#include <errno.h>
#include <iostream>
#include <string.h>
#include <unistd.h>

#define FELLOW_TABLE "fellow"
#define MESSAGE_TABLE "message"

#define CHECK_SQLITE_RET(ret, action, result)           \
    if (ret != SQLITE_OK)                               \
    {                                                   \
        cout << "failed to" #action ":" << ret << endl; \
        return result;                                  \
    }

#define CHECK_SQLITE_RET2(ret, action)                  \
    if (ret != SQLITE_OK)                               \
    {                                                   \
        cout << "failed to" #action ":" << ret << endl; \
    }

History::History()
{
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

    //创建表
    if (needCreateTable)
    {
        string createFellowTable = "create table " FELLOW_TABLE "(id integer,ip text, name text, mac text, primary key(id));";
        string createMessageTable = "create table " MESSAGE_TABLE " (id integer, sender integer, receiver integer, time integer, content blob, primary key(id))";

        ret = sqlite3_exec(mDb, createFellowTable.c_str(), nullptr, nullptr, nullptr);
        CHECK_SQLITE_RET(ret, "create fellow table", false);

        ret = sqlite3_exec(mDb, createMessageTable.c_str(), nullptr, nullptr, nullptr);
        CHECK_SQLITE_RET(ret, "create message table", false);
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

void History::add(const HistoryRecord &record)
{
    int ret;

    //先更新好友信息
    auto senderFellowId = ensureFindFellowId(*record.sender);
    auto receiverFellowId = ensureFindFellowId(*record.receiver);

    //再增加记录
    Parcel parcel;
    record.what->writeTo(parcel);
    string sql = "insert into " MESSAGE_TABLE " values(null," + to_string(senderFellowId) + "," + to_string(receiverFellowId) + "," + to_string(record.time.time_since_epoch().count()) + ",?);";

    sqlite3_stmt *stmt = nullptr;
    ret = sqlite3_prepare_v2(mDb, sql.c_str(), sql.length(), &stmt, nullptr);
    CHECK_SQLITE_RET2(ret, "prepare to insert message");

    Defer finalizeStmt
    {
        [stmt]()
        {
            sqlite3_finalize(stmt);
        }
    };

    auto buf = parcel.raw();
    ret = sqlite3_bind_blob(stmt, 1, buf.data(), buf.size(), nullptr);
    CHECK_SQLITE_RET2(ret, "bind content to blob");

    ret = sqlite3_step(stmt);
    CHECK_SQLITE_RET2(ret, "insert message");
}
vector<Fellow> History::queryFellows(const string &selection)
{
    vector<Fellow> result;
    string sql = "select * from " FELLOW_TABLE " where " + selection;
    
    sqlite3_stmt *stmt = nullptr;
    auto ret = sqlite3_prepare_v2(mDb, sql.c_str(), sql.length(), &stmt, nullptr);
    if (ret != SQLITE_OK)
    {
        cout << "failed to prepare queryFellows: " << ret << endl;
        return result;
    }
    
    Defer finalizeStmt
    {
        [stmt]()
        {
            sqlite3_finalize(stmt);
        }};
    
    while (true)
    {
        ret = sqlite3_step(stmt);
        if (ret == SQLITE_DONE)
        {
            break;
        }
        else if (ret != SQLITE_ROW)
        {
            cerr << "error occur while step queryFellows:" << ret << endl;
            break;
        }
        else
        {
            Fellow fellow;
            const char* ipStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            const char* nameStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            const char* macStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
            
            if (ipStr)
                fellow.setIp(ipStr);
            if (nameStr)
                fellow.setName(nameStr);
            if (macStr)
                fellow.setMac(macStr);
            
            result.push_back(fellow);
        }
    }
    
    return result;
}



vector<HistoryRecord> History::query(const string &selection, const vector<string> &args)
{
    vector<HistoryRecord> result;

    sqlite3_stmt *stmt;
    string sql = "select * from " MESSAGE_TABLE " where " + selection;
    auto ret = sqlite3_prepare_v2(mDb, sql.c_str(), sql.length(), &stmt, nullptr);
    CHECK_SQLITE_RET(ret, "prepare to query", result);

    Defer finalizeStmt
    {
        [stmt]()
        {
            sqlite3_finalize(stmt);
        }};

    int len = args.size();
    for (auto i = 0; i < len; i++)
    {
        ret = sqlite3_bind_blob(stmt, i + 1, args[i].c_str(), args[i].length(), nullptr);
        CHECK_SQLITE_RET(ret, "bind args", result);
    }

    while (true)
    {
        ret = sqlite3_step(stmt);
        if (ret == SQLITE_DONE)
        {
            return result;
        }
        else if (ret != SQLITE_ROW)
        {
            cerr << "error occur while step query:" << ret << endl;
            return result;
        }
        else
        {
            auto senderFellowId = sqlite3_column_int(stmt, 1);
            auto receiverFellowId = sqlite3_column_int(stmt, 2);
            auto time = sqlite3_column_int(stmt, 3);
            auto contentData = sqlite3_column_blob(stmt, 4);
            auto contentLen = sqlite3_column_bytes(stmt, 4);

            HistoryRecord record;
            auto senderFellow = getFellow(senderFellowId);
            auto receiverFellow = getFellow(receiverFellowId);

            record.sender = shared_ptr<Fellow>(std::move(senderFellow));
            record.receiver = shared_ptr<Fellow>(std::move(receiverFellow));

            record.time = time_point<system_clock, milliseconds>(milliseconds(time));

            Parcel parcel;
            parcel.fillWith(contentData, contentLen);
            parcel.resetForRead();
            record.what = ContentParcelFactory::createFromParcel(parcel);

            result.push_back(record);
        }
    }

    return result;
}

unique_ptr<Fellow> History::getFellow(int id)
{
    unique_ptr<Fellow> fellow = make_unique<Fellow>();
    string sql = "select ip, name, mac from " FELLOW_TABLE " where id = " + to_string(id);

    sqlite3_stmt *stmt = nullptr;
    auto ret = sqlite3_prepare_v2(mDb, sql.c_str(), sql.length(), &stmt, nullptr);
    if (ret != SQLITE_OK)
    {
        cout << "failed to prepare getFellow: " << ret << endl;
        return nullptr;
    }

    Defer finalizeStmt
    {
        [stmt]()
        {
            sqlite3_finalize(stmt);
        }};

    ret = sqlite3_step(stmt);
    if (ret != SQLITE_ROW)
    {
        cout << "no fellow found for id: " << id << endl;
        return nullptr;
    }

    const char *ipStr = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
    const char *nameStr = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
    const char *macStr = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));

    if (ipStr)
        fellow->setIp(ipStr);
    if (nameStr)
        fellow->setName(nameStr);
    if (macStr)
        fellow->setMac(macStr);

    return fellow;
}
int History::ensureFindFellowId(const Fellow &fellow)
{
    auto fellowId = findFellowId(fellow.getIp());
    if (fellowId < 0)
    {
        string sql = "insert into " FELLOW_TABLE " values(null,'" + fellow.getIp() + "','" + fellow.getName() + "','" + fellow.getMac() + "');";
        auto ret = sqlite3_exec(mDb, sql.c_str(), nullptr, nullptr, nullptr);
        CHECK_SQLITE_RET2(ret, "insert fellow to db");

        fellowId = findFellowId(fellow.getIp());
    }

    return fellowId;
}
int History::findFellowId(const string &ip)
{
    int fellowId = -1;
    string sql = "select id from " FELLOW_TABLE " where ip = '" + ip + "'";

    sqlite3_stmt *stmt = nullptr;
    auto ret = sqlite3_prepare_v2(mDb, sql.c_str(), sql.length(), &stmt, nullptr);
    if (ret != SQLITE_OK)
    {
        cout << "failed to prepare findFellowId: " << ret << endl;
        return -1;
    }

    Defer finalizeStmt
    {
        [stmt]()
        {
            sqlite3_finalize(stmt);
        }};

    ret = sqlite3_step(stmt);
    if (ret == SQLITE_ROW)
    {
        fellowId = sqlite3_column_int(stmt, 0);
    }

    return fellowId;
}