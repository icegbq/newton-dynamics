/* Copyright (c) <2003-2011> <Julio Jerez, Newton Game Dynamics>
* 
* This software is provided 'as-is', without any express or implied
* warranty. In no event will the authors be held liable for any damages
* arising from the use of this software.
* 
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely, subject to the following restrictions:
* 
* 1. The origin of this software must not be misrepresented; you must not
* claim that you wrote the original software. If you use this software
* in a product, an acknowledgment in the product documentation would be
* appreciated but is not required.
* 
* 2. Altered source versions must be plainly marked as such, and must not be
* misrepresented as being the original software.
* 
* 3. This notice may not be removed or altered from any source distribution.
*/

#include "dgPhysicsStdafx.h"
#include "dgBody.h"
#include "dgWorld.h"
#include "dgContact.h"
#include "dgCollisionTaperedCapsule.h"

#define DG_TAPED_CAPSULE_SEGMENTS		10
#define DG_TAPED_CAPSULE_CAP_SEGMENTS	12

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
dgCollisionTaperedCapsule::dgCollisionTaperedCapsule(dgMemoryAllocator* allocator, dgUnsigned32 signature, dgFloat32 radio0, dgFloat32 radio1, dgFloat32 height)
	:dgCollisionConvex(allocator, signature, m_taperedCapsuleCollision)
{
	Init (radio0, radio1, height);
}

dgCollisionTaperedCapsule::dgCollisionTaperedCapsule(dgWorld* const world, dgDeserialize deserialization, void* const userData, dgInt32 revisionNumber)
	:dgCollisionConvex (world, deserialization, userData, revisionNumber)
{
	dgVector size;
	deserialization(userData, &size, sizeof (dgVector));
	deserialization(userData, &m_p0, sizeof (dgVector));
	deserialization(userData, &m_p1, sizeof (dgVector));
	deserialization(userData, &m_transform, sizeof (dgVector));
	m_radio0 = size.m_x;
	m_radio1 = size.m_y;
	m_height = size.m_z;

//	Init (size.m_x, size.m_y, size.m_z);
}


dgCollisionTaperedCapsule::~dgCollisionTaperedCapsule()
{
}

