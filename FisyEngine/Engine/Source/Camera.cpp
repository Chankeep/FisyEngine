#include "Camera.h"

Camera::Camera()
{
	SetLens(XM_PIDIV4, 1.0f, 0.1f, 1000.0f);
}

Camera::Camera(UINT screenWidth, UINT screenHeight)
{
	SetLens(XM_PIDIV4, static_cast<float>(screenWidth) / static_cast<float>(screenHeight), 0.1f, 1000.0f);
}

XMVECTOR Camera::GetPosition() const
{
	return XMLoadFloat3(&camPosition);
}

XMFLOAT3 Camera::GetPosition3f() const
{
	return camPosition;
}

void Camera::SetPosition(float x, float y, float z)
{
	camPosition = XMFLOAT3(x, y, z);
	needChange = true;
}

void Camera::SetPosition(const XMFLOAT3& v)
{
	camPosition = v;
	needChange = true;
}

DirectX::XMVECTOR Camera::GetRight() const
{
	return XMLoadFloat3(&right);
}
DirectX::XMFLOAT3 Camera::GetRight3f() const
{
	return right;
}
DirectX::XMVECTOR Camera::GetUp() const
{
	return XMLoadFloat3(&up);
}
DirectX::XMFLOAT3 Camera::GetUp3f() const
{
	return up;
}
DirectX::XMVECTOR Camera::GetAtPos() const
{
	return XMLoadFloat3(&lookAt);
}
DirectX::XMFLOAT3 Camera::GetAtPos3f() const
{
	return lookAt;
}
float Camera::GetNearZ() const
{
	return zNear;
}
float Camera::GetFarZ() const
{
	return zFar;
}
float Camera::GetAspect() const
{
	return aspectRatio;
}
float Camera::GetFovY() const
{
	return fovY;
}
float Camera::GetFovX() const
{
	const float halfWidth = 0.5f * GetNearWindowWidth();
	return 2.0f * atan(halfWidth / zNear);
}
float Camera::GetNearWindowWidth() const
{
	return aspectRatio * nearWindowHeight;
}
float Camera::GetNearWindowHeight() const
{
	return nearWindowHeight;
}
float Camera::GetFarWindowWidth() const
{
	return aspectRatio * farWindowHeight;
}
float Camera::GetFarWindowHeight() const
{
	return farWindowHeight;
}
void Camera::SetLens(float fovY, float aspect, float zn, float zf)
{
	this->fovY = fovY;
	aspectRatio = aspect;
	zNear = zn;
	zFar = zf;

	nearWindowHeight = 2.0f * zNear * tanf(0.5f * fovY);
	farWindowHeight = 2.0f * zFar * tanf(0.5f * fovY);

	const XMMATRIX proj = XMMatrixPerspectiveFovLH(fovY, aspectRatio, zNear, zFar);
	XMStoreFloat4x4(&projMat, proj);
}
void Camera::LookAt(FXMVECTOR pos, FXMVECTOR target, FXMVECTOR worldUp)
{
	XMVECTOR look = XMVector3Normalize(XMVectorSubtract(target, pos));
	XMVECTOR Right = XMVector3Normalize(XMVector3Cross(worldUp, look));
	XMVECTOR Up = XMVector3Cross(look, Right);

	XMStoreFloat3(&camPosition, pos);
	XMStoreFloat3(&lookAt, look);
	XMStoreFloat3(&right, Right);
	XMStoreFloat3(&up, Up);

	needChange = true;
}
void Camera::LookAt(const XMFLOAT3& pos, const XMFLOAT3& target, const XMFLOAT3& up)
{
	XMVECTOR Pos = XMLoadFloat3(&pos);
	XMVECTOR Target = XMLoadFloat3(&target);
	XMVECTOR Up = XMLoadFloat3(&up);

	LookAt(Pos, Target, Up);

	needChange = true;
}
XMMATRIX Camera::GetViewMatrix() const
{
	assert(!needChange);
	return XMLoadFloat4x4(&viewMat);
}
XMMATRIX Camera::GetProjMatrix() const
{
	return XMLoadFloat4x4(&projMat);
}
XMFLOAT4X4 Camera::GetViewMatrix4x4f() const
{
	return viewMat;
}
XMFLOAT4X4 Camera::GetProjMatrix4x4f() const
{
	return projMat;
}
void Camera::Strafe(float d)
{
	//camPosition += distance * rightVec
	XMVECTOR distance = XMVectorReplicate(d);
	XMVECTOR Right = XMLoadFloat3(&right);
	XMVECTOR pos = XMLoadFloat3(&camPosition);
	XMStoreFloat3(&camPosition, XMVectorMultiplyAdd(distance, Right, pos));

	needChange = true;
}
void Camera::Walk(float d)
{
	//camPosition += distance * forwardVec
	XMVECTOR distance = XMVectorReplicate(d);
	XMVECTOR Forward = XMLoadFloat3(&lookAt);
	XMVECTOR pos = XMLoadFloat3(&camPosition);
	XMStoreFloat3(&camPosition, XMVectorMultiplyAdd(distance, Forward, pos));

	needChange = true;
}
void Camera::Pitch(float angle)
{
	XMMATRIX R = XMMatrixRotationAxis(XMLoadFloat3(&right), angle);

	XMStoreFloat3(&up, XMVector3TransformNormal(XMLoadFloat3(&up), R));
	XMStoreFloat3(&lookAt, XMVector3TransformNormal(XMLoadFloat3(&lookAt), R));

	needChange = true;
}
void Camera::Yaw(float angle)
{
	XMMATRIX R = XMMatrixRotationY(angle);

	XMStoreFloat3(&right, XMVector3TransformNormal(XMLoadFloat3(&right), R));
	XMStoreFloat3(&up, XMVector3TransformNormal(XMLoadFloat3(&up), R));
	XMStoreFloat3(&lookAt, XMVector3TransformNormal(XMLoadFloat3(&lookAt), R));

	needChange = true;
}
void Camera::UpdateViewMatrix()
{
	if (needChange)
	{
		XMVECTOR R = XMLoadFloat3(&right);
		XMVECTOR U = XMLoadFloat3(&up);
		XMVECTOR L = XMLoadFloat3(&lookAt);
		XMVECTOR P = XMLoadFloat3(&camPosition);

		// Keep camera's axes orthogonal to each other and of unit length.
		L = XMVector3Normalize(L);
		U = XMVector3Normalize(XMVector3Cross(L, R));

		// U, L already ortho-normal, so no need to normalize cross product.
		R = XMVector3Cross(U, L);

		// Fill in the view matrix entries.
		float x = -XMVectorGetX(XMVector3Dot(P, R));
		float y = -XMVectorGetX(XMVector3Dot(P, U));
		float z = -XMVectorGetX(XMVector3Dot(P, L));

		XMStoreFloat3(&right, R);
		XMStoreFloat3(&up, U);
		XMStoreFloat3(&lookAt, L);

		viewMat(0, 0) = right.x;
		viewMat(1, 0) = right.y;
		viewMat(2, 0) = right.z;
		viewMat(3, 0) = x;

		viewMat(0, 1) = up.x;
		viewMat(1, 1) = up.y;
		viewMat(2, 1) = up.z;
		viewMat(3, 1) = y;

		viewMat(0, 2) = lookAt.x;
		viewMat(1, 2) = lookAt.y;
		viewMat(2, 2) = lookAt.z;
		viewMat(3, 2) = z;

		viewMat(0, 3) = 0.0f;
		viewMat(1, 3) = 0.0f;
		viewMat(2, 3) = 0.0f;
		viewMat(3, 3) = 1.0f;

		needChange = false;
	}
}
