#include "../include/SomeObj.h"
#include "../include/Utils.h"
#include "../include/GitliteException.h"
#include "../include/Index.h"
#include "../include/Ignore.h"
#include "../include/Repository.h" 
#include <iostream>
#include <ctime>
#include <vector>
#include <queue>
#include <cctype>
SomeObj::SomeObj() {}

// ---------------- Subtask 1：init / add / commit / rm ----------------

void SomeObj::init() {
    // TODO: 创建 .gitlite 目录、initial commit 与 master 分支
    if(Utils::isDirectory(Repository::getGitliteDir())){
        Utils::exitWithMessage("A Gitlite version-control system already exists in the current directory.");
    }
    Repository::Init();
}

void SomeObj::add(const std::string& filename) {
    // TODO: 将文件的当前内容加入暂存区
    //如果文件不存在，报错
    if(!Utils::isFile(filename)){
        Utils::exitWithMessage("File does not exist.");
    }
    //处理文件在.gitignore中的情况
    Commit current = Repository::GetCurrentCommit();
    Index index;
    Ignore ignore;
    ignore.load();
    index.load();
    if(ignore.IsIgnored(filename)||!current.isTracked(filename)||!index.IsStageAdd(filename)){
        return;
    }
    //对比commit中记录的文件内容是否与待提交文件一致
    std::string content = Utils::readContentsAsString(filename);
    std::string NewID = ObjectStore::BlobID(content);
    std::string OldID = "";
    if(current.snapshot.count(filename) > 0){
        OldID = current.snapshot[filename];
    }
    if(OldID != "" && NewID == OldID){
        index.StagedAdd.erase(filename);
        index.StagedRemove.erase(filename);
    }
    else{
        index.StagedAdd[filename] = NewID;
        index.StagedRemove.erase(filename);
    }
    index.save();

}

void SomeObj::commit(const std::string& message) {
    // TODO: 将暂存区保存为新提交，并推进当前分支
    //消息为空则返回
    if(message.empty()){
        Utils::exitWithMessage("Please enter a commit message.");
    }

    //初始化暂存区，暂存区为空则返回
    Index index;
    index.load();
    if(index.StagedAdd.empty() && index.StagedRemove.empty()){
        Utils::exitWithMessage("No changes added to the commit.");
    }
    //初始化父提交
    Commit ParentCommit = Repository::GetCurrentCommit();
    std::string Branch = Repository::GetCurrentBranch();
    std::string ParentID = RefStore::ReadRef(Branch);
    //初始化当前提交，按缓存区内容修改，保存提交
    Commit NewCommit;
    NewCommit.parents.push_back(ParentID);
    NewCommit.message = message;
    NewCommit.timestamp = static_cast<long long>(std::time(nullptr));
    NewCommit.snapshot = ParentCommit.snapshot;
    for(auto const& file : index.StagedAdd){
        NewCommit.snapshot[file.first] = file.second;
    }
    for(auto const& file : index.StagedRemove){
        NewCommit.snapshot.erase(file);
    }
    Repository::SaveCommit(NewCommit);
    //重置暂存区
    index.clear();
    index.save();

}

void SomeObj::rm(const std::string& filename) {
    // TODO: 将文件取消暂存，或标记为待删除
    Commit commit = Repository::GetCurrentCommit();
    Index index;
    index.load();
    //不在暂存区且未被跟踪，退出
    if(!commit.isTracked(filename)&&!index.IsStageAdd(filename)){
        Utils::exitWithMessage("No reason to remove the file.");
    }
    //未被跟踪，暂存区中删除
    if(!commit.isTracked(filename)){
        index.StagedAdd.erase(filename);
        index.save();
        return;
    }
    index.StagedRemove.insert(filename);
    if(Utils::isFile(filename)){
        Utils::restrictedDelete(filename);
    }
    index.save();
}

// ---------------- Subtask 2：log / global-log / tag ----------------

