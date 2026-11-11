#ifndef COMMIT_H
#define COMMIT_H

#include <string>
#include <vector>
#include <map>
// 建议类：抽象一次提交（消息、时间戳、父提交、文件快照），可自由设计或删除。
// tag 是指向提交的仓库引用，不属于 Commit 对象自身。
// 用于管理commit
class Commit{
    public:
        std::vector<std::string> parents;
        std::string message;
        long long timestamp;
        std::map<std::string, std::string> snapshot;
        Commit();
        std::string  Serialize() const;
        static Commit Deserialize(const std::string& next);
        std::string id() const;
};

#endif // COMMIT_H
