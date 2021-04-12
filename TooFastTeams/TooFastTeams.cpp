#include "pch.h"
#include "TooFastTeams.h"


BAKKESMOD_PLUGIN(TooFastTeams, "Too fast plugin for teams", plugin_version, PLUGINTYPE_FREEPLAY)

std::shared_ptr<CVarManagerWrapper> _globalCvarManager;

bool blueEnabled = false;
bool orangeEnabled = false;
float blueSpeedMultiplier = 1.01;
float blueSpeedThreshold = 2300.0;
float blueMaxSpeed = 2300.0;
float orangeSpeedMultiplier = 1.01;
float orangeSpeedThreshold = 2300.0;
float orangeMaxSpeed = 2300.0;
float turnThreshold = 0.5;

void TooFastTeams::onLoad()
{
	_globalCvarManager = cvarManager;

	cvarManager->registerCvar("toofastteams_max_blue", "2300", "sets blue max speed", true, true, 0)
		.addOnValueChanged([this](std::string cvarName, CVarWrapper cvar) {
			if (gameWrapper->IsInOnlineGame()) { return; }

			Netcode->SendMessage("maxb" + cvar.getStringValue());});

	cvarManager->registerCvar("toofastteams_max_orange", "2300", "sets orange max speed", true, true, 0)
		.addOnValueChanged([this](std::string cvarName, CVarWrapper cvar) {
			if (gameWrapper->IsInOnlineGame()) { return; }

			Netcode->SendMessage("maxo" + cvar.getStringValue());});

	cvarManager->registerCvar("toofastteams_mult_blue", "1.01", "sets blue speed multiplier", true, true, -1, true, 2)
		.addOnValueChanged([this](std::string cvarName, CVarWrapper cvar) {
			if (gameWrapper->IsInOnlineGame()) { return; }

			Netcode->SendMessage("multb" + cvar.getStringValue());});

	cvarManager->registerCvar("toofastteams_mult_orange", "1.01", "sets orange speed multiplier", true, true, -1, true, 2)
		.addOnValueChanged([this](std::string cvarName, CVarWrapper cvar) {
			if (gameWrapper->IsInOnlineGame()) { return; }

			Netcode->SendMessage("multo" + cvar.getStringValue());});

	cvarManager->registerCvar("toofastteams_threshold_blue", "2300", "sets blue speed threshold", true, true, 0)
		.addOnValueChanged([this](std::string cvarName, CVarWrapper cvar) {
			if (gameWrapper->IsInOnlineGame()) { return; }

			Netcode->SendMessage("threshb" + cvar.getStringValue());});

	cvarManager->registerCvar("toofastteams_threshold_orange", "2300", "sets orange speed threshold", true, true, 0)
		.addOnValueChanged([this](std::string cvarName, CVarWrapper cvar) {
			if (gameWrapper->IsInOnlineGame()) { return; }

			Netcode->SendMessage("thresho" + cvar.getStringValue());});

	gameWrapper->HookEventPost("Function TAGame.GameEvent_TA.AddCar",
		[this](...) { 
			if (gameWrapper->IsInOnlineGame()) { return; }
		
			Netcode->SendMessage("maxb" + cvarManager->getCvar("toofastteams_max_blue").getStringValue());
			Netcode->SendMessage("maxo" + cvarManager->getCvar("toofastteams_max_orange").getStringValue());
			Netcode->SendMessage("multb" + cvarManager->getCvar("toofastteams_mult_blue").getStringValue());
			Netcode->SendMessage("multb" + cvarManager->getCvar("toofastteams_mult_orange").getStringValue());
			Netcode->SendMessage("threshb" + cvarManager->getCvar("toofastteams_threshold_blue").getStringValue());
			Netcode->SendMessage("thresho" + cvarManager->getCvar("toofastteams_threshold_orange").getStringValue());

			cvarManager->log("sent messages");
		});
}

void TooFastTeams::onUnload()
{
}

ServerWrapper TooFastTeams::GetCurrentGameState() {
	if (gameWrapper->IsInReplay())
		return gameWrapper->GetGameEventAsReplay().memory_address;
	else if (gameWrapper->IsInOnlineGame())
		return gameWrapper->GetOnlineGame();
	else
		return gameWrapper->GetGameEventAsServer();
}

// FULFILL REQUEST //
void TooFastTeams::OnMessageReceived(const std::string& Message, PriWrapper Sender) {
	if (Sender.IsNull()) { return; }

	if (Message.find("max") == 0) {
		// gets speed value (right after prefix Speed)
		// either "b12.34" or "o12.34"
		std::string teamString = Message.substr(3, 1);

		std::string speedString = Message.substr(4);
		float speedValue;

		try {
			speedValue = std::stof(speedString);
		}
		catch (...) {
			cvarManager->log("error in receving new max speed (invalid number) on message: " + Message);
			return;
		}

		if (teamString == "b") {
			blueMaxSpeed = speedValue;
		} else if (teamString == "o") {
			orangeMaxSpeed = speedValue;
		} else {
			cvarManager->log("error in receving new max speed (invalid team) on message: " + Message);
		}
	} else if (Message.find("mult") == 0) {
		// gets speed value (right after prefix Speed)
		// either "b12.34" or "o12.34"
		std::string teamString = Message.substr(4, 1);

		std::string speedString = Message.substr(5);
		float speedValue;

		try {
			speedValue = std::stof(speedString);
		}
		catch (...) {
			cvarManager->log("error in receving new speed mult (invalid number) on message: " + Message);
			return;
		}

		if (teamString == "b") {
			blueSpeedMultiplier = speedValue;
		}
		else if (teamString == "o") {
			orangeSpeedMultiplier = speedValue;
		}
		else {
			cvarManager->log("error in receving new speed mult (invalid team) on message: " + Message);
		}
	} else if (Message.find("thresh") == 0) {
		// gets speed value (right after prefix Speed)
		// either "b12.34" or "o12.34"
		std::string teamString = Message.substr(6, 1);

		std::string speedString = Message.substr(7);
		float speedValue;

		try {
			speedValue = std::stof(speedString);
		}
		catch (...) {
			cvarManager->log("error in receving new speed thresh (invalid number) on message: " + Message);
			return;
		}

		if (teamString == "b") {
			blueSpeedThreshold = speedValue;
		}
		else if (teamString == "o") {
			orangeSpeedThreshold = speedValue;
		}
		else {
			cvarManager->log("error in receving new speed thresh (invalid team) on message: " + Message);
		}
	} else {
		cvarManager->log("unknown message received: " + Message);
	}
}