#include "pch.h"
#include "TooFastTeams.h"
#include "../NetcodeManager/NetcodeManager.h"


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

	Netcode = std::make_shared<NetcodeManager>(cvarManager, gameWrapper, exports,
		[this](const std::string& Message, PriWrapper Sender) { OnMessageReceived(Message, Sender); });

	cvarManager->registerCvar("toofastteams_max_blue", "2300", "sets blue max speed", true, true, 0)
		.addOnValueChanged([this](std::string cvarName, CVarWrapper cvar) {
			if (gameWrapper->IsInOnlineGame()) { return; }

			Netcode->SendMessageB("maxb" + cvar.getStringValue());
		});

	cvarManager->registerCvar("toofastteams_max_orange", "2300", "sets orange max speed", true, true, 0)
		.addOnValueChanged([this](std::string cvarName, CVarWrapper cvar) {
			if (gameWrapper->IsInOnlineGame()) { return; }

			Netcode->SendMessageB("maxo" + cvar.getStringValue());});

	cvarManager->registerCvar("toofastteams_mult_blue", "1.01", "sets blue speed multiplier", true, true, -1, true, 2)
		.addOnValueChanged([this](std::string cvarName, CVarWrapper cvar) {
			if (gameWrapper->IsInOnlineGame()) { return; }

			Netcode->SendMessageB("multb" + cvar.getStringValue());});

	cvarManager->registerCvar("toofastteams_mult_orange", "1.01", "sets orange speed multiplier", true, true, -1, true, 2)
		.addOnValueChanged([this](std::string cvarName, CVarWrapper cvar) {
			if (gameWrapper->IsInOnlineGame()) { return; }

			Netcode->SendMessageB("multo" + cvar.getStringValue());});

	cvarManager->registerCvar("toofastteams_threshold_blue", "2300", "sets blue speed threshold", true, true, 0)
		.addOnValueChanged([this](std::string cvarName, CVarWrapper cvar) {
			if (gameWrapper->IsInOnlineGame()) { return; }

			Netcode->SendMessageB("threshb" + cvar.getStringValue());});

	cvarManager->registerCvar("toofastteams_threshold_orange", "2300", "sets orange speed threshold", true, true, 0)
		.addOnValueChanged([this](std::string cvarName, CVarWrapper cvar) {
			if (gameWrapper->IsInOnlineGame()) { return; }

			Netcode->SendMessageB("thresho" + cvar.getStringValue());});

	cvarManager->registerNotifier("toofastteams_isonline", [this](...) {
		cvarManager->log(std::to_string(gameWrapper->IsInOnlineGame()));
		}, "checks if online match", PERMISSION_ALL);

	/*gameWrapper->RegisterDrawable([this](CanvasWrapper canvas) { Render(canvas); });*/

	gameWrapper->HookEventPost("Function TAGame.GameEvent_TA.AddCar",
		[this](...) { 
			if (gameWrapper->IsInOnlineGame()) { return; }
		
			Netcode->SendMessageB("maxb" + cvarManager->getCvar("toofastteams_max_blue").getStringValue());
			Netcode->SendMessageB("maxo" + cvarManager->getCvar("toofastteams_max_orange").getStringValue());
			Netcode->SendMessageB("multb" + cvarManager->getCvar("toofastteams_mult_blue").getStringValue());
			Netcode->SendMessageB("multo" + cvarManager->getCvar("toofastteams_mult_orange").getStringValue());
			Netcode->SendMessageB("threshb" + cvarManager->getCvar("toofastteams_threshold_blue").getStringValue());
			Netcode->SendMessageB("thresho" + cvarManager->getCvar("toofastteams_threshold_orange").getStringValue());

			cvarManager->log("sent messages");
		});

	gameWrapper->HookEventPost("Function TAGame.Car_TA.SetVehicleInput",
		[this](...) { onTick(); });

	GenerateSettingsFile();
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

	cvarManager->log(Message);

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
		} catch (...) {
			cvarManager->log("error in receving new speed mult (invalid number) on message: " + Message);
			return;
		}

		if (teamString == "b") {
			blueSpeedMultiplier = speedValue;
		} else if (teamString == "o") {
			orangeSpeedMultiplier = speedValue;
		} else {
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
		} else if (teamString == "o") {
			orangeSpeedThreshold = speedValue;
		} else {
			cvarManager->log("error in receving new speed thresh (invalid team) on message: " + Message);
		}
	} else {
		cvarManager->log("unknown message received: " + Message);
	}
}


