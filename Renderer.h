#pragma once

class Renderer {
protected:
	bool m_Active;

public:

	// Get a reference to the Renderer
	static Renderer& Get();

	Renderer() {}
	virtual ~Renderer() {}

	bool Start(); // Called by Renderer() by default
	bool Stop(); 

	bool IsActive() { return m_Active; }
};