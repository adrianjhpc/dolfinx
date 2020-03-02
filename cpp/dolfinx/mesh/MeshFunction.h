// Copyright (C) 2006-2009 Anders Logg
//
// This file is part of DOLFINX (https://www.fenicsproject.org)
//
// SPDX-License-Identifier:    LGPL-3.0-or-later

#pragma once

#include "Geometry.h"
#include "Mesh.h"
#include "Topology.h"
#include <dolfinx/common/IndexMap.h>
#include <dolfinx/common/MPI.h>
#include <dolfinx/common/UniqueIdGenerator.h>
#include <dolfinx/graph/AdjacencyList.h>
#include <map>
#include <memory>
#include <unordered_set>

namespace dolfinx
{
namespace mesh
{

template <typename T>
class MeshValueCollection;

/// A MeshFunction is a function that can be evaluated at a set of mesh
/// entities. A MeshFunction is discrete and is only defined at the set
/// of mesh entities of a fixed topological dimension.  A MeshFunction
/// may for example be used to store a global numbering scheme for the
/// entities of a (parallel) mesh, marking sub domains or boolean
/// markers for mesh refinement.
/// @tparam Type
template <typename T>
class MeshFunction
{
public:
  /// Create MeshFunction of given dimension on given mesh and
  /// initialize to a value
  /// @param[in] mesh The mesh to create mesh function on
  /// @param[in] dim (std::size_t) The mesh entity dimension
  /// @param[in] value The initial value of the MeshFunction
  MeshFunction(std::shared_ptr<const Mesh> mesh, std::size_t dim,
               const T& value);

  /// Create mesh function from a MeshValueCollection
  /// @param[in] mesh The mesh to create mesh function on
  /// @param[in] value_collection The mesh value collection for the mesh
  ///                             function data.
  /// @param[in] default_value The default value if unset in
  ///                          value_collection
  MeshFunction(std::shared_ptr<const Mesh> mesh,
               const MeshValueCollection<T>& value_collection,
               const T& default_value);

  /// Copy constructor
  /// @param[in] f The object to be copied
  MeshFunction(const MeshFunction<T>& f) = default;

  /// Move constructor
  /// @param f The object to be moved
  MeshFunction(MeshFunction<T>&& f) = default;

  /// Destructor
  ~MeshFunction() = default;

  /// Assign MeshFunction to other mesh function
  /// @param[in] f A MeshFunction object to assign to another
  ///              MeshFunction
  MeshFunction<T>& operator=(const MeshFunction<T>& f) = default;

  /// Return mesh associated with mesh function
  /// @return The mesh
  std::shared_ptr<const Mesh> mesh() const;

  /// Return topological dimension
  /// @return The dimension.
  int dim() const;

  /// Return array of values (const. version)
  /// @return The mesh function values
  const Eigen::Array<T, Eigen::Dynamic, 1>& values() const;

  /// Return array of values
  /// @return The mesh function values
  Eigen::Array<T, Eigen::Dynamic, 1>& values();

  /// Marking function used to identify mesh entities
  using marking_function = std::function<Eigen::Array<bool, Eigen::Dynamic, 1>(
      const Eigen::Ref<
          const Eigen::Array<double, 3, Eigen::Dynamic, Eigen::RowMajor>>& x)>;

  /// Set values. If all vertices of a mesh entity satisfy the marking
  /// function then the entity is marked with the given value.
  /// @param[in] mark Marking function used to identify which mesh
  ///                 entities to set value to.
  /// @param[in] value The value to set for marked mesh entities
  void mark(const marking_function& mark, T value);

  /// Name
  std::string name = "m";

  /// Unique ID
  const std::size_t id = common::UniqueIdGenerator::id();

private:
  // Underlying data array
  Eigen::Array<T, Eigen::Dynamic, 1> _values;

  // The mesh
  std::shared_ptr<const Mesh> _mesh;

