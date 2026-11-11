#include "../include/Repository.h"
#include "../include/Utils.h"       
#include "../include/RefStore.h"   
#include "../include/ObjectStore.h" 
#include "../include/Commit.h"
// main.cpp 的 checkCWD 依赖此方法，请保留。
std::string Repository::getGitliteDir() {
    return ".gitlite";
}
