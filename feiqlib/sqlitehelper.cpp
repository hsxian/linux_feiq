#include "sqlitehelper.h"
#include <sqlite3.h>
#include <iostream>
#include "defer.h"

using namespace std;

SqliteHelper::SqliteHelper(sqlite3 *db)
{
    mDb = db;
}

bool SqliteHelper::query(const string &tableName, const string &selection, const string &where,  const vector<string> &args, const function<void (sqlite3_stmt *)> &callback)
{
    string sql = "select " + selection + " from " + tableName + " where " + where;
    sqlite3_stmt *stmt = nullptr;
    auto ret = sqlite3_prepare_v2(mDb, sql.c_str(), sql.length(), &stmt, nullptr);
    CHECK_SQLITE_RET2(ret, "prepare to " + tableName, false);

    Defer finalizeStmt
    {
        [stmt]()
        {
            sqlite3_finalize(stmt);
        }
    };

    int len = args.size();
    for (auto i = 0; i < len; i++)
    {
        ret = sqlite3_bind_text(stmt, i + 1, args[i].c_str(), args[i].length(), nullptr);
        CHECK_SQLITE_RET2(ret, "bind args", false);
    }

    ret = sqlite3_step(stmt);
    CHECK_SQLITE_STEP_RET2(ret, "step to " + tableName, false);
    callback(stmt);
    return true;
}

bool SqliteHelper::queryList(const string &tableName, const string &selection, const string &where, const vector<string> &args, const function<void (sqlite3_stmt *)> &callback)
{
    string sql = "select " + selection + " from " + tableName + " where " + where;
    sqlite3_stmt *stmt;
    auto ret = sqlite3_prepare_v2(mDb, sql.c_str(), sql.length(), &stmt, nullptr);
    CHECK_SQLITE_RET2(ret, "prepare to " + tableName, false);

    Defer finalizeStmt
    {
        [stmt]()
        {
            sqlite3_finalize(stmt);
        }
    };

    int len = args.size();
    for (auto i = 0; i < len; i++)
    {
        ret = sqlite3_bind_text(stmt, i + 1, args[i].c_str(), args[i].length(), nullptr);
        CHECK_SQLITE_RET2(ret, "bind args", false);
    }

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
            callback(stmt);
        }
    }

    return true;
}

bool SqliteHelper::modifyInt64(const string &tableName, const string &set, const int64_t &v, const string &where, const vector<string> &args)
{
    int ret;
    string sql = "update " + tableName + " set " + set + " = ? where " + where;
    sqlite3_stmt *stmt = nullptr;
    ret = sqlite3_prepare_v2(mDb, sql.c_str(), sql.length(), &stmt, nullptr);
    CHECK_SQLITE_RET2(ret, "prepare to " + tableName, false);

    Defer finalizeStmt
    {
        [stmt]()
        {
            sqlite3_finalize(stmt);
        }
    };

    ret = sqlite3_bind_int64(stmt, 1, v);
    CHECK_SQLITE_RET2(ret, tableName + " bind " + set, false);


    int len = args.size();
    for (auto i = 0; i < len; i++)
    {
        ret = sqlite3_bind_text(stmt, i + 2, args[i].c_str(), args[i].length(), nullptr);
        CHECK_SQLITE_RET2(ret, tableName + " bind args", false);
    }

    ret = sqlite3_step(stmt);
    CHECK_SQLITE_STEP_RET2(ret, "modify " + tableName, false);
    return true;
}

long long SqliteHelper::insert(const string &tableName, const string &valuetions, const function<void (sqlite3_stmt *)> &callback)
{
    string sql = "insert into " + tableName + " values(" + valuetions + ")";

    sqlite3_stmt *stmt = nullptr;
    auto ret = sqlite3_prepare_v2(mDb, sql.c_str(), sql.length(), &stmt, nullptr);
    CHECK_SQLITE_RET2(ret, "prepare to insert " + tableName, -1);

    Defer finalizeStmt
    {
        [stmt]()
        {
            sqlite3_finalize(stmt);
        }
    };
    //绑定参数
    callback(stmt);

    ret = sqlite3_step(stmt);
    CHECK_SQLITE_STEP_RET2(ret, "insert " + tableName, -1);

    return sqlite3_last_insert_rowid(mDb);
}

bool SqliteHelper::createTable(const string &tableName, const string &columns)
{
    string createTable = "create table if not exists " + tableName + "(" + columns + ")";
    auto ret = sqlite3_exec(mDb, createTable.c_str(), nullptr, nullptr, nullptr);
    CHECK_SQLITE_RET2(ret, "create table " + tableName, false);
    return true;
}

