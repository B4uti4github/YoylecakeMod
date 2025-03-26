#include <Geode/modify/GJBaseGameLayer.hpp>
#include <algorithm>

static const std::set<int> sawblades = {88, 89, 98, 183, 184, 185, 186, 187, 188, 397, 398, 399, 678, 679, 680, 740, 741, 742, 1619, 1620, 1701, 1702, 1703, 1705, 1706, 1707, 1708, 1709, 1710, 1734, 1735, 1736};

using namespace geode::prelude;

CCSprite* jesus_christ = nullptr;

float time_counter = 0.0;
float last_jesus_time = -1000.0;

bool isValidImage = false;
bool skipSolidObjects = false;
bool skipInvisibleObjects = false;
bool enabled = false;
bool playLayer = false;
bool levelEditorLayer = false;

int64_t volume = 0;

std::string customSound = "";
std::string customImage = "";

float sensitivity = 2.0f;

bool imageExists = false;
bool soundExists = false;

bool getBoolSetting(const std::string_view key) {
	return Mod::get()->getSettingValue<bool>(key);
}
std::filesystem::path getFileSetting(const std::string_view key) {
	return Mod::get()->getSettingValue<std::filesystem::path>(key);
}
std::string getFileSettingAsString(const std::string_view key) {
	return getFileSetting(key).string();
}
int64_t getIntSetting(const std::string_view key) {
	return Mod::get()->getSettingValue<int64_t>(key);
}
double getDoubleSetting(const std::string_view key) {
	return Mod::get()->getSettingValue<double>(key);
}
bool modEnabled() {
	return enabled;
}
bool isValidSprite(CCNode* obj) {
	return obj && !obj->getUserObject("geode.texture-loader/fallback");
}
bool playLayerEnabled() {
	GJBaseGameLayer* gjbgl = GJBaseGameLayer::get();
	PlayLayer* pl = PlayLayer::get();
	if (!gjbgl || !pl) return false;
	return playLayer && gjbgl == pl;
}
bool levelEditorLayerEnabled() {
	GJBaseGameLayer* gjbgl = GJBaseGameLayer::get();
	LevelEditorLayer* lel = LevelEditorLayer::get();
	if (!gjbgl || !lel) return false;
	return levelEditorLayer && gjbgl == lel;
}
void resetJesus() {
	time_counter = 0.0;
	last_jesus_time = -1000.0;
}

class $modify(MyGJBaseGameLayer, GJBaseGameLayer) {
	void jesus() {
		if (!modEnabled() || (!playLayerEnabled() && !levelEditorLayerEnabled())) return;
		
		auto scene = CCDirector::get()->getRunningScene();

		// A section of this code was copied from https://github.com/NicknameGG/robtop-jumpscare --iliashdz
		log::info("customImage: {}", customImage);
		log::info("isValidImage: {}", isValidImage);
		log::info("imageExists: {}", imageExists);
		if (!scene->getChildByIDRecursive("jesus"_spr)) {
			if (isValidImage && customImage != "Please choose an image file." && imageExists) {
				jesus_christ = CCSprite::create(customImage.c_str());
				log::info("created jesus from {}", customImage);
			}
			else jesus_christ = CCSprite::create("jesus.png"_spr);
			jesus_christ->setID("jesus"_spr);
			CCSize winSize = CCDirector::get()->getWinSize();

			float ratio_x = winSize.width / jesus_christ->getContentSize().width;
			float ratio_y = winSize.height / jesus_christ->getContentSize().height;
			float ratio = std::max(ratio_x, ratio_y);

			jesus_christ->setScaleX(ratio);
			jesus_christ->setScaleY(ratio);

			jesus_christ->setPosition({ winSize.width / 2, winSize.height / 2 });
			scene->addChild(jesus_christ, 100);
			jesus_christ->setOpacity(0);
		}

		if ((time_counter < 1.5) || (time_counter - last_jesus_time < 0.2)) return;
		last_jesus_time = time_counter;

		auto system = FMODAudioEngine::get()->m_system;
		FMOD::Channel* channel;
		FMOD::Sound* sound;
		if (customSound != "Please choose an audio file." && soundExists) system->createSound(customSound.c_str(), FMOD_DEFAULT, nullptr, &sound);
		else system->createSound((Mod::get()->getResourcesDir() / "bell.ogg").string().c_str(), FMOD_DEFAULT, nullptr, &sound);
		system->playSound(sound, nullptr, false, &channel);
		channel->setVolume(volume / 100.0f);

		if (jesus_christ->getActionByTag(1)) jesus_christ->stopActionByTag(1);

		jesus_christ->setOpacity(255);
		jesus_christ->runAction(CCFadeOut::create(1.0))->setTag(1);
	}