void SomeObj::log() {
    // TODO: 从当前提交沿第一个父提交打印日志
    //找到当前提交
    std::string branch = RefStore::ReadHEAD();
    std::string CurrentID = RefStore::ReadRef(branch);
    //循环打印父提交
    while(!CurrentID.empty()){
        std::string text = ObjectStore::ReadObject(CurrentID);
        Commit current = Commit::Deserialize(text);
        current.Print();
        if(current.parents.empty()){
            break;
        }
        CurrentID = current.parents[0];
    }
}

void SomeObj::globalLog() {
    // TODO: 从所有本地分支、远程跟踪分支和标签出发，输出全部可达提交并按规范排序
    //用bfs查找所有可达提交  
    std::queue<std::string> q;
    std::map<std::string, Commit> visited;
    std::vector<Commit> AllCommit;
    //读取本地分支与tags内容
    auto branches = Utils::plainFilenamesIn(".gitlite/refs/heads");
    for (const auto& branch : branches) {
        q.push(Utils::readContentsAsString(".gitlite/refs/heads/" + branch));
    }
    auto tags = Utils::plainFilenamesIn(".gitlite/refs/tags");
    for (const auto& tag : tags) {
        q.push(Utils::readContentsAsString(".gitlite/refs/tags/" + tag));
    }
    while(!q.empty()){
        std::string id = q.front();
        q.pop();
        if(visited.count(id)) continue;
        Commit commit = Commit::Deserialize(ObjectStore::ReadObject(id));
        visited[id] = commit;
        AllCommit.push_back(commit);
        for(const auto& parent : commit.parents){
            q.push(parent);
        }
    }
    std::sort(AllCommit.begin(), AllCommit.end(), [](const Commit& a, const Commit& b){
        if(a.timestamp != b.timestamp){
            return a.timestamp > b.timestamp;
        }
        return a.id() < b.id();
    });
    for(auto const& commit : AllCommit){
        commit.Print();
    }
    
}

void SomeObj::listTags() {
    // TODO: 按字典序打印所有标签及其目标提交 id
    std::string TagsDir = ".gitlite/refs/tags";
    std::vector<std::string> tags = Utils::plainFilenamesIn(TagsDir);
    std::sort(tags.begin(), tags.end());
    for(const std::string& name : tags){
        std::string FilePath = TagsDir + "/" + name;
        std::string CommitID = Utils::readContentsAsString(FilePath);
        std::cout << name << " " << CommitID << "\n";
    }
}

bool SomeObj::IsValidRefName(std::string name){
    //辅助函数，判断tag名是否合法
    if(name.empty()) return false;
    char first = name[0];
    if(!((first >= 'A' && first <= 'Z') || (first >= 'a' && first <= 'z') || (first >= '0' && first <= '9'))) return false;
    for(char c : name){
        if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) && c != '.' && c != '_' && c != '-') return false;
    }
    return true;
}
void SomeObj::tag(const std::string& name) {
    // TODO: 为当前提交创建轻量标签
    if(!IsValidRefName(name)){
        Utils::exitWithMessage("Incorrect operands.");
    }
    std::string path = ".gitlite/refs/tags/" + name;
    if (Utils::isFile(path)) {
        Utils::exitWithMessage("A tag with that name already exists.");
    }
    Commit commit = Repository::GetCurrentCommit();
    RefStore::WriteTag(name, commit.id());
}

void SomeObj::tag(const std::string& name, const std::string& rev) {
    // TODO: 为指定提交或标签所指提交创建轻量标签
    if(!IsValidRefName(name)){
        Utils::exitWithMessage("Incorrect operands.");
    }
    std::string path = ".gitlite/refs/tags/" + name;
    if (Utils::isFile(path)) {
        Utils::exitWithMessage("A tag with that name already exists.");
    }
    std::string id = Repository::ResolveRevision(rev);
    if (id.empty()) {
        Utils::exitWithMessage("No commit with that id exists.");
    }
    RefStore::WriteTag(name, id);
}

