#ifndef REFSTORE_H
#define REFSTORE_H
#include <string>
class RefStore{
    public:
        static std::string ReadRef(const std::string& branch);
        static void WriteRef(const std::string& branch,const std::string& CommitId);
        static std::string ReadHEAD();
        static void WriteHEAD(const std::string& branch);
};
#endif