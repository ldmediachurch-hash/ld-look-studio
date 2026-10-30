#include "look-filter.hpp"
#include "look-library.hpp"
#include "snapshot-manager.hpp"

#include <obs-frontend-api.h>
#include <graphics/graphics.h>
#include <graphics/half.h>
#include <util/bmem.h>
#include <util/dstr.h>
#include <util/platform.h>

#include <algorithm>
#include <cmath>
#include <mutex>
#include <vector>

#include <QImage>
#include <QMetaObject>

static inline float srgb_to_linear(float u)
{
	return (u <= 0.04045f) ? (u / 12.92f) : powf((u + 0.055f) / 1.055f, 2.4f);
}

static std::mutex s_filterMutex;
static std::vector<LookFilterData *> s_activeFilters;

// Global pointer to the snapshot manager set by dock
static SnapshotManager *s_snapshotManager = nullptr;

void look_filter_set_snapshot_manager(SnapshotManager *mgr)
{
	s_snapshotManager = mgr;
}

static const char *look_filter_get_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return "LD Look Studio";
}

static void free_filter_resources(LookFilterData *filter)
{
	if (filter->clutTexture) {
		gs_voltexture_destroy(filter->clutTexture);
		filter->clutTexture = nullptr;
	}
	if (filter->stageSurface) {
		gs_stagesurface_destroy(filter->stageSurface);
		filter->stageSurface = nullptr;
	}
}

static void load_clut_texture(LookFilterData *filter, const char *path)
{
	obs_enter_graphics();
	free_filter_resources(filter);
	obs_leave_graphics();

	if (!path || *path == '\0') {
		return;
	}

	CubeLut lut;
	if (!CubeParser::loadCubeFile(QString::fromUtf8(path), lut)) {
		blog(LOG_WARNING, "[LD Look Studio] Failed to parse .cube file: %s", path);
		return;
	}

	vec3_set(&filter->domainMin, lut.domainMin[0], lut.domainMin[1], lut.domainMin[2]);
	vec3_set(&filter->domainMax, lut.domainMax[0], lut.domainMax[1], lut.domainMax[2]);

	uint32_t width = lut.size;
	if (width < 2) {
		return;
	}

	// Scale and offset for 3D LUT sampling
	struct vec3 domain_scale;
	vec3_sub(&domain_scale, &filter->domainMax, &filter->domainMin);
	if (domain_scale.x <= 0.0001f)
		domain_scale.x = 1.0f;
	if (domain_scale.y <= 0.0001f)
		domain_scale.y = 1.0f;
	if (domain_scale.z <= 0.0001f)
		domain_scale.z = 1.0f;

	const float width_minus_one = static_cast<float>(width - 1);
	vec3_set(&filter->clutScale, width_minus_one, width_minus_one, width_minus_one);
	vec3_div(&filter->clutScale, &filter->clutScale, &domain_scale);

	vec3_neg(&filter->clutOffset, &filter->domainMin);
	vec3_mul(&filter->clutOffset, &filter->clutOffset, &filter->clutScale);

	vec3_divf(&filter->clutScale, &filter->clutScale, static_cast<float>(width));
	vec3_addf(&filter->clutOffset, &filter->clutOffset, 0.5f);
	vec3_divf(&filter->clutOffset, &filter->clutOffset, static_cast<float>(width));

	obs_enter_graphics();

	const size_t totalVoxels = static_cast<size_t>(width * width * width);
	std::vector<struct half> halfData(totalVoxels * 4);
	for (size_t i = 0; i < totalVoxels; ++i) {
		halfData[i * 4 + 0] = half_from_float(srgb_to_linear(lut.data[i * 4 + 0]));
		halfData[i * 4 + 1] = half_from_float(srgb_to_linear(lut.data[i * 4 + 1]));
		halfData[i * 4 + 2] = half_from_float(srgb_to_linear(lut.data[i * 4 + 2]));
		halfData[i * 4 + 3] = half_from_bits(0x3c00); // 1.0
	}

	const uint8_t *halfPtr = reinterpret_cast<const uint8_t *>(halfData.data());
	filter->clutTexture = gs_voltexture_create(width, width, width, GS_RGBA16F, 1, &halfPtr, 0);

	obs_leave_graphics();
}