// ---------------- Subtask 3：status / branch / rm-branch ----------------
bool SomeObj::PrintSection(const std::string& title, std::vector<std::string> items){
    //辅助打印函数
    std::cout << "=== " << title << " ===\n";
    std::sort(items.begin(), items.end());
    for (const auto& item : items){
        std::cout << item << "\n";
    }
};
void SomeObj::status() {
    // TODO: 显示分支、暂存区、未暂存修改与未跟踪文件
    //获取环境数据
    Commit commit = Repository::GetCurrentCommit();
    std::string NowBranch = RefStore::ReadHEAD();
    Index index;
    index.load();
    Ignore ignore;
    ignore.load();
    //处理branch
    std::vector<std::string> branches = Utils::plainFilenamesIn(".gitlite/refs/heads");
    std::sort(branches.begin(), branches.end());
    for(auto& branch : branches){
        if(branch == NowBranch) branch = "*" + branch; 
    }
    //处理staged files
    std::vector<std::string> staged;
    for (const auto& pair : index.StagedAdd) {
        staged.push_back(pair.first);
    }
    //处理removed files
    std::vector<std::string> removed;
    for(auto const& file : index.StagedRemove){
        removed.push_back(file);
    }
    //处理modifications files
    std::map<std::string,std::string> modifications;
    //初始化所有待检查文件
    std::set<std::string> FilesToCheck;
    for(const auto& pair : commit.snapshot) FilesToCheck.insert(pair.first);
    for(const auto& pair : index.StagedAdd) FilesToCheck.insert(pair.first);
    //开始检查
    for(const std::string& file : FilesToCheck){
        bool Exists = Utils::isFile(file);
        bool IsTracked = commit.isTracked(file);
        bool IsStageAdd = index.IsStageAdd(file);
        bool IsStageRemove = index.IsStageRemove(file);
        if(!Exists){
            //判断deleted情况
            if(IsStageAdd || (IsTracked && !IsStageRemove))
            modifications[file] = "(deleted)";
        } 
        else{
            //判断modified情况
            std::string blob = ObjectStore::BlobID(Utils::readContentsAsString(file));
            //已暂存但是又修改了
            if(IsStageAdd){
                if(index.StagedAdd[file] != blob){
                    modifications[file] = "(modified)";
                }
            }
            else {
                //已跟踪未暂存删除但是又改了
                if(IsTracked && !IsStageRemove){
                    if(commit.snapshot[file] != blob){
                        modifications[file] = "(modified)";
                    }
                }
            }
        }
    }
    //处理Untracked Files
    std::vector<std::string> UntrackedFiles;
    std::vector<std::string> WorkFiles = Utils::plainFilenamesIn(".");
    for (const std::string& file : WorkFiles) {
        bool isTracked = commit.isTracked(file);
        bool isStagedAdd = index.IsStageAdd(file);
        bool isStagedRemove = index.IsStageRemove(file);

        if (!isStagedAdd) {
            if (!isTracked) {
            //未跟踪文件，检查 ignore 规则
                if (!ignore.IsIgnored(file)) {
                    UntrackedFiles.push_back(file);
                }
            } else if (isStagedRemove) {
                // 特例（被跟踪，暂存删除了，但在工作区又重建了）
                // 无论是否在 ignore 里，都强制显示
                UntrackedFiles.push_back(file);
            }
        }
    }
    //全部打印
    std::cout << "=== Branches ===\n";
    for (const auto& b : branches) std::cout << b << "\n";
    std::cout << "\n";

    PrintSection("Staged Files", staged); 
    std::cout << "\n";

    PrintSection("Removed Files", removed);
    std::cout << "\n";

    std::cout << "=== Modifications Not Staged For Commit ===\n";
    for (const auto& pair : modifications) {
        std::cout << pair.first << " " << pair.second << "\n";
    }
    std::cout << "\n";

    PrintSection("Untracked Files", UntrackedFiles);
}

