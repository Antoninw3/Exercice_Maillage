#pragma once

#include <OpenMesh/Core/IO/MeshIO.hh>
#include <OpenMesh/Core/Mesh/PolyMesh_ArrayKernelT.hh>

typedef OpenMesh::PolyMesh_ArrayKernelT<> Maillage;
typedef Maillage::VertexHandle Sommet;
typedef Maillage::FaceHandle Face;
typedef Maillage::Point Point;