static void look_filter_update(void *data, obs_data_t *settings)
{
	LookFilterData *filter = static_cast<LookFilterData *>(data);
	if (!filter)
		return;

	const char *path = obs_data_get_string(settings, "lut_path");
	filter->intensity = static_cast<float>(obs_data_get_double(settings, "intensity"));
	filter->skinProtect = static_cast<float>(obs_data_get_double(settings, "skin_protect"));
	filter->splitMode = static_cast<int>(obs_data_get_int(settings, "split_mode"));
	filter->wipePos = static_cast<float>(obs_data_get_double(settings, "wipe_pos"));
	filter->showDivider = obs_data_get_bool(settings, "show_divider");

	if (!filter->lutPath || strcmp(filter->lutPath, path ? path : "") != 0) {
		bfree(filter->lutPath);
		filter->lutPath = bstrdup(path ? path : "");
		load_clut_texture(filter, filter->lutPath);
	}
}

static void *look_filter_create(obs_data_t *settings, obs_source_t *context)
{
	LookFilterData *filter = static_cast<LookFilterData *>(bzalloc(sizeof(LookFilterData)));
	filter->context = context;
	filter->intensity = 1.0f;
	filter->skinProtect = 0.5f;
	filter->wipePos = 0.5f;
	filter->showDivider = true;

	char *effect_file = obs_module_file("look-studio.effect");
	obs_enter_graphics();
	filter->effect = gs_effect_create_from_file(effect_file, nullptr);
	obs_leave_graphics();
	bfree(effect_file);

	if (!filter->effect) {
		blog(LOG_ERROR, "[LD Look Studio] Could not load look-studio.effect");
	}

	look_filter_update(filter, settings);

	{
		std::lock_guard<std::mutex> lock(s_filterMutex);
		s_activeFilters.push_back(filter);
	}

	return filter;
}

static void look_filter_destroy(void *data)
{
	LookFilterData *filter = static_cast<LookFilterData *>(data);
	if (!filter)
		return;

	{
		std::lock_guard<std::mutex> lock(s_filterMutex);
		auto it = std::find(s_activeFilters.begin(), s_activeFilters.end(), filter);
		if (it != s_activeFilters.end()) {
			s_activeFilters.erase(it);
		}
	}

	obs_enter_graphics();
	free_filter_resources(filter);
	if (filter->effect) {
		gs_effect_destroy(filter->effect);
		filter->effect = nullptr;
	}
	obs_leave_graphics();

	bfree(filter->lutPath);
	bfree(filter);
}

