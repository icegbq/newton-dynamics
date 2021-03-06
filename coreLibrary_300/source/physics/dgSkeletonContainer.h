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

#ifndef _DG_SKELETON_CONTAINER_H__
#define _DG_SKELETON_CONTAINER_H__

#define DG_SKELETON_BASEW_UNIQUE_ID	10

#include "dgConstraint.h"

class dgDynamicBody;
class dgSkeletonContainer;
typedef void (dgApi *dgOnSkeletonContainerDestroyCallback) (dgSkeletonContainer* const me);

class dgSkeletonContainer
{
	public:
	class dgGraph;

	class dgNodePair
	{
		public:
		dgInt32 m_m0;
		dgInt32 m_m1;
	};

	DG_MSC_VECTOR_ALIGMENT
	class dgForcePair
	{
		public:
		dgSpatialVector m_joint;
		dgSpatialVector m_body;
	} DG_GCC_VECTOR_ALIGMENT;

	DG_MSC_VECTOR_ALIGMENT class dgMatriData
	{
		public:
		dgSpatialMatrix m_jt;
		dgSpatialMatrix m_mass;
		dgSpatialMatrix m_invMass;
	} DG_GCC_VECTOR_ALIGMENT;

	DG_MSC_VECTOR_ALIGMENT class dgBodyJointMatrixDataPair
	{
		public:
		dgMatriData m_body;
		dgMatriData m_joint;
	} DG_GCC_VECTOR_ALIGMENT;

	DG_CLASS_ALLOCATOR(allocator)
	dgSkeletonContainer(dgWorld* const world, dgDynamicBody* const rootBody);
	~dgSkeletonContainer();

	void Finalize ();
	dgWorld* GetWorld() const; 
	dgInt32 GetId () const {return m_id;}
	dgInt32 GetJointCount () const {return m_nodeCount - 1;}
	dgGraph* AddChild (dgBody* const child, dgBody* const parent);
	void AddJointList (dgInt32 count, dgBilateralConstraint** const array);
	void SetDestructorCallback (dgOnSkeletonContainerDestroyCallback destructor);
	
	void CalculateJointForce (dgJointInfo* const jointInfoArray, const dgBodyInfo* const bodyArray, dgJacobian* const internalForces, dgJacobianMatrixElement* const matrixRow);
	
	dgGraph* GetRoot () const;
	dgBody* GetBody(dgGraph* const node) const;
	dgBilateralConstraint* GetParentJoint(dgGraph* const node) const;
	dgGraph* GetParent (dgGraph* const node) const;
	dgGraph* GetFirstChild (dgGraph* const parent) const;
	dgGraph* GetNextSiblingChild (dgGraph* const sibling) const;

	private:
	DG_INLINE void SolveBackward (dgForcePair* const force, const dgBodyJointMatrixDataPair* const data) const;
	DG_INLINE void SolveFoward (dgForcePair* const force, const dgForcePair* const accel, const dgBodyJointMatrixDataPair* const data) const;
	DG_INLINE void UpdateForces (dgJointInfo* const jointInfoArray, dgJacobian* const internalForces, dgJacobianMatrixElement* const matrixRow, const dgForcePair* const force) const;
	DG_INLINE void InitMassMatrix (const dgJointInfo* const jointInfoArray, dgJacobianMatrixElement* const matrixRow, dgBodyJointMatrixDataPair* const data);
	DG_INLINE void CalculateJointAccel (dgJointInfo* const jointInfoArray, const dgJacobian* const internalForces, dgJacobianMatrixElement* const matrixRow, dgForcePair* const force) const;
	void BuildAuxiliaryMassMatrix (const dgJointInfo* const jointInfoArray, const dgJacobian* const internalForces, const dgJacobianMatrixElement* const matrixRow, const dgBodyJointMatrixDataPair* const data, const dgForcePair* const accel, dgForcePair* const force) const;
	void BruteForceSolve (const dgJointInfo* const jointInfoArray, dgJacobian* const internalForces, dgJacobianMatrixElement* const matrixRow, const dgBodyJointMatrixDataPair* const data, const dgForcePair* const accel, dgForcePair* const force) const;

	dgGraph* FindNode (dgDynamicBody* const node) const;
	dgGraph* AddChild (dgDynamicBody* const child, dgDynamicBody* const parent);
	void SortGraph (dgGraph* const root, dgGraph* const parent, dgInt32& index);
	
	static void ResetUniqueId(dgInt32 id);

	dgWorld* m_world;
	dgGraph* m_skeleton;
	dgGraph** m_nodesOrder;
	dgOnSkeletonContainerDestroyCallback m_destructor;
	dgInt32 m_id;
	dgInt32 m_lru;
	dgInt16 m_nodeCount;
	dgInt16 m_rowCount;
	dgInt16 m_auxiliaryRowCount;
	static dgInt32 m_uniqueID;
	static dgInt32 m_lruMarker;

	friend class dgWorld;
	friend class dgWorldDynamicUpdate;
};

#endif

