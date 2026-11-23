//用于管理暂存区

#include "../include/Index.h"
#include "../include/Utils.h"

#include <sstream>
#include <fstream>
void Index::load(){
    //从index中加载add与remove内容
    clear();
    std::string path = ".gitlite/index";
    if(!Utils::isFile(path)) return;
    std::string content = Utils::readContentsAsString(path);
    std::istringstream in(content);
    std::string line;
    while(std::getline(in, line)){
        if(line.empty()) continue;
        std::istringstream linestream(line);
        std::string action, filename, BlobID;
        linestream >> action >> filename;
        if(action == "add"){
            linestream >> BlobID;
            StagedAdd[filename] = BlobID;
        }
        if(action == "remove"){
            StagedRemove.insert(filename);
        }
    }
}
void Index::save() const{
    //向index中写入add和remove内容
    std::ostringstream out;
    for(const auto& pair : StagedAdd){
        out << "add " << pair.first << " " << pair.second << "\n";
    }
    for(const std::string& filename : StagedRemove){
        out << "remove " << filename << "\n";
    }
    std::string path = ".gitlite/index";
    Utils::writeContents(path,out.str());
}
void Index::clear(){
    StagedAdd.clear();
    StagedRemove.clear();
}
bool Index::IsStageAdd(const std::string& filename) const{
    return StagedAdd.count(filename) > 0;
}
bool Index::IsStageRemove(const std::string& filename) const{
    return StagedRemove.count(filename) > 0;
}