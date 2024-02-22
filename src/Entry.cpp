#include "Entry.h"
#include "Global.h"
#include <fmt/format.h>
#include <functional>
#include <ll/api/Config.h>
#include <ll/api/io/FileUtils.h>
#include <ll/api/plugin/NativePlugin.h>
#include <ll/api/plugin/PluginManagerRegistry.h>


#include <direct.h>
#include <fstream>
#include <io.h>
#include <iostream>
#include <string>


#include <memory>
#include <stdexcept>
ll::Logger logger(PLUGIN_NAME);

namespace change_this {

namespace {


using namespace std;
// ll3 事件注册，玩家进入游戏
ll::event::ListenerPtr playerJoinEventListener;
// ll3 事件注册，玩家退出游戏
ll::event::ListenerPtr player_life_game_Listener;
// ll3 事件注册，玩家重生事件
ll::event::ListenerPtr player_respawn_Listener;
// ll3 事件注册，玩家跳跃事件
ll::event::ListenerPtr player_jump_Listener;
std::unique_ptr<std::reference_wrapper<ll::plugin::NativePlugin>>
    selfPluginInstance; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

auto disable(ll::plugin::NativePlugin& /*self*/) -> bool {
    logger.info("disabling...");

    auto& eventBus = ll::event::EventBus::getInstance();
    // 取消事件：玩家进入游戏
    eventBus.removeListener(playerJoinEventListener);
    // 取消事件：玩家退出游戏
    eventBus.removeListener(player_life_game_Listener);
    // 取消事件：玩家重生事件
    eventBus.removeListener(player_respawn_Listener);
    // 取消事件：玩家跳跃事件
    eventBus.removeListener(player_jump_Listener);

    logger.info("disabled");
    return true;
}

// 检测文件是否存在
bool isFileExists_ifstream(string& name) {
    ifstream f(name.c_str());
    return f.good();
}

// 读取文件内的nbt给玩家
bool read_fileto_playernbt(mce::UUID uuid, string player_path) {

    // 删除玩家的nbt
    GMLIB_Player::deletePlayerNbt(uuid);

    ifstream infile;
    // 打开文件player_path
    ifstream fin(player_path);
    // stringstream object
    stringstream buffer;
    // read file content in stringstream object
    buffer << fin.rdbuf();
    // store file content in a string
    string str(buffer.str());
    // 使用CompoundTag::fromSnbt将data转换为CompoundTag
    // logger.info("玩家snbt:\n{}", str);
    auto file_toplayer_nbt = CompoundTag::fromSnbt(str);
    // 所以这里的第二个参数需要*来解引用
    auto player_write = GMLIB_Player::setPlayerNbt(uuid, *file_toplayer_nbt);
    // 检查是写入到玩家nbt成功
    return player_write;
}


// 将玩家的nbt写入到文件
bool player_write_nbt(mce::UUID uuid, string player_path) {
    auto player_nbt = GMLIB_Player::getPlayerNbt(uuid);
    // 将snbt转换为snbt
    const auto nbt_string = player_nbt->toSnbt();
    // 输出到文件player_path，没有就创建
    ofstream outfile(player_path);
    // 输入nbt_string
    outfile << nbt_string;
    // logger.info("玩家snbt:\n{}", nbt_string);
    // 关闭文件
    outfile.close();
    return true;
}

auto enable(ll::plugin::NativePlugin& self) -> bool {
    auto& logger = getSelfPluginInstance().getLogger();
    logger.info("enabling...");

    auto& eventBus = ll::event::EventBus::getInstance();

    // 注册玩家登录事件监听
    playerJoinEventListener = eventBus.emplaceListener<GMLIB::Event::PlayerEvent::PlayerLoginAfterEvent>(
        [&logger, &self](GMLIB::Event::PlayerEvent::PlayerLoginAfterEvent& event) {
            // 从SDK-GMLIB\GMLIB\Event\Player\PlayerLoginEvent.h 拿到uuid  这里拿到的uuid是mce::uuid
            auto   uuid         = event.getUuid();
            auto   player_name  = event.getRealName();
            string dataFilePath = self.getDataDir().string();

            // 将uuid字符串化
            string player_path = dataFilePath + "/" + uuid.asString() + ".json";
            // 查询存储玩家的snbt文件是否存在
            bool ret = isFileExists_ifstream(player_path);
            if (ret) {
                logger.info("正在读取玩家{}->数据文件： {}", player_name, player_path);
                auto player_write = read_fileto_playernbt(uuid, player_path);

                if (player_write) {
                    logger.info("读取成功->{}", player_name);
                } else {
                    logger.info("读取失败->{}", player_name);
                }
            } else {
                logger.info("正在初始化玩家{}-> 数据文件： {}", player_name, player_path);
                // 通过离线api获取玩家CompoundTag
                player_write_nbt(uuid, player_path);
            }
        }
    );

    // 注册玩家退出游戏监听
    player_life_game_Listener = eventBus.emplaceListener<ll::event::player::PlayerLeaveEvent>(
        [&logger, &self](ll::event::player::PlayerLeaveEvent& event) {
            // 从SDK-GMLIB\GMLIB\Event\Player\PlayerLoginEvent.h 拿到uuid  这里拿到的uuid是mce::uuid
            auto&  player       = event.self();
            auto   uuid         = player.getUuid();
            auto   player_name  = player.getRealName();
            string dataFilePath = self.getDataDir().string();
            string player_path  = dataFilePath + "/" + uuid.asString() + ".json";
            player_write_nbt(uuid, player_path);
        }
    );
    // 玩家重生
    player_respawn_Listener = eventBus.emplaceListener<ll::event::player::PlayerRespawnEvent>(
        [&logger, &self](ll::event::player::PlayerRespawnEvent& event) {
            // 从SDK-GMLIB\GMLIB\Event\Player\PlayerLoginEvent.h 拿到uuid  这里拿到的uuid是mce::uuid
            auto&  player       = event.self();
            auto   uuid         = player.getUuid();
            auto   player_name  = player.getRealName();
            string dataFilePath = self.getDataDir().string();
            string player_path  = dataFilePath + "/" + uuid.asString() + ".json";
            player_write_nbt(uuid, player_path);
        }
    );

    // ll3 事件注册，玩家跳跃事件
    player_jump_Listener = eventBus.emplaceListener<ll::event::player::PlayerJumpEvent>(
        [&logger, &self](ll::event::player::PlayerJumpEvent& event) {
            // 从SDK-GMLIB\GMLIB\Event\Player\PlayerLoginEvent.h 拿到uuid  这里拿到的uuid是mce::uuid
            auto&  player       = event.self();
            auto   uuid         = player.getUuid();
            auto   player_name  = player.getRealName();
            string dataFilePath = self.getDataDir().string();
            string player_path  = dataFilePath + "/" + uuid.asString() + ".json";
            player_write_nbt(uuid, player_path);
        }
    );
    logger.info("enabled");
    return true;
}


auto load(ll::plugin::NativePlugin& self) -> bool {
    logger.info("玩家nbt数据备份加载中");

    selfPluginInstance = std::make_unique<std::reference_wrapper<ll::plugin::NativePlugin>>(self);

    string plugin_name  = (PLUGIN_NAME);
    string datafile_dir = self.getDataDir().string();
    int    i            = mkdir(datafile_dir.c_str());
    logger.info("玩家nbt数据备份加载完毕");

    return true;
}

auto unload(ll::plugin::NativePlugin& self) -> bool {
    auto& logger = self.getLogger();

    logger.info("unloading...");

    selfPluginInstance.reset();

    // Your code here.

    logger.info("unloaded");

    return true;
}

} // namespace

auto getSelfPluginInstance() -> ll::plugin::NativePlugin& {
    if (!selfPluginInstance) {
        throw std::runtime_error("selfPluginInstance is null");
    }

    return *selfPluginInstance;
}

} // namespace change_this

extern "C" {
_declspec(dllexport) auto ll_plugin_disable(ll::plugin::NativePlugin& self) -> bool {
    return change_this::disable(self);
}
_declspec(dllexport) auto ll_plugin_enable(ll::plugin::NativePlugin& self) -> bool { return change_this::enable(self); }
_declspec(dllexport) auto ll_plugin_load(ll::plugin::NativePlugin& self) -> bool { return change_this::load(self); }
_declspec(dllexport) auto ll_plugin_unload(ll::plugin::NativePlugin& self) -> bool { return change_this::unload(self); }
}
