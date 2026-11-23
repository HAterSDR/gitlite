#include "../include/Repository.h"
#include "../include/Utils.h"       
#include "../include/RefStore.h"   
#include "../include/ObjectStore.h" 
#include "../include/Commit.h"
#include "../include/Index.h"
#include "../include/Ignore.h"
#include <dirent.h>
#include <queue>
// main.cpp 的 checkCWD 依赖此方法，请保留。
std::string Repository::getGitliteDir() {
    return ".gitlite";
}
void Repository::Init(){
    //若无文件，新建文件
    Utils::createDirectories(".gitlite/objects");
    Utils::createDirectories(".gitlite/refs/heads");
    //初始化commit对象
    Commit initial;
    initial.message = "initial commit";
    initial.timestamp = 0;
    //将首次提交的commit对象写入objects中
    std::string CommitID = ObjectStore::WriteObject("commit", initial.Serialize());
    //将commitID写入引用中
    RefStore::WriteRef("master", CommitID);
    RefStore::WriteHEAD("master");
    Utils::writeContents(".gitlite/index", "");
}
std::string Repository::GetCurrentBranch(){
    return RefStore::ReadHEAD();
}
Commit Repository::GetCurrentCommit(){
    //拿到当前分支
    std::string branch = GetCurrentBranch();
    //从分支处获得commitid，也就是sha
    std::string CommitID = RefStore::ReadRef(branch);
    //从objects处获得text
    std::string text = ObjectStore::ReadObject(CommitID);
    return Commit::Deserialize(text);
}
void Repository::SaveCommit(const Commit& commit){
    //存入库中
    std::string NewID = ObjectStore::WriteObject("commit", commit.Serialize());
    std::string branch = GetCurrentBranch();
    //把分支指向新ID
    RefStore::WriteRef(branch,NewID);
}

std::string Repository::ResolveRevision(const std::string& rev){
    //解析revision
    //如果存在tag，直接返回tag内容
    std::string path = ".gitlite/refs/tags/" + rev;
    if(Utils::isFile(path)){
        return Utils::readContentsAsString(path);
    }
    //如果是40串完整哈希，返回rev
    if(rev.length() == 40 && Utils::isFile(".gitlite/objects/" + rev.substr(0, 2) + "/" + rev.substr(2))) return rev;
    //如果不是完整哈希，匹配前缀，按字典序返回最小的
    std::vector<std::string> matches;
    //遍历所有两位前缀，看看有哪些匹配文件
    for (int i = 0; i < 256; ++i) {
        // 把整数转成2位的十六进制字符串
        char HexStr[3];
        std::snprintf(HexStr, sizeof(HexStr), "%02x", i);
        std::string DirPrefix = HexStr;
        // 拼出子目录路径
        std::string SubDir = ".gitlite/objects/" + DirPrefix;
        auto files = Utils::plainFilenamesIn(SubDir);
        for (const auto& fileName : files) {
            // 拼成完整的 40 位哈希
            std::string FullHash = DirPrefix + fileName;
            //判断是否以用户输入的前缀开头
            if (FullHash.rfind(rev, 0) == 0) {
                // 去读一下内容，确认它是 commit 而不是 blob
                std::string content = Utils::readContentsAsString(SubDir + "/" + fileName);
                if (content.rfind("commit ", 0) == 0) {
                    matches.push_back(FullHash);
                }
            }
        }
    }
    if (!matches.empty()) {
        std::sort(matches.begin(), matches.end());
        return matches[0];
    }
    return "";
};

void Repository::RestoreFromCommit(const Commit& commit, const std::string& filename){
    //将commit中filename文件写入工作区
    std::string BlobID = commit.snapshot.at(filename);
    std::string content = ObjectStore::ReadObject(BlobID);
    std::string RealContent = content.substr(5);
    Utils::writeContents(filename,RealContent);
};

