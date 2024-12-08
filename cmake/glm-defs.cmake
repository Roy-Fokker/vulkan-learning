# Set properties for GLM basically want this anytime it is used
LIST(APPEND glm_definitions
	GLM_FORCE_DEPTH_ZERO_TO_ONE # GLM clip space should be in Z-axis to 0 to 1
	GLM_FORCE_LEFT_HANDED       # GLM should use left-handed coordinates, +z goes into screen
	GLM_FORCE_RADIANS           # GLM should always use radians not degrees.
)