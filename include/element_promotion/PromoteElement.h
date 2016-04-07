/*------------------------------------------------------------------------*/
/*  Copyright 2014 Sandia Corporation.                                    */
/*  This software is released under the license detailed                  */
/*  in the file, LICENSE, which is located in the top-level NaluUnit      */
/*  directory structure                                                   */
/*------------------------------------------------------------------------*/
#ifndef PromoteElement_h
#define PromoteElement_h

#include <element_promotion/ElementDescription.h>
#include <element_promotion/FaceOperations.h>

#include <stk_mesh/base/CoordinateSystems.hpp>

#include <stk_mesh/base/Entity.hpp>
#include <stk_mesh/base/Field.hpp>
#include <stk_mesh/base/Selector.hpp>
#include <stk_mesh/base/Types.hpp>
#include <stk_mesh/base/HashEntityAndEntityKey.hpp>

#include <boost/functional/hash/hash.hpp>

#include <array>
#include <cstddef>
#include <iosfwd>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace stk {
namespace mesh {
class Bucket;
class BulkData;
class MetaData;
}  // namespace mesh
}  // namespace stk

// field types
typedef stk::mesh::Field<double, stk::mesh::Cartesian>  VectorFieldType;

namespace sierra {
namespace naluUnit {

class PromoteElement
{
public:
  explicit PromoteElement(ElementDescription& elemDescription);
  ~PromoteElement() {};

  void promote_elements(
    const stk::mesh::PartVector& baseParts,
    VectorFieldType& coordinates,
    stk::mesh::BulkData& mesh
  );

  void populate_boundary_connectivity_map_using_super_elems(
    const stk::mesh::BulkData& mesh,
    const stk::mesh::PartVector& mesh_parts
  );

  const ElementDescription& element_description() const { return elemDescription_; };
  stk::mesh::PartVector base_part_vector() const { return baseParts_; };
  stk::mesh::PartVector promoted_part_vector() const { return promotedParts_; };
  unsigned nodes_per_element() const { return nodesPerElement_; };
  unsigned added_nodes_per_element() const { return (elemDescription_.addedConnectivities.size()); };
  unsigned base_nodes_per_element() const { return (nodes_per_element()-added_nodes_per_element()); };

  stk::mesh::Entity const* begin_side_nodes_all(const stk::mesh::Bucket& bucket, stk::mesh::EntityId id) const
  {
    return (elemNodeMapBC_.at(bucket[id]).data());
  }
  stk::mesh::Entity const* begin_side_nodes_all(const stk::mesh::Entity& elem) const
  {
    return (elemNodeMapBC_.at(elem).data());
  }
  stk::mesh::Entity const* begin_elems_all(const stk::mesh::Bucket& bucket, stk::mesh::EntityId id) const
  {
    return (nodeElemMap_.at(bucket[id]).data());
  }
  stk::mesh::Entity const* begin_elems_all(const stk::mesh::Entity& node) const
  {
    return (nodeElemMap_.at(node).data());
  }
  stk::mesh::Entity const* begin_side_elems_all(const stk::mesh::Bucket& bucket, stk::mesh::EntityId id) const
  {
    return (nodeElemMapBC_.at(bucket[id]).data());
  }
  stk::mesh::Entity const* begin_side_elems_all(const stk::mesh::Entity& elem) const
  {
    return (nodeElemMapBC_.at(elem).data());
  }

  size_t num_elems(const stk::mesh::Entity& node) const
  {
    return (nodeElemMap_.at(node).size());
  }

  size_t num_side_elems(const stk::mesh::Entity& node) const
  {
    return (nodeElemMapBC_.at(node).size());
  }

  size_t num_side_nodes(const stk::mesh::Entity& elem) const
  {
    return (elemNodeMapBC_.at(elem).size());
  }

private:
  class ChildNodeRequest
  {
  public:
    explicit ChildNodeRequest(std::vector<stk::mesh::EntityId>  in_parentIds);
    ~ChildNodeRequest() = default;

    bool operator==(const ChildNodeRequest& other) const
    {
      return parentIds_ == other.parentIds_;
    }

    void determine_sharing_procs(const stk::mesh::BulkData& mesh) const;
    void set_node_entity_for_request(
      stk::mesh::BulkData& mesh,
      const stk::mesh::PartVector& node_parts
    ) const;
    void add_shared_elem(const stk::mesh::Entity& elem) const;

    std::vector<size_t> determine_child_node_ordinals(
      const stk::mesh::BulkData& mesh,
      const ElementDescription& elemDesc,
      unsigned elemNumber) const;

    void set_num_children(size_t num) const
    {
      children_.resize(num);
      idProcPairsFromAllProcs_.resize(num);
    }

    stk::mesh::PartVector mesh_parts_for_child_nodes(
      const stk::mesh::BulkData& mesh,
      stk::mesh::PartVector base_parts) const;

    size_t num_children() const { return children_.size(); }
    size_t num_parents() const { return parentIds_.size(); }

    void add_proc_id_pair(int proc_id, stk::mesh::EntityId id, int childNumber) const;
    stk::mesh::EntityId suggested_node_id(int childNumber) const;
    stk::mesh::EntityId get_id_for_child(int childNumber) const;

    std::vector<stk::mesh::EntityId> parentIds_; // invariant
    mutable std::vector<stk::mesh::EntityId> unsortedParentIds_;
    mutable std::vector<stk::mesh::Entity> children_;
    mutable std::vector<std::vector<size_t>> childOrdinalsForElem_;
    mutable std::vector<std::vector<size_t>> reorderedChildOrdinalsForElem_;
    mutable std::vector<stk::mesh::Entity> sharedElems_;
    mutable std::vector<int> sharingProcs_;

