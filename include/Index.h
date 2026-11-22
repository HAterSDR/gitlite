#ifndef INDEX_H
#define INDEX_H
#include <string>
#include <map>
#include <set>
class Index{
public:
    std::map<std::string, std::string> StagedAdd;
    std::set<std::string> StagedRemove;
    void load();
    void save() const;
    void clear();
    bool IsStageAdd(const std::string& filename) const;
    bool IsStageRemove(const std::string& filename) const;
};
#endif