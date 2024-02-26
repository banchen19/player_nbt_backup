#include "Entry.h"
#include "Global.h"
#include <filesystem>

#include <chrono>
#include <fmt/format.h>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <thread>
ll::Logger logger(PLUGIN_NAME);
using namespace std;
using namespace ll::schedule;
using namespace ll::chrono_literals;

namespace change_this {

namespace {
namespace fs = std::filesystem;
// 需要一个计划任务：定时保存全部玩家的nbt数据

// 创建调度器的实例
ServerTimeScheduler s;

// ll3 事件注册，玩家进入游戏
ll::event::ListenerPtr playerJoinEventListener;

// ll3 事件注册，玩家退出游戏
ll::event::ListenerPtr player_life_game_Listener;

// ll3 事件注册，玩家重生事件
ll::event::ListenerPtr player_respawn_Listener;

std::unique_ptr<std::reference_wrapper<ll::plugin::NativePlugin>> selfPluginInstance;

auto disable(ll::plugin::NativePlugin& /*self*/) -> bool {
    auto& eventBus = ll::event::EventBus::getInstance();
    // 取消事件：玩家进入游戏
    eventBus.removeListener(playerJoinEventListener);
    // 取消事件：玩家退出游戏
    eventBus.removeListener(player_life_game_Listener);
    // 取消事件：玩家重生事件
    eventBus.removeListener(player_respawn_Listener);
    return true;
}

// 不可更改的函数方法：创建路径
string create_path_str(mce::UUID uuid) {
    string plugin_name  = (PLUGIN_NAME);
    string dataFilePath = getSelfPluginInstance().getDataDir().string();
    string player_path  = dataFilePath + "/" + uuid.asString() + ".json";
    return player_path;
}

// 不可更改的函数方法：读取指定文件内的数据并写给玩家
void read_fileto_playernbt(mce::UUID uuid) {
    GMLIB_Player::deletePlayerNbt(uuid);
    string            player_path = create_path_str(uuid);
    std::ifstream     infile(player_path);
    std::stringstream buffer;
    buffer << infile.rdbuf();
    std::string str(buffer.str());
    auto        file_toplayer_nbt = CompoundTag::fromSnbt(str);
    GMLIB_Player::setPlayerNbt(uuid, *file_toplayer_nbt);
}

// 不可更改的函数方法：从玩家获取nbt并写入文件
void player_nbt_write_to_file(Player* player) {
    auto& logger = getSelfPluginInstance().getLogger();
    if (player) {
        auto player_nbt =CompoundTag{};
        const auto   uuid       = player->getUuid();
        player->save(player_nbt);
        if (!player_nbt.isEmpty()) {    
            string     player_path = create_path_str(uuid);
            const auto nbt_string  = player_nbt.toSnbt();
            ofstream   outfile(player_path);
            outfile << nbt_string;
            outfile.close();
        } else {
            logger.error("Player NBT is null for UUID: {}", uuid.asString());
        }
    } else {
        logger.error("Player is null");
    }
}


bool isFileExists_ifstream(const string& name) { return std::filesystem::exists(name); }

void periodicTask() {
    auto level = ll::service::getLevel();
    if (level.has_value()) {
        level->forEachPlayer([&](Player& player) {
            change_this::player_nbt_write_to_file(&player);
            return true;
        });
    }
}

auto enable(ll::plugin::NativePlugin& self) -> bool {
    auto& eventBus = ll::event::EventBus::getInstance();
    auto& logger   = self.getLogger();

    // 10秒一次
    s.add<RepeatTask>(10s, [&] { periodicTask(); });

    //玩家已经进入服务器
    playerJoinEventListener = eventBus.emplaceListener<ll::event::player::PlayerJoinEvent>(
        [&logger, &self](ll::event::player::PlayerJoinEvent& event) {
            auto&      player = event.self();
            const auto uuid   = player.getUuid();
            if (isFileExists_ifstream(create_path_str(uuid))) {
                read_fileto_playernbt(uuid);
            }
        }
    );

    // 玩家退出游戏
    player_life_game_Listener = eventBus.emplaceListener<ll::event::player::PlayerLeaveEvent>(
        [&logger, &self](ll::event::player::PlayerLeaveEvent& event) {
            auto&  player       = event.self();
            change_this::player_nbt_write_to_file(&player);
        }
    );
    
    // 玩家重生
    player_respawn_Listener = eventBus.emplaceListener<ll::event::player::PlayerRespawnEvent>(
        [&logger, &self](ll::event::player::PlayerRespawnEvent& event) {
            auto&  player       = event.self();
            change_this::player_nbt_write_to_file(&player);
        }
    );


    return true;
}

auto load(ll::plugin::NativePlugin& self) -> bool {
    selfPluginInstance  = std::make_unique<std::reference_wrapper<ll::plugin::NativePlugin>>(self);
    string datafile_dir = self.getDataDir().string();
    std::filesystem::create_directories(datafile_dir);
    return true;
}

auto unload(ll::plugin::NativePlugin& self) -> bool {
    auto& logger = self.getLogger();
    selfPluginInstance.reset();

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
