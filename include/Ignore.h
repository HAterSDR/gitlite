#ifndef IGNORE_H
#define IGNORE_H

#include<string>
#include<vector>

class Ignore{
public:
    void load();
    bool IsIgnored(const std::string& filename)const;
private:
    enum class TokType { Star, Qmark, Lit };
    struct Tok {
        TokType type;
        char ch;
    };//打包可能出现的所有字符
    struct Rule{
        bool negated;
        std::vector<Tok>tokens;
    };//存储某一行模式串字符化之后的信息，是否取反等
    std::vector<Rule> rules;
    static std::vector<Tok> tokenize(const std::string& pattern, size_t start);
    static bool MatchTokens(const std::vector<Tok>& tokens, const std::string& s);
};
#endif //IGNORE_H