#ifndef REPOSITORY_H
#define REPOSITORY_H

#include "../include/ObjectStore.h"
#include "../include/RefStore.h"
#include "../include/Commit.h"
#include "../include/Index.h"
#include "../include/Ignore.h"
#include <string>
#include <map>
// 建议类：抽象仓库的持久化状态，包括 HEAD、分支、标签、暂存区与对象库；
// 可自由扩充或重新设计。
// 注意：main.cpp 的 checkCWD 依赖 getGitliteDir()，请保留该方法。
class Repository {
public:
    enum class MergeAction{Keep,Write,Delete,Conflict};
    struct MergeItem{
        std::string filename;
        MergeAction action;
        std::string BlobID;
    };
    //专门处理merge的类型

    // 元数据目录名，即 ".gitlite"
    static std::string getGitliteDir();

    static void Init();

    static std::string GetCurrentBranch();

    static Commit GetCurrentCommit();

    static void SaveCommit(const Commit& commit);

    static std::string ResolveRevision(const std::string& rev);

    static void RestoreFromCommit(const Commit& commit, const std::string& filename);

    static void CheckOutSnapshot(const Commit& current, const Commit& target);

    static bool HasUntracked(const Commit& current, const Commit& target, const Index& index, const Ignore& ignore);

    static Commit GetCommitbyID(const std::string ID);

    static std::string FindSplit(const std::string& CurrID, const std::string& GivenID);

    static std::vector<MergeItem> GenerateMergePlan(const Commit& base, const Commit& curr, const Commit& given);

    static bool ApplyMergePlan(const std::vector<MergeItem>& plan, Index& index, const Commit& CurrCommit, const Commit& GivenCommit, const Ignore& ignore);
    
    static std::map<std::string, std::string> ReadConfig();

    static void SaveConfig(const std::map<std::string, std::string>& configs);

    static void CopyObjects(const std::string SrcRepo, const std::string TargetRepo);

    static bool IsAncestor(const std::string& AncestorID, const std::string& DescendantID);
};

#endif // REPOSITORY_H