void SomeObj::branch(const std::string& name) {
    // TODO: 新建指向当前提交的分支，不切换
    //判断是否合法
    if(!IsValidRefName(name)){
        Utils::exitWithMessage("Incorrect operands.");
    }
    //判断是否已存在
    std::string path = ".gitlite/refs/heads/" + name;
    if (Utils::isFile(path)) {
        Utils::exitWithMessage("A branch with that name already exists.");
    }
    std::string branch = RefStore::ReadHEAD();
    std::string CommitID = RefStore::ReadRef(branch);
    RefStore::WriteRef(name, CommitID);
}

void SomeObj::rmBranch(const std::string& name) {
    // TODO: 删除分支指针，不删除其提交
    std::string path = ".gitlite/refs/heads/" + name;
    if(!Utils::isFile(path)){
        Utils::exitWithMessage("A branch with that name does not exist.");
    }
    if(name == Repository::GetCurrentBranch()){
        Utils::exitWithMessage("Cannot remove the current branch.");
    }
    std::remove(path.c_str());
}

// ---------------- Subtask 4：checkout / reset ----------------

void SomeObj::checkoutBranch(const std::string& name) {
    // TODO: 切换到指定分支
    //前置条件判断
    std::string path = ".gitlite/refs/heads/" + name;
    if(!Utils::isFile(path)){
        Utils::exitWithMessage("No such branch exists.");
    }
    if(name == Repository::GetCurrentBranch()){
        Utils::exitWithMessage("No need to checkout the current branch.");
    }
    //初始化
    Commit current = Repository::GetCurrentCommit();
    Commit target = Repository::GetCommitbyID(RefStore::ReadRef(name));
    Index index;
    index.load();
    Ignore ignore;
    ignore.load();
    //判断是否Untracked
    if (Repository::HasUntracked(current, target, index, ignore)) {
        Utils::exitWithMessage("There is an untracked file in the way; delete it, or add and commit it first.");
    }
    RefStore::WriteHEAD(name);
    Repository::CheckOutSnapshot(current, target);
    index.clear();
    index.save();
}

void SomeObj::checkoutFile(const std::string& filename) {
    // TODO: 用当前提交中的版本覆盖工作目录中的文件
    Commit commit = Repository::GetCurrentCommit();
    checkoutFileInCommit(commit.id(), filename);
}

void SomeObj::checkoutFileInCommit(const std::string& rev, const std::string& filename) {
    // TODO: 用指定 revision 的版本覆盖工作文件，支持标签与 id 缩写
    std::string CommitID = Repository::ResolveRevision(rev);
    if(CommitID.empty()) Utils::exitWithMessage("No commit with that id exists.");

    Commit target = Repository::GetCommitbyID(CommitID);
    if(!target.isTracked(filename)){
        Utils::exitWithMessage("File does not exist in that commit.");
    }
    Repository::RestoreFromCommit(target, filename);
}

void SomeObj::reset(const std::string& rev) {
    // TODO: 解析 revision，检出指定提交并移动当前分支头
    std::string CommitID = Repository::ResolveRevision(rev);
    if (CommitID.empty()) Utils::exitWithMessage("No commit with that id exists.");
    Commit current = Repository::GetCurrentCommit();
    Commit target = Repository::GetCommitbyID(CommitID);
    Index index; 
    index.load();
    Ignore ignore; 
    ignore.load();
    if(Repository::HasUntracked(current, target, index, ignore)){
        Utils::exitWithMessage("There is an untracked file in the way; delete it, or add and commit it first.");
    }
    RefStore::WriteRef(RefStore::ReadHEAD(), CommitID);
    Repository::CheckOutSnapshot(current, target);
    index.clear();
    index.save();
}

// ---------------- Subtask 5：merge ----------------