  // Topological dimension
  int _dim;
};

//---------------------------------------------------------------------------
// Implementation of MeshFunction
//---------------------------------------------------------------------------
template <typename T>
MeshFunction<T>::MeshFunction(std::shared_ptr<const Mesh> mesh, std::size_t dim,
                              const T& value)
    : _mesh(mesh), _dim(dim)
{
  assert(mesh);
  mesh->create_entities(dim);
  _values.resize(mesh->num_entities(dim));
  _values = value;
}
//---------------------------------------------------------------------------
template <typename T>
MeshFunction<T>::MeshFunction(std::shared_ptr<const Mesh> mesh,
                              const MeshValueCollection<T>& value_collection,
                              const T& default_value)
    : _mesh(mesh), _dim(value_collection.dim())
{
  assert(_mesh);
  _mesh->create_entities(_dim);

  // Initialise values with default
  auto map = _mesh->topology().index_map(_dim);
  _values.resize(map->size_local() + map->num_ghosts());
  _values = default_value;

  // Get mesh connectivity D --> d
  const std::size_t d = _dim;
  const std::size_t D = _mesh->topology().dim();
  assert(d <= D);

  // Generate connectivity if it does not exist
  _mesh->create_connectivity(D, d);
  assert(_mesh->topology().connectivity(D, d));
  const graph::AdjacencyList<std::int32_t>& connectivity
      = *_mesh->topology().connectivity(D, d);

  // Iterate over all values
  std::unordered_set<std::size_t> entities_values_set;
  const std::map<std::pair<std::size_t, std::size_t>, T>& values
      = value_collection.values();
  for (auto it = values.begin(); it != values.end(); ++it)
  {
    // Get value collection entry data
    const std::size_t cell_index = it->first.first;
    const std::size_t local_entity = it->first.second;
    const T& value = it->second;

    std::size_t entity_index = 0;
    if (d != D)
    {
      // Get global (local to to process) entity index
      assert(cell_index < _mesh->num_entities(D));
      entity_index = connectivity.links(cell_index)[local_entity];
    }
    else
    {
      entity_index = cell_index;
      assert(local_entity == 0);
    }

    // Set value for entity
    assert(entity_index < _values.size());
    _values[entity_index] = value;

    // Add entity index to set (used to check that all values are set)
    entities_values_set.insert(entity_index);
  }
}
//---------------------------------------------------------------------------
template <typename T>
std::shared_ptr<const Mesh> MeshFunction<T>::mesh() const
{
  assert(_mesh);
  return _mesh;
}
//---------------------------------------------------------------------------
template <typename T>
int MeshFunction<T>::dim() const
{
  return _dim;
}
//---------------------------------------------------------------------------
template <typename T>
const Eigen::Array<T, Eigen::Dynamic, 1>& MeshFunction<T>::values() const
{
  return _values;
}
//---------------------------------------------------------------------------
template <typename T>
Eigen::Array<T, Eigen::Dynamic, 1>& MeshFunction<T>::values()
{
  return _values;
}
//---------------------------------------------------------------------------
template <typename T>
void MeshFunction<T>::mark(
    const std::function<Eigen::Array<bool, Eigen::Dynamic, 1>(
        const Eigen::Ref<const Eigen::Array<double, 3, Eigen::Dynamic,
                                            Eigen::RowMajor>>& x)>& mark,
    T value)
{
  assert(_mesh);
  const int tdim = _mesh->topology().dim();

  // FIXME: This should be at vertices only
  // Get all nodes of the mesh and  evaluate the marker function at each
  // node
  const Eigen::Array<double, 3, Eigen::Dynamic, Eigen::RowMajor>& x
      = _mesh->geometry().x().transpose();
  Eigen::Array<bool, Eigen::Dynamic, 1> marked = mark(x);

  // Get connectivities
  auto e_to_v = _mesh->topology().connectivity(_dim, 0);
  assert(e_to_v);
  auto c_to_v = _mesh->topology().connectivity(tdim, 0);
  assert(c_to_v);

  // Get geometry dofmap
  const graph::AdjacencyList<std::int32_t>& x_dofmap
      = _mesh->geometry().dofmap();

  // Build map from vertex -> geometry dof
  auto map_v = _mesh->topology().index_map(0);
  assert(map_v);
  std::vector<std::int32_t> vertex_to_x(map_v->size_local()
                                        + map_v->num_ghosts());
  auto map_c = _mesh->topology().index_map(tdim);
  assert(map_c);
  for (int c = 0; c < map_c->size_local() + map_c->num_ghosts(); ++c)
  {
    auto vertices = c_to_v->links(c);
    auto dofs = x_dofmap.links(c);
    for (int i = 0; i < vertices.rows(); ++i)
    {
      // FIXME: We are making an assumption here on the
      // ElementDofLayout. We should use an ElementDofLayout to map
      // between local vertex index an x dof index.
      vertex_to_x[vertices[i]] = dofs(i);
    }
  }

  // Iterate over all mesh entities of the dimension of this
  // MeshFunction
  auto map = _mesh->topology().index_map(_dim);
  assert(map);
  const int num_entities = map->size_local() + map->num_ghosts();
  for (int e = 0; e < num_entities; ++e)
  {
    // By default, assume maker is 'true' at all vertices of this entity
    bool all_marked = true;

    // Iterate over all vertices of this mesh entity
    auto vertices = e_to_v->links(e);
    for (int i = 0; i < vertices.rows(); ++i)
    {
      const std::int32_t idx = vertex_to_x[vertices[i]];
      all_marked = (marked[idx] and all_marked);
    }

    // If all vertices belonging to this mesh entity are marked, then
    // mark this mesh entity
    if (all_marked)
      _values[e] = value;
  }
}
//---------------------------------------------------------------------------

} // namespace mesh
} // namespace dolfinx
