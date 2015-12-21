/*------------------------------------------------------------------------*/
/*  Copyright 2014 Sandia Corporation.                                    */
/*  This software is released under the license detailed                  */
/*  in the file, LICENSE, which is located in the top-level Nalu          */
/*  directory structure                                                   */
/*------------------------------------------------------------------------*/


#ifndef MasterElement_h
#define MasterElement_h

#include <cstdlib>
#include <stdexcept>
#include <vector>

namespace sierra{
namespace naluUnit{

namespace Jacobian{
enum Direction
{
  S_DIRECTION = 0,
  T_DIRECTION = 1,
  U_DIRECTION = 2
};
}

class MasterElement
{
public:

  MasterElement();
  virtual ~MasterElement();

  virtual void determinant(
    const int nelem,
    const double *coords,
    double *volume,
    double * error ) {
    throw std::runtime_error("determinant not implemented");}

  virtual void grad_op(
    const int nelem,
    const double *coords,
    double *gradop,
    double *deriv,
    double *det_j,
    double * error ) {
    throw std::runtime_error("grad_op not implemented");}

  virtual void shifted_grad_op(
    const int nelem,
    const double *coords,
    double *gradop,
    double *deriv,
    double *det_j,
    double * error ) {
    throw std::runtime_error("grad_op not implemented");}

  virtual void nodal_grad_op(
    const int nelem,
    double *deriv,
    double * error ) {
    throw std::runtime_error("nodal_grad_op not implemented");}

  virtual void face_grad_op(
    const int nelem,
    const int face_ordinal,
    const double *coords,
    double *gradop,
    double *det_j,
    double * error ) {
    throw std::runtime_error("face_grad_op not implemented; avoid this element type at open bcs, walls and symms");}
  
  virtual const int * adjacentNodes() {
    throw std::runtime_error("adjacentNodes not implementedunknown bc");
    return NULL;}

  virtual const int * ipNodeMap(int ordinal = 0) {
      throw std::runtime_error("ipNodeMap not implemented");
      return NULL;}

  virtual void shape_fcn(
    double *shpfc) {
    throw std::runtime_error("shape_fcn not implemented"); }

  virtual void shifted_shape_fcn(
    double *shpfc) {
    throw std::runtime_error("shifted_shape_fcn not implemented"); }

  virtual int opposingNodes(
    const int ordinal, const int node) {
    throw std::runtime_error("adjacentNodes not implemented"); }

  virtual int opposingFace(
    const int ordinal, const int node) {
    throw std::runtime_error("opposingFace not implemented"); 
    return 0; }

  virtual double isInElement(
    const double *elemNodalCoord,
    const double *pointCoord,
    double *isoParCoord) {
    throw std::runtime_error("isInElement not implemented"); 
    return 1.0e6; }

  virtual void interpolatePoint(
    const int &nComp,
    const double *isoParCoord,
    const double *field,
    double *result) {
    throw std::runtime_error("interpolatePoint not implemented"); }
  
  virtual void general_shape_fcn(
    const int numIp,
    const double *isoParCoord,
    double *shpfc) {
    throw std::runtime_error("general_shape_fcn not implement"); }

  virtual void general_face_grad_op(
    const int face_ordinal,
    const double *isoParCoord,
    const double *coords,
    double *gradop,
    double *det_j,
    double * error ) {
    throw std::runtime_error("general_face_grad_op not implemented");}

  virtual void sidePcoords_to_elemPcoords(
    const int & side_ordinal,
    const int & npoints,
    const double *side_pcoords,
    double *elem_pcoords) {
    throw std::runtime_error("sidePcoords_to_elemPcoords");}

  virtual const int * faceNodeOnExtrudedElem() {
    throw std::runtime_error("faceNodeOnExtrudedElem not implement"); }

  virtual const int * opposingNodeOnExtrudedElem() {
    throw std::runtime_error("opposingNodeOnExtrudedElem not implement"); }

  virtual const int * faceScsIpOnExtrudedElem() {
    throw std::runtime_error("faceScsIpOnExtrudedElem not implement"); }

  virtual const int * faceScsIpOnFaceEdges() {
    throw std::runtime_error("faceScsIpOnFaceEdges not implement"); }

  virtual const double * edgeAlignedArea() {
    throw std::runtime_error("edgeAlignedArea not implement"); }

  double isoparametric_mapping(const double b, const double a, const double xi) const;

  int nDim_;
  int nodesPerElement_;
  int numIntPoints_;
  double scaleToStandardIsoFac_;

