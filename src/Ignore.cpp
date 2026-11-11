//用于处理gitignore

#include "../include/Ignore.h"
#include "../include/Utils.h"

#include<fstream>

void Ignore::load(){
    rules.clear();

    const std::string path = ".gitliteignore";
    if(!Utils::isFile(path)) return;
    std::ifstream in(path,std::ios::binary);
    if(!in.is_open())return;

    std::string line;
    while(std::getline(in,line)){
        if(line.empty()) continue;
        if(line.back()=='\r') line.pop_back();
        if(line[0] == '#') continue;
        Rule rule;
        size_t pos = 0;
        if(line[0] == '!'){
            rule.negated = true;
            pos = 1;
        } else{
            rule.negated = false;
        }
        if(pos < line.size() && line[pos] == '/') ++pos;
        rule.tokens = tokenize(line,pos);
        if(rule.tokens.empty()) continue;

        rules.push_back(std::move(rule));
    }
};//读取并解析gitliteignore

bool Ignore::IsIgnored(const std::string& filename) const{
    bool ignored = false;
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
