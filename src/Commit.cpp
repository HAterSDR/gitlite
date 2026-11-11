#include "../include/Commit.h"
#include "../include/Utils.h"
#include <sstream>
Commit::Commit(){
    timestamp = 0;
}
std::string Commit::Serialize() const{
    //序列化commit的信息
    std::ostringstream packet;
    for(const std::string& p:parents){
        packet <<"parent "<< p << "\n";
    }
    packet << "time "<< timestamp << "\n";
    packet << "message "<< message << "\n";
    for(const auto& pair:snapshot){
        packet << "file " << pair.first << " " << pair.second << "\n";
    }
    return packet.str();
}
Commit Commit::Deserialize(const std::string& text){
    Commit commit_;
    //反序列化commit的信息
    std::istringstream in;
    std::string line;
    while(getline(in, line)){
        if(line.empty()) continue;
        if(line.rfind("parent ", 0) == 0) commit_.parents.push_back(line.substr(7));
        if(line.rfind("time ", 0) == 0) commit_.timestamp = std::stoll(line.substr(5));
        if(line.rfind("message ", 0) == 0) commit_.message = line.substr(8);
        if(line.rfind("file ", 0) == 0){
            std::istringstream linestream(line.substr(5));
            std::string filename, blobID;
            linestream >> filename >> blobID;
            commit_.snapshot[filename] = blobID;
        }
    }
    return commit_;
}
// 建议类 Commit 的实现文件，可按需使用或删除。