void dgCollisionTaperedCapsule::Init (dgFloat32 radio0, dgFloat32 radio1, dgFloat32 height)
{
	m_rtti |= dgCollisionTaperedCapsule_RTTI;

	radio0 = dgAbsf(radio0);
	radio1 = dgAbsf(radio1);
	height = dgAbsf(height);

	m_transform = dgVector (dgFloat32 (1.0f), dgFloat32 (1.0f), dgFloat32 (1.0f), dgFloat32 (0.0f));
	if (radio0 > radio1) {
		m_transform.m_x = dgFloat32 (-1.0f);
		m_transform.m_y = dgFloat32 (-1.0f);
		dgSwap(radio0, radio1);
	}

	m_radio0 = radio0;
	m_radio1 = radio1;
	m_height = height * dgFloat32 (0.5f);

	m_p0 = dgVector (- m_height, m_radio0, dgFloat32 (0.0f), dgFloat32 (0.0f)); 
	m_p1 = dgVector (  m_height, m_radio1, dgFloat32 (0.0f), dgFloat32 (0.0f)); 
	dgVector side (dgFloat32 (0.0f), dgFloat32 (0.0f), dgFloat32 (1.0f), dgFloat32 (0.0f));

	for (int i = 0; i < 16; i ++) {
		dgVector p1p0 (m_p1 - m_p0);
		dgVector dir(side * p1p0);
		dir = dir.Scale4(dgFloat32 (1.0f) / dgSqrt(dir.DotProduct4(dir).GetScalar()));
		dgVector support0(dir.Scale4(m_radio0));
		dgVector support1(dir.Scale4(m_radio1));
		support0.m_x -= m_height;
		support1.m_x += m_height;
		dgFloat32 distance0 = support0.DotProduct4(dir).GetScalar();
		dgFloat32 distance1 = support1.DotProduct4(dir).GetScalar();

		if (distance1 > distance0) {
			m_p1 = support1;
		} else if (distance1 < distance0) {
			m_p0 = support0;
		} else {
			i = 1000;
		}
	}

	dgVector tempVertex[4 * DG_TAPED_CAPSULE_CAP_SEGMENTS * DG_TAPED_CAPSULE_SEGMENTS + 100];
	dgInt32 index = 0;
	dgInt32 dx0 = dgInt32(dgFloor(DG_TAPED_CAPSULE_SEGMENTS * ((m_p0.m_x + m_height + m_radio0) / m_radio0)) + dgFloat32(1.0f));
	dgFloat32 step = m_radio0 / DG_TAPED_CAPSULE_SEGMENTS;
	dgFloat32 x0 = m_p0.m_x - step * dx0;
	for (dgInt32 j = 0; j < dx0; j++) {
		x0 += step;
		dgFloat32 x = x0 + m_height;
		dgFloat32 arg = dgMax (m_radio0 * m_radio0 - x * x, dgFloat32 (1.0e-3f));
		dgFloat32 r0 = dgSqrt (arg);
		
		dgFloat32 angle = dgFloat32(0.0f);
		for (dgInt32 i = 0; i < DG_TAPED_CAPSULE_CAP_SEGMENTS; i++) {
			dgFloat32 z = dgSin(angle);
			dgFloat32 y = dgCos(angle);
			tempVertex[index] = dgVector(x0, y * r0, z * r0, dgFloat32(0.0f));
			index++;
			angle += dgPI2 / DG_TAPED_CAPSULE_CAP_SEGMENTS;
			dgAssert(index < sizeof (tempVertex) / sizeof (tempVertex[0]));
		}
	}
	
	dgFloat32 x1 = m_p1.m_x;
	dgInt32 dx1 = dgInt32 (dgFloor (DG_TAPED_CAPSULE_SEGMENTS * ((m_height + m_radio1 - m_p1.m_x) / m_radio1)) + dgFloat32 (1.0f));
	step = m_radio1 / DG_TAPED_CAPSULE_SEGMENTS;
	for (dgInt32 j = 0; j < dx1; j ++) {
		dgFloat32 x = x1 - m_height;
		dgFloat32 arg = dgMax (m_radio1 * m_radio1 - x * x, dgFloat32 (1.0e-3f));
		dgFloat32 r1 = dgSqrt (arg);
		dgFloat32 angle = dgFloat32 (0.0f);
		for (dgInt32 i = 0; i < DG_TAPED_CAPSULE_CAP_SEGMENTS; i ++) {
			dgFloat32 z = dgSin (angle);
			dgFloat32 y = dgCos (angle);
			tempVertex[index] = dgVector ( x1, y * r1, z * r1, dgFloat32 (0.0f));
			index ++;
			angle += dgPI2 / DG_TAPED_CAPSULE_CAP_SEGMENTS;
			dgAssert (index < sizeof (tempVertex) / sizeof (tempVertex[0]));
		}
		x1 += step;
	}

	m_vertexCount = dgInt16 (index);
	dgCollisionConvex::m_vertex = (dgVector*) m_allocator->Malloc (dgInt32 (m_vertexCount * sizeof (dgVector)));
	memcpy (dgCollisionConvex::m_vertex, tempVertex, m_vertexCount * sizeof (dgVector));

	dgPolyhedra polyhedra(m_allocator);
	polyhedra.BeginFace ();

	dgInt32 wireframe[DG_TAPED_CAPSULE_SEGMENTS + 10];

	dgInt32 i1 = 0;
	dgInt32 i0 = DG_TAPED_CAPSULE_CAP_SEGMENTS - 1;
	const dgInt32 n = index / DG_TAPED_CAPSULE_CAP_SEGMENTS - 1;
	for (dgInt32 j = 0; j < n; j ++) {
		for (dgInt32 i = 0; i < DG_TAPED_CAPSULE_CAP_SEGMENTS; i ++) { 
			wireframe[0] = i0;
			wireframe[1] = i1;
			wireframe[2] = i1 + DG_TAPED_CAPSULE_CAP_SEGMENTS;
			wireframe[3] = i0 + DG_TAPED_CAPSULE_CAP_SEGMENTS;
			i0 = i1;
			i1 ++;
			polyhedra.AddFace (4, wireframe);
		}
		i0 = i1 + DG_TAPED_CAPSULE_CAP_SEGMENTS - 1;
	}

	for (dgInt32 i = 0; i < DG_TAPED_CAPSULE_CAP_SEGMENTS; i ++) { 
		wireframe[i] = DG_TAPED_CAPSULE_CAP_SEGMENTS - i - 1;
	}
	polyhedra.AddFace (DG_TAPED_CAPSULE_CAP_SEGMENTS, wireframe);

	for (dgInt32 i = 0; i < DG_TAPED_CAPSULE_CAP_SEGMENTS; i ++) { 
		wireframe[i] = index - DG_TAPED_CAPSULE_CAP_SEGMENTS + i;
	}
	polyhedra.AddFace (DG_TAPED_CAPSULE_CAP_SEGMENTS, wireframe);
	polyhedra.EndFace ();

	dgAssert (SanityCheck (polyhedra));

	m_edgeCount = dgInt16 (polyhedra.GetEdgeCount());
	m_simplex = (dgConvexSimplexEdge*) m_allocator->Malloc (dgInt32 (m_edgeCount * sizeof (dgConvexSimplexEdge)));

	dgUnsigned64 i = 0;
	dgPolyhedra::Iterator iter (polyhedra);
	for (iter.Begin(); iter; iter ++) {
		dgEdge* const edge = &(*iter);
		edge->m_userData = i;
		i ++;
	}

	for (iter.Begin(); iter; iter ++) {
		dgEdge* const edge = &(*iter);

		dgConvexSimplexEdge* const ptr = &m_simplex[edge->m_userData];

		ptr->m_vertex = edge->m_incidentVertex;
		ptr->m_next = &m_simplex[edge->m_next->m_userData];
		ptr->m_prev = &m_simplex[edge->m_prev->m_userData];
		ptr->m_twin = &m_simplex[edge->m_twin->m_userData];
	}
	SetVolumeAndCG ();
}