static void look_filter_render(void *data, gs_effect_t *effect)
{
	UNUSED_PARAMETER(effect);
	LookFilterData *filter = static_cast<LookFilterData *>(data);
	if (!filter || !filter->context)
		return;

	obs_source_t *target = obs_filter_get_target(filter->context);
	if (!target || !filter->effect) {
		obs_source_skip_video_filter(filter->context);
		return;
	}

	uint32_t width = obs_source_get_base_width(target);
	uint32_t height = obs_source_get_base_height(target);

	const enum gs_color_space preferred_spaces[] = {
		GS_CS_SRGB,
		GS_CS_SRGB_16F,
		GS_CS_709_EXTENDED,
	};

	const enum gs_color_space source_space = obs_source_get_color_space(
		obs_filter_get_target(filter->context), OBS_COUNTOF(preferred_spaces), preferred_spaces);
	if (source_space == GS_CS_709_EXTENDED) {
		obs_source_skip_video_filter(filter->context);
		return;
	}

	const enum gs_color_format format = gs_get_format_from_space(source_space);
	if (!obs_source_process_filter_begin_with_color_space(filter->context, format, source_space,
							      OBS_ALLOW_DIRECT_RENDERING)) {
		return;
	}

	// Capture snapshot if requested by operator
	if (filter->captureSnapshotPending.exchange(false) && width > 0 && height > 0) {
		gs_stagesurf_t *surf = gs_stagesurface_create(width, height, GS_RGBA);
		if (surf) {
			gs_texture_t *curTarget = gs_get_render_target();
			if (curTarget) {
				gs_stage_texture(surf, curTarget);
				uint8_t *mappedData = nullptr;
				uint32_t linesize = 0;
				if (gs_stagesurface_map(surf, &mappedData, &linesize)) {
					QImage captured(mappedData, static_cast<int>(width), static_cast<int>(height),
							static_cast<int>(linesize), QImage::Format_RGBA8888);
					QImage deepCopy = captured.copy();
					gs_stagesurface_unmap(surf);

					if (s_snapshotManager) {
						QMetaObject::invokeMethod(
							s_snapshotManager,
							[deepCopy]() {
								if (s_snapshotManager) {
									s_snapshotManager->setBaseStill(deepCopy);
								}
							},
							Qt::QueuedConnection);
					}
				}
			}
			gs_stagesurface_destroy(surf);
		}
	}

	if (!filter->clutTexture) {
		// Passthrough if no LUT loaded
		obs_source_process_filter_end(filter->context, filter->effect, width, height);
		return;
	}

	// Set shader uniforms
	gs_eparam_t *param = gs_effect_get_param_by_name(filter->effect, "clut_3d");
	gs_effect_set_texture(param, filter->clutTexture);

	param = gs_effect_get_param_by_name(filter->effect, "intensity");
	gs_effect_set_float(param, filter->intensity);

	param = gs_effect_get_param_by_name(filter->effect, "skin_protect");
	gs_effect_set_float(param, filter->skinProtect);

	param = gs_effect_get_param_by_name(filter->effect, "clut_scale");
	gs_effect_set_vec3(param, &filter->clutScale);

	param = gs_effect_get_param_by_name(filter->effect, "clut_offset");
	gs_effect_set_vec3(param, &filter->clutOffset);

	param = gs_effect_get_param_by_name(filter->effect, "domain_min");
	gs_effect_set_vec3(param, &filter->domainMin);

	param = gs_effect_get_param_by_name(filter->effect, "domain_max");
	gs_effect_set_vec3(param, &filter->domainMax);

	param = gs_effect_get_param_by_name(filter->effect, "split_mode");
	gs_effect_set_int(param, filter->splitMode);

	param = gs_effect_get_param_by_name(filter->effect, "wipe_pos");
	gs_effect_set_float(param, filter->wipePos);

	param = gs_effect_get_param_by_name(filter->effect, "show_divider");
	gs_effect_set_float(param, filter->showDivider ? 1.0f : 0.0f);

	struct vec2 screenSize;
	vec2_set(&screenSize, static_cast<float>(width), static_cast<float>(height));
	param = gs_effect_get_param_by_name(filter->effect, "screen_size");
	gs_effect_set_vec2(param, &screenSize);

	gs_blend_state_push();
	gs_blend_function(GS_BLEND_ONE, GS_BLEND_INVSRCALPHA);

	obs_source_process_filter_tech_end(filter->context, filter->effect, width, height, "Draw");

	gs_blend_state_pop();
}

