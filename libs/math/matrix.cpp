#include "matrix.h"

namespace
{
	/// \brief Returns \p euler angles converted from degrees to radians.
	inline Vector3 euler_degrees_to_radians(const Vector3& euler)
	{
		return Vector3(
			degrees_to_radians(euler.x()),
			degrees_to_radians(euler.y()),
			degrees_to_radians(euler.z())
		);
	}
}

// Named constructors

// Identity matrix
const Matrix4& Matrix4::getIdentity()
{
    static const Matrix4 _identity(
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    );
    return _identity;
}

// Get a translation matrix for the given vector
Matrix4 Matrix4::getTranslation(const Vector3& translation)
{
    return Matrix4::byColumns(
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        translation.x(), translation.y(), translation.z(), 1
    );
}

// Get a rotation from 2 vectors (named constructor)
Matrix4 Matrix4::getRotation(const Vector3& a, const Vector3& b)
{
	float angle = a.angle(b);
	Vector3 axis = b.crossProduct(a).getNormalised();

	return getRotation(axis, angle);
}

Matrix4 Matrix4::getRotation(const Vector3& axis, const float angle)
{
	// Pre-calculate the terms
	float cosPhi = cos(angle);
	float sinPhi = sin(angle);
	float oneMinusCosPhi = static_cast<float>(1) - cos(angle);
	float x = axis.x();
	float y = axis.y();
	float z = axis.z();
	return Matrix4::byColumns(
		cosPhi + oneMinusCosPhi*x*x, oneMinusCosPhi*x*y - sinPhi*z, oneMinusCosPhi*x*z + sinPhi*y, 0,
		oneMinusCosPhi*y*x + sinPhi*z, cosPhi + oneMinusCosPhi*y*y, oneMinusCosPhi*y*z - sinPhi*x, 0,
		oneMinusCosPhi*z*x - sinPhi*y, oneMinusCosPhi*z*y + sinPhi*x, cosPhi + oneMinusCosPhi*z*z, 0,
		0, 0, 0, 1
	);
}

Matrix4 Matrix4::getRotationAboutXForSinCos(float s, float c)
{
	return Matrix4::byColumns(
		1, 0, 0, 0,
		0, c, s, 0,
		0,-s, c, 0,
		0, 0, 0, 1
	);
}

Matrix4 Matrix4::getRotationAboutYForSinCos(float s, float c)
{
	return Matrix4::byColumns(
		c, 0,-s, 0,
		0, 1, 0, 0,
		s, 0, c, 0,
		0, 0, 0, 1
	);
}

Matrix4 Matrix4::getRotationAboutZForSinCos(float s, float c)
{
	return Matrix4::byColumns(
		c, s, 0, 0,
		-s, c, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1
	);
}

/*! \verbatim
clockwise rotation around X, Y, Z, facing along axis
 1  0   0    cy 0 -sy   cz  sz 0
 0  cx  sx   0  1  0   -sz  cz 0
 0 -sx  cx   sy 0  cy   0   0  1

rows of Z by cols of Y
 cy*cz -sy*cz+sz -sy*sz+cz
-sz*cy -sz*sy+cz

  .. or something like that..

final rotation is Z * Y * X
 cy*cz -sx*-sy*cz+cx*sz  cx*-sy*sz+sx*cz
-cy*sz  sx*sy*sz+cx*cz  -cx*-sy*sz+sx*cz
 sy    -sx*cy            cx*cy

transposed
cy.cz + 0.sz + sy.0            cy.-sz + 0 .cz +  sy.0          cy.0  + 0 .0  +   sy.1       |
sx.sy.cz + cx.sz + -sx.cy.0    sx.sy.-sz + cx.cz + -sx.cy.0    sx.sy.0  + cx.0  + -sx.cy.1  |
-cx.sy.cz + sx.sz +  cx.cy.0   -cx.sy.-sz + sx.cz +  cx.cy.0   -cx.sy.0  + 0 .0  +  cx.cy.1  |
\endverbatim */
Matrix4 Matrix4::getRotationForEulerXYZ(const Vector3& euler)
{
	float cx = cos(euler[0]);
	float sx = sin(euler[0]);
	float cy = cos(euler[1]);
	float sy = sin(euler[1]);
	float cz = cos(euler[2]);
	float sz = sin(euler[2]);

	return Matrix4::byColumns(
		cy*cz,
		cy*sz,
		-sy,
		0,
		sx*sy*cz + cx*-sz,
		sx*sy*sz + cx*cz,
		sx*cy,
		0,
		cx*sy*cz + sx*sz,
		cx*sy*sz + -sx*cz,
		cx*cy,
		0,
		0,
		0,
		0,
		1
	);
}

