#include <errno.h>
#include <sys/stat.h>

#include "wl_def.h"

#define FILENAME "crispy-wolf3d.cfg"

boolean ConfigOptionFullscreen = true;
boolean ConfigOptionHighRes = true;
boolean ConfigOptionAspectRatioCorrection = true;
unsigned int ConfigOptionWindowScale = 1;

boolean ConfigOptionModernKeyboardMouse = true;
unsigned int ConfigOptionMouseThreshold = 10;
float ConfigOptionMouseAccel = 1.0f;

static ConfigOption options[] = {
	{ .name = "fullscreen",
	  .type = CONFIG_OPTION_TYPE_BOOL,
	  .boolValue = &ConfigOptionFullscreen },
	{ .name = "high_res",
	  .type = CONFIG_OPTION_TYPE_BOOL,
	  .boolValue = &ConfigOptionHighRes },
	{ .name = "aspect_ratio_correction",
	  .type = CONFIG_OPTION_TYPE_BOOL,
	  .boolValue = &ConfigOptionAspectRatioCorrection },
	{ .name = "window_scale",
	  .type = CONFIG_OPTION_TYPE_UINT,
	  .uintValue = &ConfigOptionWindowScale },

	{ .name = "modern_keyboard_mouse",
	  .type = CONFIG_OPTION_TYPE_BOOL,
	  .boolValue = &ConfigOptionModernKeyboardMouse },
	{ .name = "mouse_threshold",
	  .type = CONFIG_OPTION_TYPE_UINT,
	  .uintValue = &ConfigOptionMouseThreshold },
	{ .name = "mouse_accel",
	  .type = CONFIG_OPTION_TYPE_FLOAT,
	  .floatValue = &ConfigOptionMouseAccel }
};

static ConfigOption *FindConfigOption(char *name);
static int ParseConfigOption(ConfigOption *configOption, char *value);

void CONFIG_Read(const char *configDir)
{
	char configPath[300];

	if (configDir[0])
		snprintf(configPath, sizeof(configPath), "%s/%s", configDir, FILENAME);
	else
		strcpy(configPath, FILENAME);

	struct stat buffer;
	if (stat(configPath, &buffer) != 0) {
		LOG_Info("No config found at %s", configPath);
		CONFIG_Save(configDir);
		return;
	}

	FILE *file = fopen(configPath, "r");
	if (!file) {
		LOG_Error("Failed to open config at %s", configPath);
		return;
	}

	char line[256];
	while (fgets(line, 256, file)) {
		char *name = strtok(line, "=");
		if (!name) {
			LOG_Error("Error reading name from option in config");
			continue;
		}

		ConfigOption *configOption = FindConfigOption(name);
		if (!configOption) {
			LOG_Error("Invalid option %s in config", name);
			continue;
		}

		char *value = strtok(NULL, "=");
		if (!value) {
			LOG_Error("Error reading value for option %s in config", name);
			continue;
		}

		// Strip the trailing newline from the value.
		value[strcspn(value, "\n")] = 0;

		if (ParseConfigOption(configOption, value) != 0) {
			LOG_Error("Error updating value for option %s from config", name);
			continue;
		}

		LOG_Info("Config option %s loaded with value %s", name, value);
	}
}

void CONFIG_Save(const char *configDir)
{
	char configPath[300];

	if (configDir[0])
		snprintf(configPath, sizeof(configPath), "%s/%s", configDir, FILENAME);
	else
		strcpy(configPath, FILENAME);

	FILE *file = fopen(configPath, "w");
	if (!file) {
		LOG_Error("Failed to open config at %s", configDir);
		return;
	}

	int len = lengthof(options);
	for (int i = 0; i < len; i++) {
		ConfigOption option = options[i];
		switch (option.type) {
		case CONFIG_OPTION_TYPE_UINT:
			fprintf(file, "%s=%u", option.name, *option.uintValue);
			break;
		case CONFIG_OPTION_TYPE_BOOL: {
			const char *valueStr = *option.boolValue ? "true" : "false";
			fprintf(file, "%s=%s", option.name, valueStr);
			break;
		case CONFIG_OPTION_TYPE_FLOAT:
			fprintf(file, "%s=%.1f", option.name, *option.floatValue);
			break;
		}
		}

		fprintf(file, "\n");
	}

	LOG_Info("Config successfully saved to %s", configPath);
}

static ConfigOption *FindConfigOption(char *name)
{
	int len = lengthof(options);
	for (int i = 0; i < len; i++) {
		if (strcmp(options[i].name, name) == 0)
			return &options[i];
	}

	return NULL;
}

int StrToUint(const char *s, unsigned int *out)
{
	if (strchr(s, '-') != NULL)
		return -1;

	char *endPtr;
	errno = 0;
	unsigned long ul = strtoul(s, &endPtr, 10);

	// Overflow, no conversion or trailing junk?
	if (errno || endPtr == s || *endPtr)
		return -1;

	*out = (unsigned int)ul;
	return 0;
}

int StrToFloat(const char *s, float *out)
{
	char *endPtr;
	errno = 0;
	float val = strtof(s, &endPtr);

	// Overflow, no conversion or trailing junk?
	if (errno || endPtr == s || *endPtr)
		return -1;

	*out = val;
	return 0;
}

static int ParseConfigOption(ConfigOption *configOption, char *value)
{
	switch (configOption->type) {
	case CONFIG_OPTION_TYPE_UINT: {
		unsigned int uintValue;
		if (StrToUint(value, &uintValue) != 0) {
			LOG_Error("Failed to parse uint option value %s", value);
			return -1;
		}

		*configOption->uintValue = uintValue;
		break;
	}
	case CONFIG_OPTION_TYPE_BOOL: {
		boolean boolValue;
		if (strcmp(value, "true") == 0) {
			boolValue = true;
		} else if (strcmp(value, "false") == 0) {
			boolValue = false;
		} else {
			LOG_Error("Failed to parse boolean option value %s", value);
			return -1;
		}

		*configOption->boolValue = boolValue;
		break;
	}
	case CONFIG_OPTION_TYPE_FLOAT: {
		float floatValue;
		if (StrToFloat(value, &floatValue) != 0) {
			LOG_Error("Failed to parse float option value %s", value);
			return -1;
		}

		if (floatValue == 0.0f || isnan(floatValue) || isinf(floatValue)) {
			LOG_Error("Float option must have a valid positive value, got: %s",
					  value);
			return -1;
		}

		*configOption->floatValue = floatValue;
		break;
	}
	}

	return 0;
}
