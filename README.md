# 星环算力调度系统

课程设计的 C++17 交互式调度器。完整规则以 OJ 题面为准；[开发计划与规格](docs/开发计划.md)记录实现顺序、状态口径、验证步骤和已知限制。

## 构建与测试

```bash
g++ -std=c++17 -O2 -pipe -Wall -Wextra -o scheduler src/main.cpp
python3 tests/test_protocol.py ./scheduler
```

向 OJ 提交 `src/main.cpp`。程序每轮输出 `ANS`、零条或多条 `RUN`、`END`，并刷新标准输出。

本地仓库的 `origin` 指向 `git@github.com:Cool-11/Computing-Power-Scheduling-System.git`。本地的 `examples/1846_samples.zip` 是用户提供的样例包，内含逐轮样例、Windows 本地评测程序和调试工具；压缩包没有纳入 Git 仓库。调试工具需要一个顶层 `.in` 参数文件才能生成完整练习世界；仓库内测试与八组固定种子练习世界的结果记录在[开发计划](docs/开发计划.md)中。本地评测不提供正式数据的利润目标，提交 OJ 后仍应保存每次提交对应的 Git 提交号和分数。