Matrix4 Matrix4::getRotationForEulerXYZDegrees(const Vector3& euler)
{
	return getRotationForEulerXYZ(euler_degrees_to_radians(euler));
}

Matrix4 Matrix4::getRotationForEulerYZX(const Vector3& euler)
{
	float cx = cos(euler[0]);
	float sx = sin(euler[0]);
	float cy = cos(euler[1]);
	float sy = sin(euler[1]);
	float cz = cos(euler[2]);
	float sz = sin(euler[2]);

	return Matrix4::byColumns(
		cy*cz,
		cx*cy*sz + sx*sy,
		sx*cy*sz - cx*sy,
		0,
		-sz,
		cx*cz,
		sx*cz,
		0,
		sy*cz,
		cx*sy*sz - sx*cy,
		sx*sy*sz + cx*cy,
		0,
		0,
		0,
		0,
		1
	);
}

Matrix4 Matrix4::getRotationForEulerYZXDegrees(const Vector3& euler)
{
	return getRotationForEulerYZX(euler_degrees_to_radians(euler));
}

Matrix4 Matrix4::getRotationForEulerXZY(const Vector3& euler)
{
	float cx = cos(euler[0]);
	float sx = sin(euler[0]);
	float cy = cos(euler[1]);
	float sy = sin(euler[1]);
	float cz = cos(euler[2]);
	float sz = sin(euler[2]);

	return Matrix4::byColumns(
		cy*cz,
		sz,
		-sy*cz,
		0,
		sx*sy - cx*cy*sz,
		cx*cz,
		cx*sy*sz + sx*cy,
		0,
		sx*cy*sz + cx*sy,
		-sx*cz,
		cx*cy - sx*sy*sz,
		0,
		0,
		0,
		0,
		1
	);
}

Matrix4 Matrix4::getRotationForEulerXZYDegrees(const Vector3& euler)
{
	return getRotationForEulerXZY(euler_degrees_to_radians(euler));
}

Matrix4 Matrix4::getRotationForEulerYXZ(const Vector3& euler)
{
	float cx = cos(euler[0]);
	float sx = sin(euler[0]);
	float cy = cos(euler[1]);
	float sy = sin(euler[1]);
	float cz = cos(euler[2]);
	float sz = sin(euler[2]);

	return Matrix4::byColumns(
		cy*cz - sx*sy*sz,
		cy*sz + sx*sy*cz,
		-cx*sy,
		0,
		-cx*sz,
		cx*cz,
		sx,
		0,
		sy*cz + sx*cy*sz,
		sy*sz - sx*cy*cz,
		cx*cy,
		0,
		0,
		0,
		0,
		1
	);
}

Matrix4 Matrix4::getRotationForEulerYXZDegrees(const Vector3& euler)
{
	return getRotationForEulerYXZ(euler_degrees_to_radians(euler));
}

Matrix4 Matrix4::getRotationForEulerZXY(const Vector3& euler)
{
	float cx = cos(euler[0]);
	float sx = sin(euler[0]);
	float cy = cos(euler[1]);
	float sy = sin(euler[1]);
	float cz = cos(euler[2]);
	float sz = sin(euler[2]);

	return Matrix4::byColumns(
		cy*cz + sx*sy*sz,
		cx*sz,
		sx*cy*sz - sy*cz,
		0,
		sx*sy*cz - cy*sz,
		cx*cz,
		sy*sz + sx*cy*cz,
		0,
		cx*sy,
		-sx,
		cx*cy,
		0,
		0,
		0,
		0,
		1
	);
}

Matrix4 Matrix4::getRotationForEulerZXYDegrees(const Vector3& euler)
{
	return getRotationForEulerZXY(euler_degrees_to_radians(euler));
}

Matrix4 Matrix4::getRotationForEulerZYX(const Vector3& euler)
{
	float cx = cos(euler[0]);
	float sx = sin(euler[0]);
	float cy = cos(euler[1]);
	float sy = sin(euler[1]);
	float cz = cos(euler[2]);
	float sz = sin(euler[2]);

	return Matrix4::byColumns(
		cy*cz,
		cx*sz + sx*sy*cz,
		sx*sz - cx*sy*cz,
		0,
		-cy*sz,
		cx*cz - sx*sy*sz,
		sx*cz + cx*sy*sz,
		0,
		sy,
		-sx*cy,
		cx*cy,
		0,
		0,
		0,
		0,
		1
	);
}

