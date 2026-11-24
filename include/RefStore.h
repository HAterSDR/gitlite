#ifndef REFSTORE_H
#define REFSTORE_H
#include <string>
#include <vector>
#include <utility>
class RefStore{
public:
    static std::string ReadRef(const std::string& branch);
    static void WriteRef(const std::string& branch,const std::string& CommitId);
    static std::string ReadHEAD();
    static void WriteHEAD(const std::string& branch);
    static std::string ReadTag(const std::string& name);
    static std::string WriteTag(const std::string& name, const std::string& CommitID);
    static std::string GetRefPath(const std::string& name);
    static void CollectRefs(const std::string& dir, const std::string& prefix, std::vector<std::pair<std::string, std::string>>& refs);
};
#endif