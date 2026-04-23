# cFS 示例应用集

这个仓库整理了三个基于 cFS 的应用：

- `mode_app`：模式管理与命令受理
- `periodic_task_app`：周期任务执行与状态计数
- `buffer_mgr_app`：缓冲数据暂存与释放

这三个应用都按完整 cFS 应用的方式组织，保留了命令处理、遥测、配置头文件、表文件和单元测试相关目录。

## 应用说明

### mode_app

`mode_app` 用来维护应用当前模式，并根据命令受理策略执行模式切换。

### periodic_task_app

`periodic_task_app` 用来处理周期 tick、更新内部状态，并记录执行过程中的计数信息。

### buffer_mgr_app

`buffer_mgr_app` 用来暂存输入数据、维护缓冲状态，并在满足条件时打开释放窗口。

## 目录结构

每个应用都保持类似的目录布局：

- `fsw/src`：主程序、命令处理、分发逻辑和辅助函数
- `fsw/inc`：对外头文件、事件号和功能码定义
- `config`：消息、表和配置相关头文件
- `fsw/tables`：默认表内容
- `unit-test`：单元测试和覆盖率相关文件
