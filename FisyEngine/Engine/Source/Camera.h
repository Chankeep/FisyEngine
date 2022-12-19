#pragma once

#include "d3dUtil.h"

static DirectX::XMFLOAT4X4 Identity4x4()
{
	static DirectX::XMFLOAT4X4 I(
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f);

	return I;
}

class Camera
{
public:

	Camera();
	Camera(UINT screenWidth, UINT screenHeight);
	~Camera() = default;

	// Get/Set world camera position.
	XMVECTOR GetPosition() const;
	XMFLOAT3 GetPosition3f() const;
	void SetPosition(float x, float y, float z);
	void SetPosition(const XMFLOAT3& v);

	// Get camera basis vectors.
	DirectX::XMVECTOR GetRight()const;
	DirectX::XMFLOAT3 GetRight3f()const;
	DirectX::XMVECTOR GetUp()const;
	DirectX::XMFLOAT3 GetUp3f()const;
	DirectX::XMVECTOR GetAtPos()const;
	DirectX::XMFLOAT3 GetAtPos3f()const;

	// Get frustum properties.
	float GetNearZ()const;
	float GetFarZ()const;
	float GetAspect()const;
	float GetFovY()const;
	float GetFovX()const;

	// Get near and far plane dimensions in view space coordinates.
	float GetNearWindowWidth()const;
	float GetNearWindowHeight()const;
	float GetFarWindowWidth()const;
	float GetFarWindowHeight()const;

	// Set frustum.
	void SetLens(float fovY, float aspect, float zn, float zf);

	// Define camera space via LookAt parameters.
	void LookAt(DirectX::FXMVECTOR pos, DirectX::FXMVECTOR target, DirectX::FXMVECTOR worldUp);
	void LookAt(const DirectX::XMFLOAT3& pos, const DirectX::XMFLOAT3& target, const DirectX::XMFLOAT3& up);

	// Get View/Proj matrices.
	DirectX::XMMATRIX GetViewMatrix()const;
	DirectX::XMMATRIX GetProjMatrix()const;

	DirectX::XMFLOAT4X4 GetViewMatrix4x4f()const;
	DirectX::XMFLOAT4X4 GetProjMatrix4x4f()const;

	// Strafe/Walk the camera a distance d.
	void Strafe(float d);
	void Walk(float d);

	// Rotate the camera.
	void Pitch(float angle);
	void Yaw(float angle);

	// After modifying camera position/orientation, call to rebuild the view matrix.
	void UpdateViewMatrix();

private:

	// Camera coordinate system with coordinates relative to world space.
	XMFLOAT3 camPosition = { 0.0f, 0.0f, -25.0f };
	XMFLOAT3 right = { 1.0f, 0.0f, 0.0f };
	XMFLOAT3 up = { 0.0f, 1.0f, 0.0f };
	XMFLOAT3 lookAt = { 0.0f, 0.0f, 1.0f };

	// Cache frustum properties.
	float zNear = 0.0f;
	float zFar = 0.0f;
	float aspectRatio = 0.0f;
	float fovY = 0.0f;
	float nearWindowHeight = 0.0f;
	float farWindowHeight = 0.0f;

	bool needChange = true;

	// Cache View/Proj matrices.
	XMFLOAT4X4 viewMat = Identity4x4();
	XMFLOAT4X4 projMat = Identity4x4();

};

