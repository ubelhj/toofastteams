#pragma once

#include "bakkesmod/plugin/bakkesmodplugin.h"
#include "bakkesmod/plugin/pluginwindow.h"
#include "../NetcodeManager/NetcodeManager.h"

#include <fstream>

#define nl(x) SettingsFile << std::string(x) << '\n'
#define blank SettingsFile << '\n'
#define cv(x) std::string(x)

#include "version.h"
constexpr auto plugin_version = stringify(VERSION_MAJOR) "." stringify(VERSION_MINOR) "." stringify(VERSION_PATCH) "." stringify(VERSION_BUILD);


class TooFastTeams: public BakkesMod::Plugin::BakkesModPlugin/*, public BakkesMod::Plugin::PluginWindow*/
{
private:
    std::shared_ptr<NetcodeManager> Netcode;

public:
    void onLoad() override;
    void onUnload() override;

    ServerWrapper GetCurrentGameState();

    void Render(CanvasWrapper canvas);

    void OnMessageReceived(const std::string& Message, PriWrapper Sender);

    void onTick();

    void GenerateSettingsFile();
};

