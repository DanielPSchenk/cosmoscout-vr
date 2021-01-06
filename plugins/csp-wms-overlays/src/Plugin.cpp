////////////////////////////////////////////////////////////////////////////////////////////////////
//                               This file is part of CosmoScout VR                               //
//      and may be used under the terms of the MIT license. See the LICENSE file for details.     //
//                        Copyright: (c) 2019 German Aerospace Center (DLR)                       //
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "Plugin.hpp"
#include "TextureOverlayRenderer.hpp"
#include "WebMapService.hpp"
#include "logger.hpp"

#include "../../../src/cs-core/GuiManager.hpp"
#include "../../../src/cs-core/InputManager.hpp"
#include "../../../src/cs-core/Settings.hpp"
#include "../../../src/cs-core/SolarSystem.hpp"
#include "../../../src/cs-core/TimeControl.hpp"

////////////////////////////////////////////////////////////////////////////////////////////////////

EXPORT_FN cs::core::PluginBase* create() {
  return new csp::wmsoverlays::Plugin;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

EXPORT_FN void destroy(cs::core::PluginBase* pluginBase) {
  delete pluginBase; // NOLINT(cppcoreguidelines-owning-memory)
}

////////////////////////////////////////////////////////////////////////////////////////////////////

namespace csp::wmsoverlays {

////////////////////////////////////////////////////////////////////////////////////////////////////

void from_json(nlohmann::json const& j, Plugin::Settings::Body& o) {
  cs::core::Settings::deserialize(j, "activeServer", o.mActiveServer);
  cs::core::Settings::deserialize(j, "activeLayer", o.mActiveLayer);
  cs::core::Settings::deserialize(j, "wms", o.mWms);
}

void to_json(nlohmann::json& j, Plugin::Settings::Body const& o) {
  cs::core::Settings::serialize(j, "activeServer", o.mActiveServer);
  cs::core::Settings::serialize(j, "activeLayer", o.mActiveLayer);
  cs::core::Settings::serialize(j, "wms", o.mWms);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void from_json(nlohmann::json const& j, Plugin::Settings& o) {
  cs::core::Settings::deserialize(j, "preFetch", o.mPrefetchCount);
  cs::core::Settings::deserialize(j, "mapCache", o.mMapCache);
  cs::core::Settings::deserialize(j, "bodies", o.mBodies);
}

void to_json(nlohmann::json& j, Plugin::Settings const& o) {
  cs::core::Settings::serialize(j, "preFetch", o.mPrefetchCount);
  cs::core::Settings::serialize(j, "mapCache", o.mMapCache);
  cs::core::Settings::serialize(j, "bodies", o.mBodies);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void Plugin::init() {
  logger().info("Loading plugin...");

  mOnLoadConnection = mAllSettings->onLoad().connect([this]() { onLoad(); });

  mOnSaveConnection = mAllSettings->onSave().connect(
      [this]() { mAllSettings->mPlugins["csp-wms-overlays"] = *mPluginSettings; });

  mGuiManager->addPluginTabToSideBarFromHTML(
      "WMS", "panorama", "../share/resources/gui/wms_overlays_tab.html");
  mGuiManager->addSettingsSectionToSideBarFromHTML(
      "WMS", "panorama", "../share/resources/gui/wms_settings.html");
  mGuiManager->addScriptToGuiFromJS("../share/resources/gui/js/csp-wms-overlays.js");

  // Updates the bounds for which map data is requested.
  mGuiManager->getGui()->registerCallback(
      "wmsOverlays.updateBounds", "Updates the bounds for map requests.", std::function([this]() {
        auto overlay = mWMSOverlays.find(mSolarSystem->pActiveBody.get()->getCenterName());
        if (overlay != mWMSOverlays.end()) {
          overlay->second->requestUpdateBounds();
        }
      }));

  // Set whether to interpolate textures between timesteps (does not work when pre-fetch is
  // inactive).
  mGuiManager->getGui()->registerCallback("wmsOverlays.setEnableTimeInterpolation",
      "Enables or disables interpolation.",
      std::function([this](bool enable) { mPluginSettings->mEnableInterpolation = enable; }));

  // Set whether to display timespan.
  mGuiManager->getGui()->registerCallback("wmsOverlays.setEnableTimeSpan",
      "Enables or disables timespan.",
      std::function([this](bool enable) { mPluginSettings->mEnableTimespan = enable; }));

  // Set WMS source.
  mGuiManager->getGui()->registerCallback("wmsOverlays.setServer",
      "Set the current planet's WMS server to the one with the given name.",
      std::function([this](std::string&& name) {
        auto overlay = mWMSOverlays.find(mSolarSystem->pActiveBody.get()->getCenterName());
        if (overlay != mWMSOverlays.end()) {
          setWMSServer(overlay->second, name);
        }
      }));

  mGuiManager->getGui()->registerCallback("wmsOverlays.setLayer",
      "Set the current planet's WMS layer to the one with the given name.",
      std::function([this](std::string&& name) {
        auto overlay = mWMSOverlays.find(mSolarSystem->pActiveBody.get()->getCenterName());
        if (overlay != mWMSOverlays.end()) {
          setWMSLayer(overlay->second, name);
        }
      }));

  mActiveBodyConnection = mSolarSystem->pActiveBody.connectAndTouch(
      [this](std::shared_ptr<cs::scene::CelestialBody> const& body) {
        if (!body) {
          return;
        }

        auto overlay = mWMSOverlays.find(body->getCenterName());

        mGuiManager->getGui()->callJavascript(
            "CosmoScout.sidebar.setTabEnabled", "WMS", overlay != mWMSOverlays.end());

        if (overlay == mWMSOverlays.end()) {
          return;
        }

        mGuiManager->getGui()->callJavascript(
            "CosmoScout.gui.clearDropdown", "wmsOverlays.setServer");
        mGuiManager->getGui()->callJavascript(
            "CosmoScout.gui.addDropdownValue", "wmsOverlays.setServer", "None", "None", "false");

        setWMSServer(overlay->second, "None");

        auto const& settings = getBodySettings(overlay->second);
        for (auto const& server : mWms) {
          bool active = server.getTitle() == settings.mActiveServer.get();
          mGuiManager->getGui()->callJavascript("CosmoScout.gui.addDropdownValue",
              "wmsOverlays.setServer", server.getTitle(), server.getTitle(), active);

          if (active) {
            setWMSServer(overlay->second, server.getTitle());
          }
        }
      });

  onLoad();

  logger().info("Loading done.");
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void Plugin::deInit() {
  logger().info("Unloading plugin...");

  mSolarSystem->pActiveBody.disconnect(mActiveBodyConnection);

  mGuiManager->removePluginTab("WMS");
  mGuiManager->removeSettingsSection("WMS");

  mGuiManager->getGui()->callJavascript(
      "CosmoScout.gui.unregisterCss", "css/csp-simple-wms-bodies.css");

  mGuiManager->getGui()->unregisterCallback("wmsOverlays.setEnableTimeInterpolation");
  mGuiManager->getGui()->unregisterCallback("wmsOverlays.setEnableTimeSpan");
  mGuiManager->getGui()->unregisterCallback("wmsOverlays.setServer");
  mGuiManager->getGui()->unregisterCallback("wmsOverlays.setLayer");

  mAllSettings->onLoad().disconnect(mOnLoadConnection);
  mAllSettings->onSave().disconnect(mOnSaveConnection);

  logger().info("Unloading done.");
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void Plugin::onLoad() {
  // Read settings from JSON.
  from_json(mAllSettings->mPlugins.at("csp-wms-overlays"), *mPluginSettings);

  // First try to re-configure existing WMS overlays. We assume that they are similar if they
  // have the same name in the settings (which means they are attached to an anchor with the same
  // name).
  auto wmsOverlay = mWMSOverlays.begin();
  while (wmsOverlay != mWMSOverlays.end()) {
    auto settings = mPluginSettings->mBodies.find(wmsOverlay->first);
    if (settings != mPluginSettings->mBodies.end()) {
      // If there are settings for this simpleWMSBody, reconfigure it.
      wmsOverlay->second->configure(settings->second);

      setWMSServer(wmsOverlay->second, settings->second.mActiveServer.get());

      ++wmsOverlay;
    } else {
      // Else delete it.
      wmsOverlay = mWMSOverlays.erase(wmsOverlay);
    }
  }

  // Then add new WMS overlays.
  for (auto const& settings : mPluginSettings->mBodies) {
    if (mWMSOverlays.find(settings.first) != mWMSOverlays.end()) {
      continue;
    }

    auto anchor = mAllSettings->mAnchors.find(settings.first);

    if (anchor == mAllSettings->mAnchors.end()) {
      throw std::runtime_error(
          "There is no Anchor \"" + settings.first + "\" defined in the settings.");
    }

    auto wmsOverlay = std::make_shared<TextureOverlayRenderer>(
        settings.first, mSolarSystem, mTimeControl, mPluginSettings);

    mWMSOverlays.emplace(settings.first, wmsOverlay);

    for (auto const& wmsUrl : settings.second.mWms) {
      try {
        mWms.emplace_back(wmsUrl);
      } catch (std::exception const& e) {
        logger().warn("Failed to parse capabilities for '{}': {}", wmsUrl, e.what());
      }
    }

    setWMSServer(wmsOverlay, settings.second.mActiveServer.get());
    wmsOverlay->configure(settings.second);
  }

  mSolarSystem->pActiveBody.touch(mActiveBodyConnection);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

Plugin::Settings::Body& Plugin::getBodySettings(
    std::shared_ptr<TextureOverlayRenderer> const& wmsOverlay) const {
  auto name = std::find_if(mWMSOverlays.begin(), mWMSOverlays.end(),
      [&](auto const& pair) { return pair.second == wmsOverlay; });
  return mPluginSettings->mBodies.at(name->first);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void Plugin::setWMSServer(
    std::shared_ptr<TextureOverlayRenderer> const& wmsOverlay, std::string const& name) const {
  mGuiManager->getGui()->callJavascript("CosmoScout.wmsOverlays.resetLayerSelect");
  mGuiManager->getGui()->callJavascript("CosmoScout.gui.clearDropdown", "wmsOverlays.setLayer");
  mGuiManager->getGui()->callJavascript(
      "CosmoScout.gui.addDropdownValue", "wmsOverlays.setLayer", "None", "None", false);

  auto&       settings = getBodySettings(wmsOverlay);
  auto const& server   = std::find_if(
      mWms.begin(), mWms.end(), [&name](WebMapService wms) { return wms.getTitle() == name; });

  setWMSLayerNone(wmsOverlay);
  if (server == mWms.end()) {
    logger().trace("No server with name '{}' found", name);
    settings.mActiveServer = "None";
    mGuiManager->getGui()->callJavascript(
        "CosmoScout.gui.setDropdownValue", "wmsOverlays.setServer", "None", false);
    return;
  }
  settings.mActiveServer = name;

  for (auto const& layer : server->getLayers()) {
    bool active = layer.getName() == settings.mActiveLayer.get();
    mGuiManager->getGui()->callJavascript("CosmoScout.gui.addDropdownValue", "wmsOverlays.setLayer",
        layer.getName(), layer.getTitle(), active);

    if (active) {
      setWMSLayer(wmsOverlay, *server, layer.getName());
    }
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void Plugin::setWMSLayer(
    std::shared_ptr<TextureOverlayRenderer> const& wmsOverlay, std::string const& name) const {
  auto&       settings = getBodySettings(wmsOverlay);
  auto const& server   = std::find_if(mWms.begin(), mWms.end(),
      [&settings](WebMapService wms) { return wms.getTitle() == settings.mActiveServer.get(); });

  if (server == mWms.end()) {
    setWMSLayerNone(wmsOverlay);
    return;
  }
  setWMSLayer(wmsOverlay, *server, name);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void Plugin::setWMSLayer(std::shared_ptr<TextureOverlayRenderer> const& wmsOverlay,
    WebMapService const& server, std::string const& name) const {
  auto&                      settings = getBodySettings(wmsOverlay);
  std::optional<WebMapLayer> layer    = server.getLayer(name);

  if (!layer.has_value()) {
    logger().trace("No layer with name '{}' found", name);
    setWMSLayerNone(wmsOverlay);
    return;
  }
  settings.mActiveLayer = name;

  wmsOverlay->setActiveWMS(
      std::make_shared<WebMapService>(server), std::make_shared<WebMapLayer>(layer.value()));
  mGuiManager->getGui()->callJavascript(
      "CosmoScout.wmsOverlays.setWMSDataCopyright", layer->getSettings().mAttribution.value_or(""));
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void Plugin::setWMSLayerNone(std::shared_ptr<TextureOverlayRenderer> const& wmsOverlay) const {
  auto& settings        = getBodySettings(wmsOverlay);
  settings.mActiveLayer = "None";
  wmsOverlay->setActiveWMS(nullptr, nullptr);
  mGuiManager->getGui()->callJavascript(
      "CosmoScout.gui.setDropdownValue", "wmsOverlays.setLayer", "None", false);
  mGuiManager->getGui()->callJavascript("CosmoScout.wmsOverlays.setWMSDataCopyright", "");
}

////////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace csp::wmsoverlays