    using IdProcPair = std::pair<int,stk::mesh::EntityId>;
    mutable std::vector<std::vector<IdProcPair>> idProcPairsFromAllProcs_;
  };

  struct RequestHash
  {
    std::size_t operator()(const ChildNodeRequest& request) const
    {
      // Order-sensitive hash function hash({1,2}) != hash({2,1})
      return boost::hash_range(request.parentIds_.begin(), request.parentIds_.end());
    }
  };

  struct EntityVecIdHash
  {
    std::size_t operator()(const std::vector<stk::mesh::EntityId>& ids) const
    {
      // Order-sensitive hash function hash({1,2}) != hash({2,1})
      return boost::hash_range(ids.begin(), ids.end());
    }
  };


  using NodeRequests = std::unordered_set<ChildNodeRequest, RequestHash>;

  using ElemRelationsMap =
      std::unordered_map< stk::mesh::Entity,
                          std::vector<stk::mesh::Entity> >;

  using NodesElemMap = std::unordered_map< std::vector<stk::mesh::EntityId>,
                                           stk::mesh::Entity,
                                           EntityVecIdHash >;

  using ExposedFaceElemMap = std::unordered_map<stk::mesh::Entity, stk::mesh::Entity>;

  bool check_elem_node_relations(
    const stk::mesh::BulkData& mesh,
    ElemRelationsMap& elemNodeMap
  ) const;

  template<typename T> std::vector<T>
  reorder_ordinals(
     const std::vector<T>& ordinals,
     const std::vector<T>& unsortedOrdinals,
     const std::vector<T>& canonicalOrdinals,
     unsigned numParents1D,
     unsigned numAddedNodes1D
   ) const;

  template<unsigned dimension>
  void set_new_node_coords(
    VectorFieldType& coordinates,
    const ElementDescription& elemDescription,
    const stk::mesh::BulkData& mesh,
    const NodeRequests& requests,
    const ElemRelationsMap& elemNodeMap) const;

  template<unsigned embedding_dimension, unsigned dimension> void
  set_coords_for_child(
    const stk::mesh::BulkData& mesh,
    VectorFieldType& coordinates,
    const stk::mesh::Entity* node_rels,
    const std::vector<size_t>& childOrdinals,
    const std::vector<stk::mesh::EntityId>& parentNodeIds,
    const std::vector<std::vector<double>>& isoParCoords) const;

  template<unsigned embedding_dimension, unsigned dimension>
  void interpolate_coords(
    const std::vector<double>& isoParCoord,
    const std::array<double, embedding_dimension*ipow(2,dimension)>& parentCoords,
    double* interpolatedCoords
  ) const;

  NodeRequests create_child_node_requests(
    const ElementDescription& elemDescription,
    stk::mesh::BulkData& mesh,
    const stk::mesh::Selector& selector
  ) const;

  void determine_child_ordinals(
    const ElementDescription& elemDescription,
    const stk::mesh::BulkData& mesh,
    NodeRequests& requests) const;

  void batch_create_child_nodes(
    const ElementDescription& elemDescription,
    stk::mesh::BulkData & mesh,
    NodeRequests& requests,
    const stk::mesh::PartVector& node_parts
  ) const;

  void parallel_communicate_ids(
    const ElementDescription& elemDescription,
    const stk::mesh::BulkData& mesh,
    NodeRequests& requests) const;

  void populate_elem_node_relations(
    const ElementDescription& elemDescription,
    stk::mesh::BulkData& mesh,
    const stk::mesh::Selector& selector,
    const NodeRequests& requests,
    ElemRelationsMap& elemNodeMap
  );

  void populate_original_elem_node_relations(
    const stk::mesh::BulkData& mesh,
    const stk::mesh::Selector& selector,
    const NodeRequests& requests,
    ElemRelationsMap& elemNodeMap
  );

  void populate_new_elem_node_relations(
    const NodeRequests& requests,
    ElemRelationsMap& elemNodeMap
  );

  void populate_boundary_elem_node_relations(
    const ElementDescription& elemDescription,
    const stk::mesh::BulkData& mesh,
    const stk::mesh::Selector& selector,
    ElemRelationsMap& elemNodeMap
  );

  void create_elements(
    stk::mesh::BulkData& mesh,
    const stk::mesh::PartVector& baseElemParts,
    ElemRelationsMap& elemNodeMap) const;

  NodesElemMap make_base_nodes_to_elem_map_at_boundary(
    const ElementDescription& elemDesc,
    const stk::mesh::BulkData& mesh,
    const stk::mesh::PartVector& mesh_parts
  ) const;

  void
  populate_exposed_face_to_super_elem_map(
    const ElementDescription& elemDesc,
    const stk::mesh::BulkData& mesh,
    const stk::mesh::PartVector& mesh_parts,
    const stk::mesh::PartVector& superElemParts);


  size_t count_nodes(
    const stk::mesh::PartVector& baseParts,
    const stk::mesh::PartVector& promotedParts) const;

  size_t count_requested_nodes(const NodeRequests& requests) const;

  const ElementDescription& elemDescription_;
  const unsigned nodesPerElement_;
  const unsigned dimension_;

  //local copy
  stk::mesh::PartVector baseParts_;
  stk::mesh::PartVector promotedParts_;

  ElemRelationsMap nodeElemMap_;

  ElemRelationsMap elemNodeMapBC_;
  ElemRelationsMap nodeElemMapBC_;
  ExposedFaceElemMap exposedFaceToSuperElemMap_;
};

} // namespace naluUnit
} // namespace Sierra

#endif
