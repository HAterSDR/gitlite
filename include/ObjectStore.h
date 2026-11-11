#ifndef OBJECTSTORE_H
#define OBJECTSTORE_H
#include <string>
class ObjectStore{
    public:
        static std::string WriteObject(const std::string& type,const std::string& content);
        static std::string ReadObject(std::string id);
};

#endif