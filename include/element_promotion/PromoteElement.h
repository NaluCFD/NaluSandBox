/*------------------------------------------------------------------------*/
/*  Copyright 2014 Sandia Corporation.                                    */
/*  This software is released under the license detailed                  */
/*  in the file, LICENSE, which is located in the top-level NaluUnit      */
/*  directory structure                                                   */
/*------------------------------------------------------------------------*/
#ifndef PromoteElement_h
#define PromoteElement_h

#include <element_promotion/ElementDescription.h>
#include <stk_mesh/base/CoordinateSystems.hpp>

#include <stk_mesh/base/Entity.hpp>
#include <stk_mesh/base/Field.hpp>
#include <stk_mesh/base/Selector.hpp>
#include <stk_mesh/base/Types.hpp>

#include <boost/functional/hash/hash.hpp>

#include <array>
#include <cstddef>
#include <iosfwd>
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

    template<class T>
    constexpr T ipow(const T base, unsigned const exponent)
    {
      return (exponent == 0) ? 1 : (base * ipow(base, exponent-1));
    }


class PromoteElement
{
public:
  explicit PromoteElement(ElementDescription& elemDescription);
  ~PromoteElement() {};

  void promote_elements(
    const stk::mesh::PartVector& old_parts,
    VectorFieldType& coordinates,
    stk::mesh::BulkData& mesh,
    stk::mesh::PartVector& new_parts
  );

  size_t num_sub_elements(
    const stk::mesh::MetaData& metaData,
    const stk::mesh::BucketVector& buckets) const;
  size_t count_entities(const stk::mesh::BucketVector& buckets) const;

  const ElementDescription& element_description() const { return elemDescription_; };
  stk::mesh::PartVector base_part_vector() const { return baseParts_; };
  stk::mesh::PartVector promoted_part_vector() const { return promotedParts_; };
  unsigned nodes_per_element() const { return nodesPerElement_; };
  unsigned added_nodes_per_element() const { return (elemDescription_.addedConnectivities.size()); };
  unsigned base_nodes_per_element() const { return (nodes_per_element()-added_nodes_per_element()); };

  stk::mesh::Entity const* begin_nodes_all(const stk::mesh::Bucket& bucket, stk::mesh::EntityId id) const;
  stk::mesh::Entity const* begin_nodes_all(const stk::mesh::Entity& elem) const;
  stk::mesh::Entity const* begin_side_nodes_all(const stk::mesh::Bucket& bucket, stk::mesh::EntityId id) const;
  stk::mesh::Entity const* begin_side_nodes_all(const stk::mesh::Entity& face) const;
  stk::mesh::Entity const* begin_elems_all(const stk::mesh::Bucket& bucket, stk::mesh::EntityId id) const;
  stk::mesh::Entity const* begin_elems_all(const stk::mesh::Entity& node) const;
  stk::mesh::Entity const* begin_side_elems_all(const stk::mesh::Bucket& bucket, stk::mesh::EntityId id) const;
  stk::mesh::Entity const* begin_side_elems_all(const stk::mesh::Entity& face) const;

  size_t num_elems(const stk::mesh::Entity& node) const;
  size_t num_nodes(const stk::mesh::Entity& elem) const;
private:
  class ChildNodeRequest
  {
  public:
    explicit ChildNodeRequest(const std::vector<stk::mesh::EntityId>& in_parentIds);
    ~ChildNodeRequest() = default;
    ChildNodeRequest(ChildNodeRequest&&) = default;
    ChildNodeRequest& operator=(ChildNodeRequest&&) = default;
    ChildNodeRequest(const ChildNodeRequest&) = delete;
    ChildNodeRequest& operator=(const ChildNodeRequest&) = delete;

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

    bool determine_child_node_ordinal(
      const stk::mesh::BulkData& mesh,
      const ElementDescription& elemDesc,
      unsigned elemNumber) const;

    void set_num_children(size_t num) const
    {
      children_.resize(num);
      idProcPairsFromAllProcs_.resize(num);
    }

    size_t num_children() const { return children_.size(); }
    size_t num_parents() const { return parentIds_.size(); }

    void add_proc_id_pair(int proc_id, stk::mesh::EntityId id, int childNumber) const;
    stk::mesh::EntityId suggested_node_id(int childNumber) const;
    stk::mesh::EntityId get_id_for_child(int childNumber) const;

    const std::vector<stk::mesh::EntityId> parentIds_; // invariant
    mutable std::vector<stk::mesh::Entity> children_;
    mutable std::vector<std::vector<size_t>> childOrdinalsForElem_;
    mutable std::vector<stk::mesh::Entity> sharedElems_;
    mutable std::vector<int> sharingProcs_;

    using IdProcPair = std::pair<int,stk::mesh::EntityId>;
    mutable std::vector<std::vector<IdProcPair>> idProcPairsFromAllProcs_;
  };

  struct EntityHash
  {
    std::size_t operator()(const stk::mesh::Entity& entity) const
    {
      return entity.local_offset();
    }
  };

  struct RequestHash
  {
    std::size_t operator()(const ChildNodeRequest& request) const
    {
      return boost::hash_range(request.parentIds_.begin(), request.parentIds_.end());
    }
  };
  using NodeRequests = std::unordered_set<ChildNodeRequest, RequestHash>;
  using ElemRelationsMap =
      std::unordered_map<
      stk::mesh::Entity, std::vector<stk::mesh::Entity>,
      EntityHash>;

  bool check_elem_node_relations(const stk::mesh::BulkData& mesh) const;

  template<unsigned embedding_dimension, unsigned dimension>
  void interpolate_coords(
    const double* par_coord,
    const std::array<double, embedding_dimension*ipow(2,dimension)>& coords,
    double* interpolatedCoords
  ) const;

  template<unsigned dimension>
  void set_new_node_coords(
    VectorFieldType& coordinates,
    const ElementDescription& elemDescription,
    const stk::mesh::BulkData& mesh,
    NodeRequests& requests
  ) const;

  template<unsigned embedding_dimension, unsigned dimension> void
  set_coords_for_child(
    VectorFieldType& coordinates,
    const stk::mesh::Entity* node_rels,
    std::vector<size_t>& childOrdinal,
    const std::vector<size_t>& parentNodeOrdinals,
    const std::vector<std::vector<double>>& isoParCoords
  ) const;

  NodeRequests create_child_node_requests(
    stk::mesh::BulkData& mesh,
    const ElementDescription& elemDescription,
    const stk::mesh::Selector& selector
  ) const;

  void batch_create_child_nodes(
    stk::mesh::BulkData & mesh,
    NodeRequests& child_node_requests,
    const stk::mesh::PartVector& node_parts
  ) const;

  void parallel_communicate_ids(
    const stk::mesh::BulkData& mesh,
    NodeRequests& requests) const;

  void populate_elem_node_relations(
    const ElementDescription& elemDescription,
    stk::mesh::BulkData& mesh,
    const stk::mesh::Selector selector,
    const NodeRequests& child_node_requests
  );

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

  ElemRelationsMap elemNodeMap_;
  ElemRelationsMap nodeElemMap_;

  ElemRelationsMap elemNodeMapBC_;
  ElemRelationsMap nodeElemMapBC_;
  std::vector<stk::mesh::Entity> elemNodes_;
};

} // namespace naluUnit
} // namespace Sierra

#endif
