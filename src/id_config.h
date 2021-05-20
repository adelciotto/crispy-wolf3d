#ifndef CRISPY_WOLF3D_ID_CONFIG_H
#define CRISPY_WOLF3D_ID_CONFIG_H

typedef enum {
	CONFIG_OPTION_TYPE_UINT,
	CONFIG_OPTION_TYPE_BOOL,
	CONFIG_OPTION_TYPE_FLOAT
} ConfigOptionType;

typedef struct {
	const char *name;
	ConfigOptionType type;

	union {
		unsigned int *uintValue;
		boolean *boolValue;
		float *floatValue;
	};
} ConfigOption;

extern boolean ConfigOptionFullscreen;
extern boolean ConfigOptionHighRes;
extern boolean ConfigOptionAspectRatioCorrection;
extern unsigned int ConfigOptionWindowScale;

extern boolean ConfigOptionModernKeyboardMouse;
extern unsigned int ConfigOptionMouseThreshold;
extern float ConfigOptionMouseAccel;

void CONFIG_Read(const char *configDir);
void CONFIG_Save(const char *configDir);

#endif //CRISPY_WOLF3D_ID_CONFIG_H