dgInt32 dgCollisionTaperedCapsule::CalculateSignature (dgFloat32 radio0, dgFloat32 radio1, dgFloat32 height)
{
	dgUnsigned32 buffer[4];

	buffer[0] = m_taperedCapsuleCollision;
	buffer[1] = Quantize (radio0);
	buffer[2] = Quantize (radio1);
	buffer[3] = Quantize (height);
	return Quantize(buffer, sizeof (buffer));
}

dgInt32 dgCollisionTaperedCapsule::CalculateSignature () const
{
	dgAssert (0);
	return 0;
//	return CalculateSignature (m_radio0, m_radio1, m_height);
}

void dgCollisionTaperedCapsule::DebugCollision (const dgMatrix& matrix, dgCollision::OnDebugCollisionMeshCallback callback, void* const userData) const
{
	dgMatrix transform (matrix);
	transform[0] = transform[0].Scale4(m_transform.m_x);
	transform[1] = transform[1].Scale4(m_transform.m_y);
	transform[2] = transform[2].Scale4(m_transform.m_z);
	dgCollisionConvex::DebugCollision (transform, callback, userData);
}


void dgCollisionTaperedCapsule::SetCollisionBBox (const dgVector& p0__, const dgVector& p1__)
{
	dgAssert (0);
}


dgVector dgCollisionTaperedCapsule::SupportVertex (const dgVector& direction, dgInt32* const vertexIndex) const
{
	dgVector dir (direction.CompProduct4(m_transform));
	dgAssert (dgAbsf(dir % dir - dgFloat32 (1.0f)) < dgFloat32 (1.0e-3f));

	dgVector p0(dir.Scale4 (m_radio0));
	dgVector p1(dir.Scale4 (m_radio1));
	p0.m_x -= m_height;
	p1.m_x += m_height;
	dgFloat32 dir0 = p0.DotProduct4(dir).GetScalar();
	dgFloat32 dir1 = p1.DotProduct4(dir).GetScalar();
	if (dir1 > dir0) {
		p0 = p1;
	}
	return p0.CompProduct4(m_transform);
}

dgVector dgCollisionTaperedCapsule::SupportVertexSpecial(const dgVector& direction, dgInt32* const vertexIndex) const
{
	*vertexIndex = -1;
	dgVector dir(direction.CompProduct4(m_transform));
	dgAssert(dgAbsf(dir % dir - dgFloat32(1.0f)) < dgFloat32(1.0e-3f));

	dgVector p0(dgVector::m_zero);
	dgVector p1(dir.Scale4(m_radio1 - m_radio0));
	p0.m_x -= m_height;
	p1.m_x += m_height;
	dgFloat32 dir0 = p0.DotProduct4(dir).GetScalar();
	dgFloat32 dir1 = p1.DotProduct4(dir).GetScalar();
	if (dir1 > dir0) {
		p0 = p1;
	}
	return p0.CompProduct4(m_transform);
}


dgVector dgCollisionTaperedCapsule::SupportVertexSpecialProjectPoint (const dgVector& testPoint, const dgVector& direction) const
{
	dgVector dir(direction.CompProduct4(m_transform));
	dgVector point(testPoint.CompProduct4(m_transform));
	point += dir.Scale4(m_radio0);
	return m_transform.CompProduct4(point);
}


dgFloat32 dgCollisionTaperedCapsule::CalculateMassProperties (const dgMatrix& offset, dgVector& inertia, dgVector& crossInertia, dgVector& centerOfMass) const
{
	return dgCollisionConvex::CalculateMassProperties (offset, inertia, crossInertia, centerOfMass);
}

void dgCollisionTaperedCapsule::GetCollisionInfo(dgCollisionInfo* const info) const
{
	dgCollisionConvex::GetCollisionInfo(info);

	info->m_taperedCapsule.m_radio0 = m_radio0;
	info->m_taperedCapsule.m_radio1 = m_radio1;
	info->m_capsule.m_height = dgFloat32 (2.0f) * m_height;

	if (m_transform.m_x < dgFloat32 (0.0f)) {
		dgSwap(info->m_taperedCapsule.m_radio0, info->m_taperedCapsule.m_radio1);
	}
}

