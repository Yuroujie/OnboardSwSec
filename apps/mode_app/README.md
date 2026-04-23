# mode_app

`mode_app` 是一个模式管理应用。

它负责接收地面命令、维护当前模式，并在满足策略条件时执行模式切换。

这个目录里主要包含：

- `fsw/src`：主循环、命令分发和模式处理逻辑
- `fsw/inc`：事件号、功能码和配置接口
- `config`：消息、表和平台配置
- `fsw/tables`：默认表

如果需要看主要入口，可以先从 `fsw/src/mode_app.c` 和 `fsw/src/mode_app_dispatch.c` 开始。
