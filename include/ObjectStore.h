#ifndef OBJECTSTORE_H
#define OBJECTSTORE_H
#include <string>
class ObjectStore{
public:
    static std::string WriteObject(const std::string& type,const std::string& content);
    static std::string ReadObject(std::string id);
    static std::string BlobID(std::string content);
    static std::string ReadBlob(std::string id);
    
};

#endif