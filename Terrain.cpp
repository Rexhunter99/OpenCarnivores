#include "Terrain.h"

#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include <exception>




Terrain::Terrain()
{
	m_XSize = 512;
	m_YSize = 512;
	m_Scale = 256.f;
	m_HScale = 32.f;
	m_HeightField = nullptr;
	m_TextureField[0] = nullptr;
	m_TextureField[1] = nullptr;
	m_ObjectField = nullptr;
	m_FlagsField = nullptr;
	m_LightField = nullptr;
	m_HeightField2 = nullptr;
	m_ObjectHeightField = nullptr;
	m_FogsField = nullptr;
	m_AmbientField = nullptr;
}


Terrain::~Terrain()
{
	reset();
}


void Terrain::reset()
{
	if (m_HeightField)
		delete[] m_HeightField;
	if (m_TextureField[0])
		delete[] m_TextureField[0];
	if (m_TextureField[1])
		delete[] m_TextureField[1];
	if (m_ObjectField)
		delete[] m_ObjectField;
	if (m_FlagsField)
		delete[] m_FlagsField;
	if (m_LightField)
		delete[] m_LightField;
	if (m_HeightField2)
		delete[] m_HeightField2;
	if (m_ObjectHeightField)
		delete[] m_ObjectHeightField;
	if (m_FogsField)
		delete[] m_FogsField;
	if (m_AmbientField)
		delete[] m_AmbientField;

	m_HeightField = nullptr;
	m_TextureField[0] = nullptr;
	m_TextureField[1] = nullptr;
	m_ObjectField = nullptr;
	m_FlagsField = nullptr;
	m_LightField = nullptr;
	m_HeightField2 = nullptr;
	m_ObjectHeightField = nullptr;
	m_FogsField = nullptr;
	m_AmbientField = nullptr;
}


void Terrain::load(const std::string& file_name)
{
	std::ifstream file(file_name, std::ios::binary);

	std::cout << "Loading map file" << std::endl;

	if (!file.is_open()) {
		std::stringstream ss;
		ss << "Error opening map file: \'" << file_name << "\'";
		throw std::runtime_error(ss.str());
		return;
	}

	this->reset();

	m_HeightField = new unsigned char[m_XSize * m_YSize];
	m_TextureField[0] = new unsigned char[m_XSize * m_YSize];
	m_TextureField[1] = new unsigned char[m_XSize * m_YSize];
	m_ObjectField = new unsigned char[m_XSize * m_YSize];
	m_FlagsField = new unsigned char[m_XSize * m_YSize];
	m_LightField = new unsigned char[m_XSize * m_YSize];
	m_HeightField2 = new unsigned char[m_XSize * m_YSize];
	m_ObjectHeightField = new unsigned char[m_XSize * m_YSize];
	m_FogsField = new unsigned char[(m_XSize/2) * (m_YSize/2)];
	m_AmbientField = new unsigned char[(m_XSize/2) * (m_YSize/2)];

	// Note: this appeases Intellisense not liking the conversion from a 4-byte integer to an 8-byte integer
	std::streamsize ss = (std::streamsize)m_XSize * (std::streamsize)m_YSize;

	file.read((char*)m_HeightField, ss);
	file.read((char*)m_TextureField[0], ss);
	file.read((char*)m_TextureField[1], ss);
	file.read((char*)m_ObjectField, ss);
	file.read((char*)m_FlagsField, ss);
	file.read((char*)m_LightField, ss);
	file.read((char*)m_HeightField2, ss); //Water?
	file.read((char*)m_ObjectHeightField, ss);
	file.read((char*)m_FogsField, ss / 2);
	file.read((char*)m_AmbientField, ss / 2);
}


int Terrain::getXSize()
{
	return m_XSize;
}


int Terrain::getYSize()
{
	return m_YSize;
}


float Terrain::getScale()
{
	return m_Scale;
}


float Terrain::getHeightScale()
{
	return m_HScale;
}


float Terrain::getHeight(int x, int y)
{
	if (x < 0)
		x = 0;
	if (y < 0)
		y = 0;
	if (x >= m_XSize)
		x = m_XSize - 1;
	if (y >= m_YSize)
		y = m_YSize - 1;

	return (float)m_HeightField[x * y] * m_HScale;
}


int Terrain::getTexture(int map, int x, int y)
{
	return 0;
}