	void update(float dt) {
		GJBaseGameLayer::update(dt);
		if (!modEnabled() || (!playLayerEnabled() && !levelEditorLayerEnabled())) return;
		time_counter += dt;
	}

	void collisionCheckObjects(PlayerObject* player, gd::vector<GameObject*>* objs, int apparentlyNeededForTheForLoopToAvoidCrashing, float v1) {
		GJBaseGameLayer::collisionCheckObjects(player, objs, apparentlyNeededForTheForLoopToAvoidCrashing, v1);
		if (!modEnabled() || (!playLayerEnabled() && !levelEditorLayerEnabled())) return;
		for (int i = 0; i < apparentlyNeededForTheForLoopToAvoidCrashing; i++) {
			GameObject* obj = objs->at(i);
			if (!obj || obj->m_isGroupDisabled) continue;
			if (obj->m_objectType != GameObjectType::Hazard && obj->m_objectType != GameObjectType::AnimatedHazard && obj->m_objectType != GameObjectType::Solid) continue;
			if (obj->m_objectType == GameObjectType::Solid && skipSolidObjects) continue;
			if (skipInvisibleObjects && (obj->m_isHide || obj->getOpacity() < 1)) continue;
			const bool isSawblade = std::binary_search(sawblades.begin(), sawblades.end(), obj->m_objectID);
			const float multiplier = isSawblade ? -2.5f : 2.f; // reduce sawblade hitboxes since the CCRect does not accurately represent the hitbox
			CCRect sensitivityRect = CCRect(obj->getObjectRect().origin - CCPoint(sensitivity, sensitivity), obj->getObjectRect().size + CCPoint(sensitivity * multiplier, sensitivity * multiplier));
			if (player->getObjectRect().intersectsRect(sensitivityRect)) jesus();
		}
	}

	void resetLevelVariables() {
		GJBaseGameLayer::resetLevelVariables();
		resetJesus();
	}
  
	bool init() {
		if (!GJBaseGameLayer::init()) return false;
		if (!modEnabled() || (!playLayerEnabled() && !levelEditorLayerEnabled())) return true;

		resetJesus();
		CCSprite* sprite = CCSprite::create(customImage.c_str());
		isValidImage = sprite;
		log::info("isValidImage: {}", isValidImage);
		// code adapted from https://github.com/geode-sdk/DevTools/tree/main/src/pages/Attributes.cpp#L152 --raydeeux
		if (isValidImage) isValidImage = isValidSprite(sprite);
		log::info("isValidImage: {}", isValidImage);

		return true;
	}
};

$on_mod(Loaded) {
	skipSolidObjects = getBoolSetting("skipSolidObjects");
	skipInvisibleObjects = getBoolSetting("skipInvisibleObjects");
	enabled = getBoolSetting("enabled");
	playLayer = getBoolSetting("playLayer");
	levelEditorLayer = getBoolSetting("levelEditorLayer");
	volume = getIntSetting("volume");
	customSound = getFileSettingAsString("customSound");
	soundExists = std::filesystem::exists(customSound);
	customImage = getFileSettingAsString("customImage");
	imageExists = std::filesystem::exists(customImage);
	sensitivity = getDoubleSetting("sensitivity");

	listenForAllSettingChanges([](std::shared_ptr<SettingV3> setting){
		skipSolidObjects = getBoolSetting("skipSolidObjects");
		skipInvisibleObjects = getBoolSetting("skipInvisibleObjects");
		enabled = getBoolSetting("enabled");
		playLayer = getBoolSetting("playLayer");
		levelEditorLayer = getBoolSetting("levelEditorLayer");
		volume = getIntSetting("volume");
		customSound = getFileSettingAsString("customSound");
		soundExists = std::filesystem::exists(customSound);
		if (!soundExists && customSound != "Please choose an audio file.") {
			FLAlertLayer::create("Hey there!", fmt::format("<cl>{}</c> does not exist!\n\n<cy>Please choose something else instead.</c>", customSound), "OK")->show();
			customSound = "Please choose an audio file.";
		}
		customImage = getFileSettingAsString("customImage");
		imageExists = std::filesystem::exists(customImage);
		if (!imageExists && customImage != "Please choose an image file.") {
			FLAlertLayer::create("Hey there!", fmt::format("<cl>{}</c> does not exist!\n\n<cy>Please choose something else instead.</c>", customImage), "OK")->show();
			customImage = "Please choose an image file.";
		}
		sensitivity = getDoubleSetting("sensitivity");
	});

	listenForSettingChanges("sensitivity", [](double sensitivity) {
		if (!Mod::get()->setSavedValue<bool>("shownSensitivityWarning", true)) FLAlertLayer::create("Hey there!", "Sensitivity settings are not 100% accurate with sawblades, and probably won't ever be in the future.\n<cl>Thanks for understanding! :)</c>", "I understand")->show();
	});
}