void TooFastTeams::onTick() {
	auto sw = GetCurrentGameState();

	if (!sw) return;

	auto cars = sw.GetCars();

	if (cars.IsNull()) return;

	std::vector<CarWrapper> BlueCars;
	std::vector<CarWrapper> OrangeCars;

	ArrayWrapper<CarWrapper> Cars = sw.GetCars();
	for (int i = 0; i < Cars.Count(); ++i)
	{
		CarWrapper Car = Cars.Get(i);
		if (Car.IsNull()) { continue; }

		if (Car.GetTeamNum2() == 0) { BlueCars.push_back(Car); }
		if (Car.GetTeamNum2() == 1) { OrangeCars.push_back(Car); }
	}

	for (CarWrapper car : BlueCars) {

		if (car.IsNull()) { continue; }

		// max speed is reset when demoed, so ensures it's always valid
		car.SetMaxLinearSpeed2(blueMaxSpeed);

		auto controller = car.GetPlayerController();

		auto input = controller.GetVehicleInput();
		//cvarManager->log("Throttle : " + std::to_string(input.Throttle));

		if (input.Throttle == 0) {
			//cvarManager->log("player is not throttling");
			continue;
		}

		Vector currentVelocity = car.GetVelocity();
		float currSpeed = car.GetForwardSpeed();

		if (!car.IsOnGround()) {
			continue;
		}

		if (blueSpeedMultiplier < 0) {
			continue;
		}

		if (std::abs(currSpeed) >= blueSpeedThreshold) {
			continue;
		}

		//cvarManager->log(std::to_string(input.Steer));

		if (std::abs(input.Steer) > 0.5) {
			continue;
		}

		if ((currSpeed <= 0 && input.Throttle < 0) || (currSpeed >= 0 && input.Throttle > 0)) {
			//cvarManager->log("Curr speed :" + std::to_string(currSpeed));
			currentVelocity *= blueSpeedMultiplier;
			car.SetVelocity(currentVelocity);
		}
	}

	for (CarWrapper car : OrangeCars) {
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
			continue;
		}

		Vector currentVelocity = car.GetVelocity();
		float currSpeed = car.GetForwardSpeed();

		if (!car.IsOnGround()) {
			continue;
		}

		if (orangeSpeedMultiplier < 0) {
			continue;
		}

		if (std::abs(currSpeed) >= orangeSpeedThreshold) {
			continue;
		}

		//cvarManager->log(std::to_string(input.Steer));

		if (std::abs(input.Steer) > 0.5) {
			continue;
		}

		if ((currSpeed <= 0 && input.Throttle < 0) || (currSpeed >= 0 && input.Throttle > 0)) {
			//cvarManager->log("Curr speed :" + std::to_string(currSpeed));
			currentVelocity *= orangeSpeedMultiplier;
			car.SetVelocity(currentVelocity);
		}
	}
}

void TooFastTeams::Render(CanvasWrapper Canvas) {
	std::vector<std::string> DebugStrings;
	DebugStrings.push_back("git gud");

	DebugStrings.push_back("blueSpeedMultiplier: " + std::to_string(blueSpeedMultiplier)); 
	DebugStrings.push_back("blueSpeedThreshold: " + std::to_string(blueSpeedThreshold)); 
	DebugStrings.push_back("blueMaxSpeed: " + std::to_string(blueMaxSpeed)); 
	DebugStrings.push_back("orangeSpeedMultiplier: " + std::to_string(orangeSpeedMultiplier)); 
	DebugStrings.push_back("orangeSpeedThreshold: " + std::to_string(orangeSpeedThreshold)); 
	DebugStrings.push_back("orangeMaxSpeed: " + std::to_string(orangeMaxSpeed)); 

	Vector2 BasePos = { 50, 50 };
	Canvas.SetColor(LinearColor{ 0, 255, 0, 255 });
	for (const auto& Whatever : DebugStrings)
	{
		Canvas.SetPosition(BasePos);
		Canvas.DrawString(Whatever);
		BasePos.Y += 20;
	}
}

void TooFastTeams::GenerateSettingsFile()
{
	std::ofstream SettingsFile(gameWrapper->GetBakkesModPath() / "plugins" / "settings" / "toofastteams.set");

	nl("TooFast Teams");
	nl("9|Makes teams way faster");
	nl("8|");
	nl("9|Speed multiplier applied each tick.Effectively is an acceleration force. Numbers below 1.005 will actually be slower than normal gameplay");
	nl("4|Blue speed multiplier|toofastteams_max_blue|1.005|1.1");
	nl("4|Orange speed multiplier|toofastteams_max_orange|1.005|1.1");
	nl("9|Maximum speed(forward or backward) where the plugin will accelerate you");
	nl("9|If set low, will just help you hit supersonic faster");
	nl("9|If set high, will make you insanely mobile");
	nl("9|If jumping isn't working for either team, hit this button");
	nl("7|");
	nl("0|Reset acceleration|toofastteams_threshold_blue -1; toofastteams_threshold_orange -1");
	nl("4|Blue acceleration max threshold|toofastteams_threshold_blue|0|23000");
	nl("4|Orange acceleration max threshold|toofastteams_threshold_orange|0|23000");
	nl("9|Increase your maximum car speed to go beyond supersonic(default rocket league value is 2300)");
	nl("4|Blue max car speed|toofastteams_max_blue|2300|23000");
	nl("4|Orange max car speed|toofastteams_max_orange|2300|23000");
	nl("8|");
	nl("9|Plugin commissioned by Striped");
	nl("9|youtube.com/c/Striped");
	nl("9|Plugin made by JerryTheBee#1117 DM me on discord for custom plugins");
	blank;

	SettingsFile.close();
	cvarManager->executeCommand("cl_settings_refreshplugins");
}