void dgCollisionTaperedCapsule::Serialize(dgSerialize callback, void* const userData) const
{
	SerializeLow(callback, userData);

	dgVector size(m_radio0, m_radio1, m_height, dgFloat32 (0.0f));
	callback (userData, &size, sizeof (dgVector));
	callback (userData, &m_p0, sizeof (dgVector));
	callback (userData, &m_p1, sizeof (dgVector));
	callback (userData, &m_transform, sizeof (dgVector));
}


dgInt32 dgCollisionTaperedCapsule::CalculatePlaneIntersection (const dgVector& direction, const dgVector& point, dgVector* const contactsOut, dgFloat32 normalSign) const
{
	dgVector normal(direction.CompProduct4(m_transform));
	dgVector origin(point.CompProduct4(m_transform));
	
	dgInt32 count = 0;
	dgVector p0 (-m_height, dgFloat32 (0.0f), dgFloat32 (0.0f), dgFloat32 (0.0f));
	dgVector dir0 (p0 - origin);
	dgFloat32 dist0 = dir0.DotProduct4(normal).GetScalar();
	if ((dist0 * dist0) < (m_radio0 * m_radio0)) {
		contactsOut[count] = m_transform.CompProduct4 (p0 - normal.Scale4 (dist0));
		count ++;
	}

	dgVector p1 (m_height, dgFloat32 (0.0f), dgFloat32 (0.0f), dgFloat32 (0.0f));
	dgVector dir1 (p1 - origin);
	dgFloat32 dist1 = dir1.DotProduct4(normal).GetScalar();
	if ((dist1 * dist1) < (m_radio1 * m_radio1)) {
		contactsOut[count] = m_transform.CompProduct4(p1 - normal.Scale4 (dist1));
		count ++;
	}
	return count;
}

dgFloat32 dgCollisionTaperedCapsule::RayCast (const dgVector& r0, const dgVector& r1, dgFloat32 maxT, dgContactPoint& contactOut, const dgBody* const body, void* const userData, OnRayPrecastAction preFilter) const
{
	dgVector q0(r0.CompProduct4(m_transform));
	dgVector q1(r1.CompProduct4(m_transform));

	dgVector origin0 (-m_height, dgFloat32 (0.0f), dgFloat32 (0.0f), dgFloat32 (0.0f));
	dgVector origin1 ( m_height, dgFloat32 (0.0f), dgFloat32 (0.0f), dgFloat32 (0.0f));
	dgFloat32 t0 = dgRayCastSphere (q0, q1, origin0, m_radio0);
	dgFloat32 t1 = dgRayCastSphere (q0, q1, origin1, m_radio1);
	if ((t0 < maxT) && (t1 < maxT)) {
		if (t0 < t1) {
			dgVector q (q0 + (q1 - q0).Scale4 (t0));
			dgVector n(q - origin0);
			dgAssert(n.m_w == dgFloat32(0.0f));
			contactOut.m_normal = m_transform.CompProduct4(n.CompProduct4(n.DotProduct4(n).InvSqrt()));
			return t0;
		} else {
			dgVector q (q0 + (q1 - q0).Scale4 (t1));
			dgVector n(q - origin1);
			dgAssert(n.m_w == dgFloat32(0.0f));
			contactOut.m_normal = m_transform.CompProduct4(n.CompProduct4(n.DotProduct4(n).InvSqrt()));
			return t1;
		}
	} else if (t1 < maxT) {
		dgVector q (q0 + (q1 - q0).Scale4 (t1));
		if (q.m_x >= m_p1.m_x) {
			dgVector n (q - origin1); 
			dgAssert (n.m_w == dgFloat32 (0.0f));
			contactOut.m_normal = m_transform.CompProduct4 (n.CompProduct4(n.DotProduct4(n).InvSqrt()));
			return t1;
		}
	} else if (t0 < maxT) {
		dgVector q (q0 + (q1 - q0).Scale4 (t0));
		if (q.m_x <= m_p0.m_x) {
			dgVector n (q - origin0); 
			dgAssert (n.m_w == dgFloat32 (0.0f));
			contactOut.m_normal = m_transform.CompProduct4 (n.CompProduct4(n.DotProduct4(n).InvSqrt()));
			return t0;
		}
	}

	dgFloat32 ret = dgCollisionConvex::RayCast (q0, q1, maxT, contactOut, body, NULL, NULL);
	if (ret <= dgFloat32 (1.0f)) {
		contactOut.m_normal = m_transform.CompProduct4 (contactOut.m_normal);
	}
	return ret;
}



