//用于存储object
#include "../include/ObjectStore.h"
#include "../include/Utils.h"
std::string ObjectStore::WriteObject(const std::string& type,const std::string& content){
    //将内容转换为sha后写入objects；type用来区分文件名和commit记录
    std::string full = type + " " + content;
    std::string sha = Utils::sha1(full);
    std::string dir = ".gitlite/objects/" + sha.substr(0,2);
    std::string filepath = dir + sha.substr(2);
    if(!Utils::isFile(filepath)){
        Utils::createDirectories(dir);
        Utils::writeContents(filepath, full);
    }
    return sha;
};
std::string ObjectStore::ReadObject(std::string id){
    std::string type_sha = id.substr(0,2);
    std::string content_sha = id.substr(2);
    std::string filepath = ".gitlite/objects/"+ type_sha + "/" + content_sha;
    std::string full;
    if(Utils::isFile(filepath)){
        full = Utils::readContentsAsString(filepath);
    }
    return full;
};