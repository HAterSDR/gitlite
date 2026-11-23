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
    //去掉开头的commit
    std::string CleanText = text;
    if (CleanText.rfind("commit ", 0) == 0) {
        CleanText = CleanText.substr(7); 
    }
    std::istringstream in(CleanText);
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
bool Commit::isTracked(const std::string& filename) const{
    return snapshot.count(filename) > 0;
}
std::string Commit::id() const{
    std::string text = (*this).Serialize();
    return Utils::sha1("commit " + text);
}
void Commit::Print() const{
    Commit current = *this;
    std::string CurrentID = id(); 
    std::cout << "===\n";
    std::cout << "commit "<< CurrentID << "\n";
    if(current.parents.size() >= 2){
        std::cout << "Merge: " << current.parents[0].substr(0,7) << " " << current.parents[1].substr(0,7) << "\n";
    }
    std::time_t t = static_cast<std::time_t>(current.timestamp);
    std::tm* tm = std::localtime(&t);
    char TimeBuffer[80];
    std::strftime(TimeBuffer, sizeof(TimeBuffer), "%a %b %d %H:%M:%S %Y %z", tm);
    std::cout << "Date: " << TimeBuffer << "\n";
    std::cout << current.message << "\n\n";
};
// 建议类 Commit 的实现文件，可按需使用或删除。
