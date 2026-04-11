#ifndef UTILS_H
#define UTILS_H

#include <vector>
#include <string>
#include <chrono>
using namespace std;
using namespace std::chrono;
using SysTimeMs = std::chrono::time_point<std::chrono::system_clock, 
                                          std::chrono::milliseconds>;
vector<string> splitAllowSeperator(vector<char>::iterator from, vector<char>::iterator to, char sep);
void stringReplace(string &target, const string &pattern, const string &candidate);
string getFileNameFromPath(const string &path);
bool startsWith(const string &str, const string &patten);
bool endsWith(const string &str, const string &patten);
string toString(const vector<char> &buf);
int64_t timeToLong(const SysTimeMs &time);
SysTimeMs longToTime(int64_t time);
#endif // UTILS_H