void Repository::CheckOutSnapshot(const Commit& current, const Commit& target){
    //用target中内容，比对current中内容后整体替换、删除工作区文件
    for(auto const& pair : target.snapshot){
        RestoreFromCommit(target,pair.first);
    }
    for(const auto& pair : current.snapshot){
        const std::string& file = pair.first;
        if(target.snapshot.find(file) == target.snapshot.end()){
            if(Utils::isFile(file)){
                Utils::restrictedDelete(file);
            }
        }
    }
};
bool Repository::HasUntracked(const Commit& current, const Commit& target, const Index& index, const Ignore& ignore){
    //检查是否存在Untracked文件
        for(const auto& pair : target.snapshot){
            const std::string& file = pair.first;
            bool IsTracked = current.isTracked(file);
            bool IsStageAdd = index.IsStageAdd(file);
            bool ignored = ignore.IsIgnored(file);

            if(Utils::isFile(file) && !IsTracked && !IsStageAdd && !ignored) return true;
        }
        return false;
};
Commit Repository::GetCommitbyID(const std::string ID){
    std::string text = ObjectStore::ReadObject(ID);
    return Commit::Deserialize(text);
};
std::string Repository::FindSplit(const std::string& CurrID, const std::string& GivenID){
    std::set<std::string> ancestors;
    std::queue<std::string> q;
    q.push(CurrID);
    //用bfs搜索当前commit的所有祖先
    while(!q.empty()){
        std::string id = q.front();
        q.pop();
        if(ancestors.count(id)) continue;
        ancestors.insert(id);
        Commit commit = GetCommitbyID(id);
        for(const auto&p : commit.parents) q.push(p);
    }
    //用bfs搜索指定commit的祖先，若发现某一祖先出现在ancestors里则返回
    std::set<std::string> visited;
    std::queue<std::string> q2;
    q2.push(GivenID);
    while(!q2.empty()){
        std::string id = q2.front();
        q2.pop();
        if(visited.count(id)) continue;
        visited.insert(id);
        if(ancestors.count(id)) return id;
        Commit commit = GetCommitbyID(id);
        for(const auto& p : commit.parents) q2.push(p);
    }
    return "";
};
std::vector<Repository::MergeItem> Repository::GenerateMergePlan(const Commit& split, const Commit& curr, const Commit& given){
    std::vector<MergeItem> plan;
    std::set<std::string> Files;
    //插入所有待标记文件
    for(const auto& p : split.snapshot){
        Files.insert(p.first);
    }
    for(const auto& p : curr.snapshot){
        Files.insert(p.first);
    }
    for(const auto& p: given.snapshot){
        Files.insert(p.first);
    }
    //判断所有文件状态，生成MergeItem，插入vector中
    for(const std::string& file: Files){
        std::string SplitID = split.snapshot.count(file) ? split.snapshot.at(file) : "";
        std::string CurrID = curr.snapshot.count(file) ? curr.snapshot.at(file) : "";
        std::string GivenID = given.snapshot.count(file) ? given.snapshot.at(file) : "";
        //版本一致，跳过
        if(CurrID == GivenID) continue;
        if(CurrID == SplitID && GivenID != SplitID){
            //指定commit中文件不存在，删除
            if(GivenID == "") plan.push_back({file, MergeAction::Delete,""});
            else{
                //指定commit中文件存在，替换
                plan.push_back({file, MergeAction::Write, GivenID});
            }
        }
        //否则冲突，两边情况不一
        else if(GivenID == SplitID && CurrID != SplitID) continue;
        else{
            plan.push_back({file, MergeAction::Conflict,""});
        }
    }
    return plan;
};
bool Repository::ApplyMergePlan(const std::vector<MergeItem>& plan, Index& index, const Commit& CurrCommit, const Commit& GivenCommit, const Ignore& ignore){
    for (const auto& item : plan) {
        // 只有需要写入、删除或写入冲突内容的路径才需要检查, 也就是不是keep都要检查
        if (item.action != MergeAction::Keep) {
            // 工作区存在该文件 && 未被当前提交跟踪 && 未暂存待添加 && 未被忽略
            if (Utils::isFile(item.filename) && !CurrCommit.isTracked(item.filename) && !index.IsStageAdd(item.filename) && !ignore.IsIgnored(item.filename)) {
                // 发现冲突，直接返回 true，交由上层去 exitWithMessage
                Utils::exitWithMessage("There is an untracked file in the way; delete it, or add and commit it first.");
            }
        }
    }
    bool HasConflict = false;
    for(const auto& item : plan){
         if(item.action == MergeAction::Delete){
            //若为待删除，且在工作区内，则删除
            if(Utils::isFile(item.filename)){
                Utils::restrictedDelete(item.filename);
            }
            index.StagedAdd.erase(item.filename);
            index.StagedRemove.insert(item.filename);
         }
        if(item.action == MergeAction::Write){
            //若为待改写，将blobid写入
            std::string text = ObjectStore::ReadObject(item.BlobID).substr(5);
            Utils::writeContents(item.filename, text);
            index.StagedAdd[item.filename] = item.BlobID;
        }
        //若为冲突，输出冲突消息
        if(item.action == MergeAction::Conflict){
            HasConflict = true;
            std::string CurrContent = CurrCommit.snapshot.count(item.filename) ? CurrCommit.snapshot.at(item.filename)
            : "";
            std::string GivenContent = GivenCommit.snapshot.count(item.filename) ? GivenCommit.snapshot.at(item.filename)
            : "";
            //按规矩生成conflict text
            std::string ConflictText = "<<<<<<< HEAD\n";
            CurrContent = ObjectStore::ReadBlob(CurrContent);
            GivenContent = ObjectStore::ReadBlob(GivenContent);
            // 如果当前内容不以换行符结尾，补齐一个
            if (!CurrContent.empty() && CurrContent.back() != '\n') {
                CurrContent += "\n";
            }
            ConflictText += CurrContent;

            ConflictText += "=======\n";
            
            if (!GivenContent.empty() && GivenContent.back() != '\n') {
                GivenContent += "\n";
            }
            ConflictText += GivenContent;

            ConflictText += ">>>>>>>\n";
            Utils::writeContents(item.filename, ConflictText);
            std::string NewBlob = ObjectStore::WriteObject("blob", ConflictText);
            index.StagedAdd[item.filename] = NewBlob;
        }
    }
    //特别返回是否有冲突
    return HasConflict;
};
