#include <obs-module.h>
#include <obs-frontend-api.h>
#include <util/platform.h>

#include <QMainWindow>
#include <QString>

#include "look-filter.hpp"
#include "look-studio-dock.hpp"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("ld-look-studio", "en-US")

MODULE_EXPORT const char *obs_module_description(void)
{
	return "LD Look Studio - Creative Look & Film Grade Filter with DaVinci-style Stills Gallery, Melanin Skin Science & Comparison Wipe";
}

MODULE_EXPORT const char *obs_module_name(void)
{
	return "LD Look Studio";
}

static LookStudioDock *g_lookDock = nullptr;

static void frontend_event_callback(enum obs_frontend_event event, void *private_data)
{
	Q_UNUSED(private_data);
	if (!g_lookDock)
		return;

	if (event == OBS_FRONTEND_EVENT_SCENE_CHANGED || event == OBS_FRONTEND_EVENT_FINISHED_LOADING ||
	    event == OBS_FRONTEND_EVENT_STUDIO_MODE_ENABLED || event == OBS_FRONTEND_EVENT_STUDIO_MODE_DISABLED) {
		g_lookDock->refreshSources();
	}
}

bool obs_module_load(void)
{
	blog(LOG_INFO, "[LD Look Studio] Initializing v1.0.0 (LD Media Church)...");

	// 1. Register Video Filter
	obs_register_source(&ld_look_studio_filter_info);

	// 2. Register Look Studio Dock
	QMainWindow *mainWindow = static_cast<QMainWindow *>(obs_frontend_get_main_window());
	if (mainWindow) {
		g_lookDock = new LookStudioDock(mainWindow);
		obs_frontend_add_dock(g_lookDock);

		char *dataPath = obs_module_file("");
		if (dataPath) {
			g_lookDock->initialize(QString::fromUtf8(dataPath));
			bfree(dataPath);
		}

		obs_frontend_add_event_callback(frontend_event_callback, nullptr);
	}

	blog(LOG_INFO, "[LD Look Studio] Loaded successfully. Color science and stills engine active.");
	return true;
}

void obs_module_unload(void)
{
	obs_frontend_remove_event_callback(frontend_event_callback, nullptr);
	blog(LOG_INFO, "[LD Look Studio] Unloaded.");
}
