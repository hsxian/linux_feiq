#ifndef SQLITEHELPER_H
#define SQLITEHELPER_H
#include <string>
#include <functional>
#include <vector>
using namespace std;
struct sqlite3;
struct sqlite3_stmt;


#define CHECK_SQLITE_RET(ret, action, result)           \
    if (ret != SQLITE_OK && ret != SQLITE_DONE)                               \
    {                                                   \
        cout << "failed to " #action " :" << ret << endl; \
        return result;                                  \
    }

#define CHECK_SQLITE_RET2(ret, action, result)           \
    if (ret != SQLITE_OK && ret != SQLITE_DONE)                               \
    {                                                   \
        cout << "failed to " << action << " :" << ret << endl; \
        return result;                                  \
    }

#define CHECK_SQLITE(ret, action)                  \
    if (ret != SQLITE_OK && ret != SQLITE_DONE)         \
    {                                                   \
        cout << "failed to " #action " :" << ret << endl; \
    }

#define CHECK_SQLITE2(ret, action)                  \
    if (ret != SQLITE_OK && ret != SQLITE_DONE)         \
    {                                                   \
        cout << "failed to " << action << " :" << ret << endl; \
    }

#define CHECK_SQLITE_STEP_RET2(ret, action, result)           \
    if (ret != SQLITE_OK && ret != SQLITE_DONE && ret != SQLITE_ROW)                               \
    {                                                   \
        cout << "failed to " << action << " :" << ret << endl; \
        return result;                                  \
    }

class SqliteHelper
{
public:
    SqliteHelper(sqlite3 *db);
    bool query(const string &tableName, const string &selection, const string &where,  const vector<string> &args, const function<void(sqlite3_stmt *)> &callback);
    bool queryList(const string &tableName, const string &selection, const string &where, const vector<string> &args, const function<void(sqlite3_stmt *)> &callback);
    bool modifyInt64(const string &tableName, const string &set, const int64_t &v, const string &where,  const vector<string> &args);
    long long insert(const string &tableName, const string &valuetions, const function<void(sqlite3_stmt *)> &callback);
    bool createTable(const string &tableName, const string &columns);
private:
    sqlite3 *mDb;
};

#endif // SQLITEHELPER_H
