#pragma once

#include <obs-module.h>
#include <graphics/vec2.h>
#include <graphics/vec3.h>

#include <atomic>
#include <memory>
#include <string>

#include "cube-parser.hpp"

#define FILTER_ID "ld_look_studio_filter"

struct LookFilterData {
	obs_source_t *context = nullptr;
	gs_effect_t *effect = nullptr;
	gs_texture_t *clutTexture = nullptr;
	gs_stagesurf_t *stageSurface = nullptr;

	char *lutPath = nullptr;
	float intensity = 1.0f;
	float skinProtect = 0.5f;
	int splitMode = 0; // 0 = off, 1 = horizontal, 2 = vertical
	float wipePos = 0.5f;
	bool showDivider = true;

	struct vec3 clutScale;
	struct vec3 clutOffset;
	struct vec3 domainMin;
	struct vec3 domainMax;

	std::atomic<bool> captureSnapshotPending{false};
};

extern struct obs_source_info ld_look_studio_filter_info;

void look_filter_request_snapshot(obs_source_t *source);
void look_filter_apply_look(obs_source_t *source, const char *lutPath);
void look_filter_set_intensity(obs_source_t *source, float intensity);
void look_filter_set_skin_protect(obs_source_t *source, float skinProtect);
void look_filter_set_split_mode(obs_source_t *source, int splitMode, float wipePos);
