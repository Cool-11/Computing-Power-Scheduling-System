# 星环算力调度系统

课程设计的 C++17 交互式调度器。完整规则以 OJ 题面为准；[开发计划与规格](docs/开发计划.md)记录实现顺序、状态口径、验证步骤和已知限制。

## 构建与测试

```bash
g++ -std=c++17 -O2 -pipe -Wall -Wextra -o scheduler src/main.cpp
python3 tests/test_protocol.py ./scheduler
```

向 OJ 提交 `src/main.cpp`。程序每轮输出 `ANS`、零条或多条 `RUN`、`END`，并刷新标准输出。

本地仓库的 `origin` 指向 `git@github.com:Cool-11/Computing-Power-Scheduling-System.git`。目前没有题目下载包中的本地评测程序；先用仓库内的交互测试和合成压力输入排除可复现错误，再根据 OJ 返回的分数迭代。OJ 提交机会有限，应保存每次提交对应的 Git 提交号和分数。
