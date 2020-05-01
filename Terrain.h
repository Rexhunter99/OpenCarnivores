#pragma once

#include <string>


class Terrain {
protected:
	int m_XSize, m_YSize;
	float m_Scale, m_HScale;

	unsigned char* m_HeightField;
	unsigned char* m_TextureField[2];
	unsigned char* m_ObjectField;
	unsigned char* m_FlagsField;
	unsigned char* m_LightField;
	unsigned char* m_HeightField2;
	unsigned char* m_ObjectHeightField;
	unsigned char* m_FogsField;
	unsigned char* m_AmbientField;

public:
	Terrain();
	~Terrain();

	void reset();
	void load(const std::string&);

	// -- Getters -- //
	int getXSize();
	int getYSize();
	float getScale();
	float getHeightScale();

	// -- Utility -- //
	float getHeight(int, int);
	int getTexture(int, int, int);
};