  std::vector<int> lrscv_;
  std::vector<int> ipNodeMap_;
  std::vector<int> oppNode_;
  std::vector<int> oppFace_;
  std::vector<double> intgLoc_;
  std::vector<double> intgLocShift_;
  std::vector<double> intgExpFace_;
  std::vector<double> nodeLoc_;
  // extrusion-based scheme
  std::vector<int> faceNodeOnExtrudedElem_;
  std::vector<int> opposingNodeOnExtrudedElem_;
  std::vector<int> faceScsIpOnExtrudedElem_;
  std::vector<int> faceScsIpOnFaceEdges_;
  std::vector<double> edgeAlignedArea_;
  
};

class HexahedralP2Element : public MasterElement
{
public:
  HexahedralP2Element();
  virtual ~HexahedralP2Element() {}

  void shape_fcn(double *shpfc);
  void shifted_shape_fcn(double *shpfc)  { throw std::runtime_error("shifted shape functions not implemented"); };

protected:
  struct ContourData {
    Jacobian::Direction direction;
    double weight;
  };

  int tensor_product_node_map(int i, int j, int k) const;

  double gauss_point_location(
    int nodeOrdinal,
    int gaussPointOrdinal) const;

  double tensor_product_weight(
    int s1Node, int s2Node, int s3Node,
    int s1Ip, int s2Ip, int s3Ip) const;

  double tensor_product_weight(
    int s1Node, int s2Node,
    int s1Ip, int s2Ip) const;

  virtual void eval_shape_functions_at_ips();

  virtual void eval_shape_derivs_at_ips();

  void eval_shape_derivs_at_face_ips();

  const double scsDist_;
  const bool useGLLGLL_;
  const int nodes1D_;
  const int numQuad_;

  // quadrature info
  std::vector<double> gaussAbscissae_;
  std::vector<double> gaussWeight_;
  std::vector<double> scsEndLoc_;

  std::vector<int> stkNodeMap_;

  std::vector<double> shapeFunctions_;
  std::vector<double> shapeDerivs_;
  std::vector<double> expFaceShapeDerivs_;

private:
  void hex27_shape_fcn(
    int numIntPoints,
    const double *intgLoc,
    double* shpfc
  ) const;

  void hex27_shape_deriv(
    int numIntPoints,
    const double *intgLoc,
    double* shapeDerivs
  ) const;
};

// 3D Quad 27 subcontrol volume
class Hex27SCV : public HexahedralP2Element
{
public:
  Hex27SCV();
  virtual ~Hex27SCV() {}

  const int * ipNodeMap(int ordinal = 0);

  void determinant(
    const int nelem,
    const double *coords,
    double *volume,
    double * error );

private:
  void set_interior_info();

  double jacobian_determinant(
    const double *elemNodalCoords,
    const double *shapeDerivs ) const;

  std::vector<double> ipWeight_;
};

// 3D Hex 27 subcontrol surface
class Hex27SCS : public HexahedralP2Element
{
public:
  Hex27SCS();
  virtual ~Hex27SCS() {}

  void determinant(
    const int nelem,
    const double *coords,
    double *areav,
    double * error );

  void grad_op(
    const int nelem,
    const double *coords,
    double *gradop,
    double *deriv,
    double *det_j,
    double * error );

  void face_grad_op(
    const int nelem,
    const int face_ordinal,
    const double *coords,
    double *gradop,
    double *det_j,
    double * error );

  const int * adjacentNodes();

  const int * ipNodeMap(int ordinal = 0);

  int opposingNodes(
    const int ordinal, const int node);

  int opposingFace(
    const int ordinal, const int node);

private:
  void set_interior_info();
  void set_boundary_info();

  void area_vector(
    const Jacobian::Direction direction,
    const double *elemNodalCoords,
    double *shapeDeriv,
    double *areaVector ) const;

  void gradient(
    const double* elemNodalCoords,
    const double* shapeDeriv,
    double* grad,
    double* det_j ) const;

  std::vector<ContourData> ipInfo_;
  int ipsPerFace_;
};

// 3D Quad 9
class Quad93DSCS : public HexahedralP2Element
{
public:
  Quad93DSCS();
  virtual ~Quad93DSCS() {}

  const int * ipNodeMap(int ordinal = 0);

  void determinant(
    const int nelem,
    const double *coords,
    double *areav,
    double * error );

private:
  void set_interior_info();
  void eval_shape_functions_at_ips();
  void eval_shape_derivs_at_ips();

  void area_vector(
    const double *elemNodalCoords,
    const double *shapeDeriv,
    double *areaVector) const;

  void quad9_shape_fcn(
    int numIntPoints,
    const double *intgLoc,
    double* shpfc
  ) const;

  void quad9_shape_deriv(
    int numIntPoints,
    const double *intgLoc,
    double* deriv
  ) const;

  std::vector<double> ipWeight_;
  const int surfaceDimension_;
};

} // namespace nalu
} // namespace Sierra

#endif
