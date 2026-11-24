//用于判断当前版本
#include "../include/RefStore.h"
#include "../include/Utils.h"
#include<dirent.h>
std::string RefStore::ReadRef(const std::string& branch){
    //读取ref中的branch名，返回commitid
    std::string filename = GetRefPath(branch);
    std::string CommitID = "";
    if(Utils::isFile(filename)){
        CommitID = Utils::readContentsAsString(filename);
    }
    return CommitID;
};
void RefStore::WriteRef(const std::string& branch,const std::string& CommitID){
    //向ref中的branch名写入commitid
    std::string filename = GetRefPath(branch);
    Utils::writeContents(filename,CommitID);
};
std::string RefStore::ReadHEAD(){
    //读取当前HEAD指向的分支名, HEAD.txt的内容类似ref: refs/heads/[当前分支名]
    std::string path = ".gitlite/HEAD";
    std::string content = Utils::readContentsAsString(path);
    std::string prefix = "ref: refs/heads/";
    if(content.rfind(prefix,0) == 0) return content.substr(prefix.length());
    return "";
};
void RefStore::WriteHEAD(const std::string& branch){
    //改变当前分支为branch
    std::string path = ".gitlite/HEAD";
    std::string content = "ref: refs/heads/" + branch;
    Utils::writeContents(path, content);
};
std::string RefStore::ReadTag(const std::string& name){
    //返回指定tag名中的内容
    std::string path = ".gitlite/refs/tags/" + name;
    if (!Utils::isFile(path)) return "";
    return Utils::readContentsAsString(path);
};
std::string RefStore::WriteTag(const std::string& name, const std::string& CommitID){
    //向指定tag文件中写入commitid，如果不存在则创建
    std::string path = ".gitlite/refs/tags/" + name;
    Utils::writeContents(path, CommitID);
    return CommitID;
};
std::string RefStore::GetRefPath(const std::string& name){
    //解析传入的branch，判断是远程名还是本地分支名
    if(name.find('/') != std::string::npos){
        return ".gitlite/refs/remotes/" + name;
    }
    return ".gitlite/refs/heads/" + name;
}
void RefStore::CollectRefs(const std::string& dir, const std::string& prefix, std::vector<std::pair<std::string, std::string>>& refs){
    //从refs文件下某个子内容中获取所有内容
     DIR* handle = opendir(dir.c_str());
    if (handle == nullptr) return;

    struct dirent* entry;
    while ((entry = readdir(handle)) != nullptr) {
        //拿到名字
        std::string name = entry->d_name;
        if (name == "." || name == "..") continue;

        std::string path = dir + "/" + name;
        //如果prefix为空，说明在第一层，如果不为空，说明在第二层
        std::string RefName = prefix.empty() ? name : prefix + "/" + name;
        //是文件，直接push入refs中，不是文件，递归进入文件夹，加上prefix
        if (Utils::isFile(path)) {
            refs.push_back({RefName, Utils::readContentsAsString(path)});
        } else if (Utils::isDirectory(path)) {
            CollectRefs(path, RefName, refs);
        }
    }
    closedir(handle);
}