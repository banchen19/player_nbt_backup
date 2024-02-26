# GMLIB-Plugin-Template——玩家nbt数据备份
Template for plugins using GMLIB

测试环境：Version: 1.20.62.02(ProtocolVersion 649) with LeviLamina-0.9.0+dc310d1e7
[GMLIB] Version: 0.9.1

本示例同时使用了ll与gmlib的api。

本项目代码所有权归属于 PHEyeji（QQ号：1102581553）

提供了非常详细的注释
--
玩家进入游戏时创建数据文件，位于data文件夹中，
当删除data文件夹时候，会重新创建玩家数据文件
如果玩家数据文件存在，则会读取玩家的nbt数据，写入给玩家，由于为了方便操作玩家nbt，写入时用是snbt，如果你有需求，可以找到玩家的数据文件进行修改，当然这样并不安全。但是提供了这样一种操作。

---
感谢 来自PHEyeji（QQ号：1102581553）的定制，以及IKUN大佬（项目来源：https://github.com/GroupMountain ）的技术帮助，

