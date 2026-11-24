//用于处理gitignore

#include "../include/Ignore.h"
#include "../include/Utils.h"

#include<fstream>

void Ignore::load(){
    //根据gitliteignore的内容重新加载类中内容,将每一行内容处理# ! \r / 等无用信息后存入rules中
    rules.clear();

    const std::string path = ".gitliteignore";
    if(!Utils::isFile(path)) return;
    std::ifstream in(path,std::ios::binary);
    if(!in.is_open())return;

    std::string line;
    while(std::getline(in,line)){
        if(line.empty()) continue;
        //去除末尾的'\r'
        if(line.back()=='\r') line.pop_back();
        //注释，直接跳过
        if(line[0] == '#') continue;
        Rule rule;
        size_t pos = 0;
        //规则取反
        if(line[0] == '!'){
            rule.negated = true;
            pos = 1;
        } else{
            rule.negated = false;
        }
        //如果开头有'/'，跳过处理下一个位置
        if(pos < line.size() && line[pos] == '/') ++pos;
        //将当前字符转化为tok后存入rule中
        rule.tokens = tokenize(line,pos);
        if(rule.tokens.empty()) continue;

        rules.push_back(std::move(rule));
    }
};//读取并解析gitliteignore

bool Ignore::IsIgnored(const std::string& filename) const{
    bool ignored = false;
    //调用MatchTokens判断文件名是否匹配，按最后一条计
    for(const auto& rule: rules){
        if(MatchTokens(rule.tokens,filename)) ignored = !rule.negated;
    }
    return ignored;
};

std::vector<Ignore::Tok> Ignore::tokenize(const std::string& pattern, size_t start){
    //start用来处理首行是！\等情况
    std::vector<Tok> toks;
    for(size_t i = start; i < pattern.size(); ++i){
        char c = pattern[i];
        switch (c){
            case ('\\'):{
                if(i + 1 < pattern.size()) toks.push_back({TokType::Lit, pattern[++i]});
                else toks.push_back({TokType::Lit, '\\'});
                break;
            };
            case ('*'):{
                if(toks.empty()||toks.back().type != TokType::Star){
                    toks.push_back({TokType::Star, 0});
                }//多个*合并成一个处理
                break;
            };
            case ('?'):{
                toks.push_back({TokType::Qmark, 0});
                break;
            }
            default:{
                toks.push_back({TokType::Lit,c});
            }
        };        
    }
    return toks;
}

bool Ignore::MatchTokens(const std::vector<Tok>& tokens, const std::string& s){
    //动态规划算法求解匹配
    size_t n = s.size();
    std::vector<char> dp(n + 1,0);
    dp[0] = 1;
    for(const Tok& t:tokens){
        if(t.type == TokType::Star){
            for(size_t j = 1; j <= n; j++){
                if(dp[j-1]) dp[j] = 1;
            }
            continue;
        }
        if(t.type == TokType::Qmark){
            for(size_t j = n; j>=1;j--){
                dp[j] = dp[j - 1];
            }
            dp[0] = 0;
            continue;
        }
        for(size_t j = n; j>=1;j--){
            dp[j] = dp[j - 1] && (s[j-1]==t.ch);
        }
        dp[0] = 0;
    }
    return dp[n] != 0;
}
