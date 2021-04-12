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

	cvarManager->registerNotifier("toofastteams_isonline", [this](...) {
		cvarManager->log(std::to_string(gameWrapper->IsInOnlineGame()));
		}, "checks if online match", PERMISSION_ALL);

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

	gameWrapper->HookEventPost("Function TAGame.Car_TA.SetVehicleInput",
		[this](...) { onTick(); });
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

void TooFastTeams::onTick() {
	auto sw = GetCurrentGameState();

	if (!sw) return;

	auto teams = sw.GetTeams();

	if (teams.IsNull()) return;

	if (teams.Count() != 2) {
		cvarManager->log("weird number of teams, speak to developer: " + std::to_string(teams.Count()));
		return;
	}

	auto bluePRIs = teams.Get(0).GetMembers();
	auto orangePRIs = teams.Get(1).GetMembers();

	for (PriWrapper pri : bluePRIs) {
		if (!pri) {
			continue;
		}

		CarWrapper car = pri.GetCar();

		if (!car) {
			continue;
		}

		// max speed is reset when demoed, so ensures it's always valid
		car.SetMaxLinearSpeed2(blueMaxSpeed);

		auto controller = car.GetPlayerController();

		auto input = controller.GetVehicleInput();
		//cvarManager->log("Throttle : " + std::to_string(input.Throttle));

		if (input.Throttle == 0) {
			//cvarManager->log("player is not throttling");
			return;
		}

		Vector currentVelocity = car.GetVelocity();
		float currSpeed = car.GetForwardSpeed();

		if (!car.IsOnGround()) {
			return;
		}

		if (blueSpeedMultiplier < 0) {
			return;
		}

		if (std::abs(currSpeed) >= blueSpeedThreshold) {
			return;
		}

		//cvarManager->log(std::to_string(input.Steer));

		if (std::abs(input.Steer) > 0.5) {
			return;
		}

		if ((currSpeed <= 0 && input.Throttle < 0) || (currSpeed >= 0 && input.Throttle > 0)) {
			//cvarManager->log("Curr speed :" + std::to_string(currSpeed));
			currentVelocity *= blueSpeedMultiplier;
			car.SetVelocity(currentVelocity);
		}
	}

	for (PriWrapper pri : orangePRIs) {
		if (!pri) {
			continue;
		}

		CarWrapper car = pri.GetCar();

		if (!car) {
			continue;
		}

		// max speed is reset when demoed, so ensures it's always valid
		car.SetMaxLinearSpeed2(orangeMaxSpeed);

		auto controller = car.GetPlayerController();

		auto input = controller.GetVehicleInput();
		//cvarManager->log("Throttle : " + std::to_string(input.Throttle));

		if (input.Throttle == 0) {
			//cvarManager->log("player is not throttling");
			return;
		}

		Vector currentVelocity = car.GetVelocity();
		float currSpeed = car.GetForwardSpeed();

		if (!car.IsOnGround()) {
			return;
		}

		if (orangeSpeedMultiplier < 0) {
			return;
		}

		if (std::abs(currSpeed) >= orangeSpeedThreshold) {
			return;
		}

		//cvarManager->log(std::to_string(input.Steer));

		if (std::abs(input.Steer) > 0.5) {
			return;
		}

		if ((currSpeed <= 0 && input.Throttle < 0) || (currSpeed >= 0 && input.Throttle > 0)) {
			//cvarManager->log("Curr speed :" + std::to_string(currSpeed));
			currentVelocity *= orangeSpeedMultiplier;
			car.SetVelocity(currentVelocity);
		}
	}

}