static obs_properties_t *look_filter_properties(void *data)
{
	obs_properties_t *props = obs_properties_create();

	obs_properties_add_path(props, "lut_path", "3D LUT (.cube)", OBS_PATH_FILE, "3D LUT (*.cube);;All Files (*.*)",
				nullptr);

	obs_properties_add_float_slider(props, "intensity", "Intensity / Mix", 0.0, 1.0, 0.01);
	obs_properties_add_float_slider(props, "skin_protect", "Melanin / Skin Protection", 0.0, 1.0, 0.01);

	obs_property_t *splitProp = obs_properties_add_list(props, "split_mode", "Comparison Wipe", OBS_COMBO_TYPE_LIST,
							    OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(splitProp, "Off (Full Graded)", 0);
	obs_property_list_add_int(splitProp, "Horizontal Split (Left=Orig, Right=Graded)", 1);
	obs_property_list_add_int(splitProp, "Vertical Split (Top=Orig, Bottom=Graded)", 2);

	obs_properties_add_float_slider(props, "wipe_pos", "Wipe Position", 0.0, 1.0, 0.01);
	obs_properties_add_bool(props, "show_divider", "Show Divider Line");

	UNUSED_PARAMETER(data);
	return props;
}

static void look_filter_defaults(obs_data_t *settings)
{
	obs_data_set_default_string(settings, "lut_path", "");
	obs_data_set_default_double(settings, "intensity", 1.0);
	obs_data_set_default_double(settings, "skin_protect", 0.0);
	obs_data_set_default_int(settings, "split_mode", 0);
	obs_data_set_default_double(settings, "wipe_pos", 0.5);
	obs_data_set_default_bool(settings, "show_divider", true);
}

static enum gs_color_space look_filter_get_color_space(void *data, size_t count,
						       const enum gs_color_space *preferred_spaces)
{
	UNUSED_PARAMETER(count);
	UNUSED_PARAMETER(preferred_spaces);

	const enum gs_color_space potential_spaces[] = {
		GS_CS_SRGB,
		GS_CS_SRGB_16F,
		GS_CS_709_EXTENDED,
	};

	LookFilterData *const filter = static_cast<LookFilterData *>(data);
	return obs_source_get_color_space(obs_filter_get_target(filter->context), OBS_COUNTOF(potential_spaces),
					  potential_spaces);
}

static struct obs_source_info init_filter_info()
{
	struct obs_source_info info = {};
	info.id = FILTER_ID;
	info.type = OBS_SOURCE_TYPE_FILTER;
	info.output_flags = OBS_SOURCE_VIDEO;
	info.get_name = look_filter_get_name;
	info.create = look_filter_create;
	info.destroy = look_filter_destroy;
	info.update = look_filter_update;
	info.get_properties = look_filter_properties;
	info.get_defaults = look_filter_defaults;
	info.video_render = look_filter_render;
	info.video_get_color_space = look_filter_get_color_space;
	return info;
}

struct obs_source_info ld_look_studio_filter_info = init_filter_info();

void look_filter_request_snapshot(obs_source_t *source)
{
	std::lock_guard<std::mutex> lock(s_filterMutex);
	for (auto *f : s_activeFilters) {
		if (!source || f->context == source) {
			f->captureSnapshotPending.store(true);
		}
	}
}

void look_filter_apply_look(obs_source_t *source, const char *lutPath)
{
	std::lock_guard<std::mutex> lock(s_filterMutex);
	for (auto *f : s_activeFilters) {
		if (!source || f->context == source) {
			obs_data_t *settings = obs_source_get_settings(f->context);
			obs_data_set_string(settings, "lut_path", lutPath);
			obs_source_update(f->context, settings);
			obs_data_release(settings);
		}
	}
}

void look_filter_set_intensity(obs_source_t *source, float intensity)
{
	std::lock_guard<std::mutex> lock(s_filterMutex);
	for (auto *f : s_activeFilters) {
		if (!source || f->context == source) {
			obs_data_t *settings = obs_source_get_settings(f->context);
			obs_data_set_double(settings, "intensity", static_cast<double>(intensity));
			obs_source_update(f->context, settings);
			obs_data_release(settings);
		}
	}
}

void look_filter_set_skin_protect(obs_source_t *source, float skinProtect)
{
	std::lock_guard<std::mutex> lock(s_filterMutex);
	for (auto *f : s_activeFilters) {
		if (!source || f->context == source) {
			obs_data_t *settings = obs_source_get_settings(f->context);
			obs_data_set_double(settings, "skin_protect", static_cast<double>(skinProtect));
			obs_source_update(f->context, settings);
			obs_data_release(settings);
		}
	}
}

void look_filter_set_split_mode(obs_source_t *source, int splitMode, float wipePos)
{
	std::lock_guard<std::mutex> lock(s_filterMutex);
	for (auto *f : s_activeFilters) {
		if (!source || f->context == source) {
			obs_data_t *settings = obs_source_get_settings(f->context);
			obs_data_set_int(settings, "split_mode", splitMode);
			obs_data_set_double(settings, "wipe_pos", static_cast<double>(wipePos));
			obs_source_update(f->context, settings);
			obs_data_release(settings);
		}
	}
}
