# periodic_task_app

`periodic_task_app` 是一个周期任务应用。

它负责处理周期 tick、更新内部状态，并通过遥测上报计数和执行结果。

这个目录里主要包含：

- `fsw/src`：主循环、tick 处理、命令处理和辅助逻辑
- `fsw/inc`：事件号、功能码和配置接口
- `config`：消息、表和平台配置
- `fsw/tables`：默认表
