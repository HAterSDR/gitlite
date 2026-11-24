# 实现类的结构
## Commit
对一次commit的抽象
包含parents, message, timestamp, snapshot四个类成员，分别对应父提交、消息、时间戳、文件快照（文件名和blobID）

序列化和反序列化函数负责将Commit转化为文本与将文本转化为Commit。序列化文本大致为
类型名1 值
类型名2 值
的格式

isTracked函数负责判断某个文件当前是否被该commit跟踪
id函数负责返回该commit的sha值

print用在log中，负责打印该commit的全部信息。

## GitliteException
未修改，未用到

## Ignore
实现对gitliteignore的解析

TokType用于判断当前字符属于哪一类，分为* ? 和普通字符

Tok将类型和字符值打包

Rule存储模式串字符化（也就是将每个字符转化为Tok）之后的信息

rules存储转化为rule的字符

MatchTokens判断gitliteignore中内容与文件名是否匹配，使用动态规划算法

## Index
实现对暂存区的管理

StageAdd用于记录暂存待添加改动

StageRemove用于记录暂存待删除

load函数从暂存区 (.gitlite/index) 中加载add与remove内容

save函数将当前index内容写入index区中

clear函数清空当前index对象内容

IsStageAdd判断filename是否暂存待添加

IsStageRemove判断filename是否暂存待删除

## ObjectStore
实现对对象(blob commit)的读写

WriteObject函数向objects中写入id内容，具体结构见注释

ReadObject函数从objects中读取id对应内容，即commit和blob的具体序列化格式内容

BlobID函数用于返回对应文件转换后的ID

ReadBlob函数用于读取id对应的blob，若不存在则返回空字符串

## RefStore
实现对refs中remotes(remote分支所在，结构类似[远程名]/[远程分支名]), heads(记录每个分支名对应的首个提交ID), tags(记录每个tag对应的CommitID)

ReadRef读取branch中对应的CommitID

WriteRef向branch中写入对应的CommitID

ReadHEAD读取当前HEAD中的分支名

WriteHEAD向HEAD中写入branch名

ReadTag返回tag名中内容

WriteTag向tag名中写入CommitID

GetRefPath负责解析名字判断是remote还是local并给ReadRef和WriteRef使用

CollectRefs从refs文件下某个子内容中获取所有内容，用于global-l;og和status

## Repository
功能整合，将前面的类的功能合并为一些函数供SomeObj使用

MergeAcion,MergeItem用于处理merge，与merge一起讲

getGitliteDir为原有函数

Init进行git初始化

GetCurrentBranch返回当前branch名

GetCurrentCommit返回当前指向的最新commit

SaveCommit将commit保存为最新commit，改变当前分支指向

ResolveRevision负责解析revision，返回对应ID

RestoreFromCommit将commit中对应的filename的文本内容写入工作区

CheckOutSnapshot负责用target内容对比current内容后修改工作区文件

HasUntracked负责检查target中是否存在Untracked文件，即在工作区中但未被current跟踪未暂存待添加未被忽略的内容

GetCommitbyID负责获取Commit对应的ID

FindSplit函数用以寻找两个commit的分割点，从而处理merge

GenerateMergePlan用以返回merge对每个文件的行为，打包为元素是Repository::MergeItem的std::vector 
MergeItem记录文件名，merge动作，blobid
MergeAction将merge中各种对文件的行为打包为一个类型

ApplyMergePlan根据拿到的plan进行merge行为

ReadConfig负责读取config中内容（远程名与对应的路径），返回一个map

SaveConfig负责将map内容全部重新写入config中

CopyObjects负责将一处repo的objects全部复制到另一处

IsAncestors负责判断ID1是不是ID2的祖先

## Someobj
内容说明见代码注释