void SomeObj::merge(const std::string& branch) {
    // TODO: 将指定分支合并到当前分支
    Index index;
    index.load();
    Ignore ignore;
    ignore.load();
    if(!index.StagedAdd.empty()||!index.StagedRemove.empty()){
        Utils::exitWithMessage("You have uncommitted changes.");
    }
    if(!Utils::isFile(".gitlite/refs/heads/" + branch)){
        Utils::exitWithMessage("A branch with that name does not exist.");
    }
    std::string CurrBranch = RefStore::ReadHEAD();
    if(CurrBranch == branch){
        Utils::exitWithMessage("Cannot merge a branch with itself.");
    }
    std::string CurrID = RefStore::ReadRef(CurrBranch);
    std::string GivenID = RefStore::ReadRef(branch);
    Commit curr = Repository::GetCommitbyID(CurrID);
    Commit given = Repository::GetCommitbyID(GivenID);
    std::string SplitID = Repository::FindSplit(CurrID, GivenID);
    Commit Split = Repository::GetCommitbyID(SplitID);

    
    if(SplitID == GivenID){
        Utils::exitWithMessage("Given branch is an ancestor of the current branch.");
    }
    if(SplitID == CurrID){
        //改变当前分支引用头
        auto plan = Repository::GenerateMergePlan(curr, curr, given);
        Repository::ApplyMergePlan(plan, index, curr, given, ignore);
        index.save();
        RefStore::WriteRef(CurrBranch, GivenID);
        std::cout << "Current branch fast-forwarded.\n";
        return;
    }
    auto plan = Repository::GenerateMergePlan(Split, curr, given);
    bool HasConflict = Repository::ApplyMergePlan(plan, index, curr, given, ignore);
    index.save();
    //生成mergecommit
    Commit MergeCommit;
    MergeCommit.parents.push_back(CurrID);   // 第一父提交
    MergeCommit.parents.push_back(GivenID);  // 第二父提交
    MergeCommit.message = "Merged " + branch + " into " + CurrBranch + ".";
    MergeCommit.timestamp = static_cast<long long>(std::time(nullptr));
    MergeCommit.snapshot = curr.snapshot;
    for (auto& p : index.StagedAdd) MergeCommit.snapshot[p.first] = p.second;
    for (auto& file : index.StagedRemove) MergeCommit.snapshot.erase(file);
    Repository::SaveCommit(MergeCommit);
    index.clear();
    index.save();
    if(HasConflict){
        std::cout << "Encountered a merge conflict.\n";
    }
}

// ---------------- Bonus-1：add-remote / rm-remote / push / fetch / pull ----------------

void SomeObj::addRemote(const std::string&, const std::string&) {
    // TODO: 保存远程仓库地址
}

void SomeObj::rmRemote(const std::string&) {
    // TODO: 删除远程仓库信息
}

void SomeObj::push(const std::string&, const std::string&) {
    // TODO: 将当前分支的提交推送到远程分支
}

void SomeObj::fetch(const std::string&, const std::string&) {
    // TODO: 将远程分支的提交与 blob 复制到本地
}

void SomeObj::pull(const std::string&, const std::string&) {
    // TODO: fetch 后将取回的分支合并到当前分支
}

// ---------------- Bonus-2：diff / show ----------------

void SomeObj::diff() {
    // TODO: 比较当前提交与工作目录并输出差异，无差异时不输出
}

void SomeObj::diffWithCommit(const std::string&) {
    // TODO: 解析 revision，比较指定提交与工作目录，无差异时不输出
}

void SomeObj::diffBetween(const std::string&, const std::string&) {
    // TODO: 解析两个 revision 并比较对应提交，无差异时不输出
}

void SomeObj::show() {
    // TODO: 显示当前提交及其相对第一父提交的差异
}

void SomeObj::show(const std::string&) {
    // TODO: 显示指定提交或标签及其相对第一父提交的差异
}
