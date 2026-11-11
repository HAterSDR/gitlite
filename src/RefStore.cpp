//用于判断当前版本
#include "../include/RefStore.h"
#include "../include/Utils.h"
std::string RefStore::ReadRef(const std::string& branch){
    //读取ref中的branch名，返回commitid
    std::string filename = ".gitlite/refs/heads/" + branch;
    std::string CommitID = "";
    if(Utils::isFile(filename)){
        CommitID = Utils::readContentsAsString(filename);
    }
            return CommitID;
};
void RefStore::WriteRef(const std::string& branch,const std::string& CommitID){
    //向ref中的branch名写入commitid
    std::string filename = ".gitlite/refs/heads/" + branch;
    Utils::writeContents(filename,CommitID);
};
std::string RefStore::ReadHEAD(){
    //读取当前分支名
    std::string path = "/.gitlite/HEAD";
    std::string content = Utils::readContentsAsString(path);
    std::string prefix = "ref: refs/heads/";
    if(content.rfind(prefix,0) == 0) return content.substr(prefix.length());
    return "";
};
void RefStore::WriteHEAD(const std::string& branch){
    //改变当前分支
    std::string path = "/.gitlite/HEAD";
    std::string content = "ref: refs/heads/" + branch;
    Utils::writeContents(path, content);
};