#include <Geode/modify/GJBaseGameLayer.hpp>

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
		if (!scene->getChildByIDRecursive("jesus"_spr)) {
			if (isValidImage && customImage != "Please choose an image file.")
				jesus_christ = CCSprite::create(customImage.c_str());
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
		if (customSound != "Please choose an audio file.") system->createSound(customSound.c_str(), FMOD_DEFAULT, nullptr, &sound);
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
			if (obj->m_objectType == GameObjectType::Solid && !skipSolidObjects) continue;
			if (skipInvisibleObjects && (obj->m_isHide || obj->getOpacity() == 0)) continue;
			CCRect sensitivityRect = CCRect(obj->getObjectRect().origin - CCPoint(sensitivity, sensitivity), obj->getObjectRect().size + CCPoint(sensitivity * 2, sensitivity * 2));
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
		// code adapted from https://github.com/geode-sdk/DevTools/tree/main/src/pages/Attributes.cpp#L152 --raydeeux
		// dank, your `CCTextureCache` doesnt work without a game restart so i had to yoink textureloader code --raydeeux
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
	customImage = getFileSettingAsString("customImage");
	sensitivity = getDoubleSetting("sensitivity");
	listenForAllSettingChanges([](std::shared_ptr<SettingV3> setting){
		skipSolidObjects = getBoolSetting("skipSolidObjects");
		skipInvisibleObjects = getBoolSetting("skipInvisibleObjects");
		enabled = getBoolSetting("enabled");
		playLayer = getBoolSetting("playLayer");
		levelEditorLayer = getBoolSetting("levelEditorLayer");
		volume = getIntSetting("volume");
		customSound = getFileSettingAsString("customSound");
		customImage = getFileSettingAsString("customImage");
		sensitivity = getDoubleSetting("sensitivity");
	});
}