Matrix4 Matrix4::getRotationForEulerZYXDegrees(const Vector3& euler)
{
	return getRotationForEulerZYX(euler_degrees_to_radians(euler));
}

// Get a scale matrix
Matrix4 Matrix4::getScale(const Vector3& scale)
{
    return Matrix4::byColumns(
        scale[0], 0, 0, 0,
        0, scale[1], 0, 0,
        0, 0, scale[2], 0,
        0, 0, 0,        1
    );
}

// Transpose the matrix in-place
void Matrix4::transpose()
{
    std::swap(_m[1], _m[4]); // xy <=> yx
    std::swap(_m[2], _m[8]); // xz <=> zx
    std::swap(_m[3], _m[12]); // xw <=> tx
    std::swap(_m[6], _m[9]); // yz <=> zy
    std::swap(_m[7], _m[13]); // yw <=> ty
    std::swap(_m[11], _m[14]); // zw <=> tz
}

// Return transposed copy
Matrix4 Matrix4::getTransposed() const
{
    return Matrix4(
        xx(), yx(), zx(), tx(),
        xy(), yy(), zy(), ty(),
        xz(), yz(), zz(), tz(),
        xw(), yw(), zw(), tw()
    );
}

// Return affine inverse
Matrix4 Matrix4::getInverse() const
{
  Matrix4 result;

  // determinant of rotation submatrix
  float det
    = _m[0] * ( _m[5]*_m[10] - _m[9]*_m[6] )
    - _m[1] * ( _m[4]*_m[10] - _m[8]*_m[6] )
    + _m[2] * ( _m[4]*_m[9] - _m[8]*_m[5] );

  // throw exception here if (det*det < 1e-25)

  // invert rotation submatrix
  det = 1.0f / det;

  result[0] = (  (_m[5]*_m[10]- _m[6]*_m[9] )*det);
  result[1] = (- (_m[1]*_m[10]- _m[2]*_m[9] )*det);
  result[2] = (  (_m[1]*_m[6] - _m[2]*_m[5] )*det);
  result[3] = 0;
  result[4] = (- (_m[4]*_m[10]- _m[6]*_m[8] )*det);
  result[5] = (  (_m[0]*_m[10]- _m[2]*_m[8] )*det);
  result[6] = (- (_m[0]*_m[6] - _m[2]*_m[4] )*det);
  result[7] = 0;
  result[8] = (  (_m[4]*_m[9] - _m[5]*_m[8] )*det);
  result[9] = (- (_m[0]*_m[9] - _m[1]*_m[8] )*det);
  result[10]= (  (_m[0]*_m[5] - _m[1]*_m[4] )*det);
  result[11] = 0;

  // multiply translation part by rotation
  result[12] = - (_m[12] * result[0] +
    _m[13] * result[4] +
    _m[14] * result[8]);
  result[13] = - (_m[12] * result[1] +
    _m[13] * result[5] +
    _m[14] * result[9]);
  result[14] = - (_m[12] * result[2] +
    _m[13] * result[6] +
    _m[14] * result[10]);
  result[15] = 1;

  return result;
}

Matrix4 Matrix4::getFullInverse() const
{
	// The inverse is generated through the adjugate matrix

	// 2x2 minors (re-usable for the determinant)
	float minor01 = zz() * tw() - zw() * tz();
	float minor02 = zy() * tw() - zw() * ty();
	float minor03 = zx() * tw() - zw() * tx();
	float minor04 = zy() * tz() - zz() * ty();
	float minor05 = zx() * tz() - zz() * tx();
	float minor06 = zx() * ty() - zy() * tx();

	// 2x2 minors (not usable for the determinant)
	float minor07 = yz() * tw() - yw() * tz();
	float minor08 = yy() * tw() - yw() * ty();
	float minor09 = yy() * tz() - yz() * ty();
	float minor10 = yx() * tw() - yw() * tx();
	float minor11 = yx() * tz() - yz() * tx();
	float minor12 = yx() * ty() - yy() * tx();
	float minor13 = yz() * zw() - yw() * zz();
	float minor14 = yy() * zw() - yw() * zy();
	float minor15 = yy() * zz() - yz() * zy();
	float minor16 = yx() * zw() - yw() * zx();
	float minor17 = yx() * zz() - yz() * zx();
	float minor18 = yx() * zy() - yy() * zx();

	// 3x3 minors (re-usable for the determinant)
	float minor3x3_11 = yy() * minor01 - yz() * minor02 + yw() * minor04;
	float minor3x3_21 = yx() * minor01 - yz() * minor03 + yw() * minor05;
	float minor3x3_31 = yx() * minor02 - yy() * minor03 + yw() * minor06;
	float minor3x3_41 = yx() * minor04 - yy() * minor05 + yz() * minor06;
	
	// 3x3 minors (not usable for the determinant)
	float minor3x3_12 = xy() * minor01 - xz() * minor02 + xw() * minor04;
	float minor3x3_22 = xx() * minor01 - xz() * minor03 + xw() * minor05;
	float minor3x3_32 = xx() * minor02 - xy() * minor03 + xw() * minor06;
	float minor3x3_42 = xx() * minor04 - xy() * minor05 + xz() * minor06;
	
	float minor3x3_13 = xy() * minor07 - xz() * minor08 + xw() * minor09;
	float minor3x3_23 = xx() * minor07 - xz() * minor10 + xw() * minor11;
	float minor3x3_33 = xx() * minor08 - xy() * minor10 + xw() * minor12;
	float minor3x3_43 = xx() * minor09 - xy() * minor11 + xz() * minor12;

	float minor3x3_14 = xy() * minor13 - xz() * minor14 + xw() * minor15;
	float minor3x3_24 = xx() * minor13 - xz() * minor16 + xw() * minor17;
	float minor3x3_34 = xx() * minor14 - xy() * minor16 + xw() * minor18;
	float minor3x3_44 = xx() * minor15 - xy() * minor17 + xz() * minor18;

	float determinant = xx() * minor3x3_11 - xy() * minor3x3_21 + xz() * minor3x3_31 - xw() * minor3x3_41;
	float invDet = 1.0f / determinant;

	return Matrix4::byColumns(
		+minor3x3_11 * invDet, -minor3x3_12 * invDet, +minor3x3_13 * invDet, -minor3x3_14 * invDet,
		-minor3x3_21 * invDet, +minor3x3_22 * invDet, -minor3x3_23 * invDet, +minor3x3_24 * invDet,
		+minor3x3_31 * invDet, -minor3x3_32 * invDet, +minor3x3_33 * invDet, -minor3x3_34 * invDet,
		-minor3x3_41 * invDet, +minor3x3_42 * invDet, -minor3x3_43 * invDet, +minor3x3_44 * invDet
	);
}

// Transform a plane
Plane3 Matrix4::transform(const Plane3& plane) const
{
    Plane3 transformed;
    transformed.normal().x() = _m[0] * plane.normal().x() + _m[4] * plane.normal().y() + _m[8] * plane.normal().z();
    transformed.normal().y() = _m[1] * plane.normal().x() + _m[5] * plane.normal().y() + _m[9] * plane.normal().z();
    transformed.normal().z() = _m[2] * plane.normal().x() + _m[6] * plane.normal().y() + _m[10] * plane.normal().z();
    transformed.dist() = -(	(-plane.dist() * transformed.normal().x() + _m[12]) * transformed.normal().x() +
                        (-plane.dist() * transformed.normal().y() + _m[13]) * transformed.normal().y() +
                        (-plane.dist() * transformed.normal().z() + _m[14]) * transformed.normal().z());
    return transformed;
}

// Inverse transform a plane
Plane3 Matrix4::inverseTransform(const Plane3& plane) const
{
    return Plane3(
        _m[ 0] * plane.normal().x() + _m[ 1] * plane.normal().y() + _m[ 2] * plane.normal().z() + _m[ 3] * plane.dist(),
        _m[ 4] * plane.normal().x() + _m[ 5] * plane.normal().y() + _m[ 6] * plane.normal().z() + _m[ 7] * plane.dist(),
        _m[ 8] * plane.normal().x() + _m[ 9] * plane.normal().y() + _m[10] * plane.normal().z() + _m[11] * plane.dist(),
        _m[12] * plane.normal().x() + _m[13] * plane.normal().y() + _m[14] * plane.normal().z() + _m[15] * plane.dist()
    );
}

// Multiply by another matrix, in-place
void Matrix4::multiplyBy(const Matrix4& other)
{
    *this = getMultipliedBy(other);
}

// Add a translation component
void Matrix4::translateBy(const Vector3& translation)
{
    multiplyBy(getTranslation(translation));
}

// Add a scale component
void Matrix4::scaleBy(const Vector3& scale)
{
    multiplyBy(getScale(scale));
}

