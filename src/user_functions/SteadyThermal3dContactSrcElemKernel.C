/*------------------------------------------------------------------------*/
/*  Copyright 2014 National Renewable Energy Laboratory.                  */
/*  This software is released under the license detailed                  */
/*  in the file, LICENSE, which is located in the top-level Nalu          */
/*  directory structure                                                   */
/*------------------------------------------------------------------------*/

#include "user_functions/SteadyThermal3dContactSrcElemKernel.h"
#include "AlgTraits.h"
#include "master_element/MasterElement.h"
#include "SolutionOptions.h"
#include "TimeIntegrator.h"

// template and scratch space
#include "BuildTemplates.h"
#include "ScratchViews.h"

// stk_mesh/base/fem
#include <stk_mesh/base/Entity.hpp>
#include <stk_mesh/base/MetaData.hpp>
#include <stk_mesh/base/BulkData.hpp>
#include <stk_mesh/base/Field.hpp>

#include <cmath>

namespace sierra {
namespace nalu {

template<typename AlgTraits>
SteadyThermal3dContactSrcElemKernel<AlgTraits>::SteadyThermal3dContactSrcElemKernel(
  const stk::mesh::BulkData& bulkData,
  SolutionOptions& solnOpts,
  ElemDataRequests& dataPreReqs)
  : Kernel(),
    ipNodeMap_(sierra::nalu::get_volume_master_element(AlgTraits::topo_)->ipNodeMap()),
    a_(1.0),
    k_(1.0),
    pi_(std::acos(-1.0))
{
  const stk::mesh::MetaData& metaData = bulkData.mesh_meta_data();
  coordinates_ = metaData.get_field<VectorFieldType>(
    stk::topology::NODE_RANK, solnOpts.get_coordinates_name());

  MasterElement *meSCV = sierra::nalu::get_volume_master_element(AlgTraits::topo_);
  meSCV->shape_fcn(&v_shape_function_(0,0));

  // add master elements
  dataPreReqs.add_cvfem_volume_me(meSCV);

  // fields and data
  dataPreReqs.add_gathered_nodal_field(*coordinates_, AlgTraits::nDim_);
  dataPreReqs.add_master_element_call(SCV_VOLUME);
}

template<typename AlgTraits>
void
SteadyThermal3dContactSrcElemKernel<AlgTraits>::execute(
  SharedMemView<double**>& /* lhs */,
  SharedMemView<double *>& rhs,
  stk::mesh::Entity ,
  ScratchViews& scratchViews)
{
  SharedMemView<double**>& v_coordinates = scratchViews.get_scratch_view_2D(*coordinates_);
  SharedMemView<double*>& v_scv_volume = scratchViews.scv_volume;

  // interpolate to ips and evaluate source
  for ( int ip = 0; ip < AlgTraits::numScvIp_; ++ip ) {

    // nearest node to ip
    const int nearestNode = ipNodeMap_[ip];

    // zero out
    for ( int j =0; j < AlgTraits::nDim_; ++j )
      v_scvCoords_(j) = 0.0;

    for ( int ic = 0; ic < AlgTraits::nodesPerElement_; ++ic ) {
      const double r = v_shape_function_(ip,ic);
      for ( int j = 0; j < AlgTraits::nDim_; ++j )
        v_scvCoords_(j) += r*v_coordinates(ic,j);
    }
    const double x = v_scvCoords_(0);
    const double y = v_scvCoords_(1);
    const double z = v_scvCoords_(2);
    rhs(nearestNode) += k_/4.0*(2.0*a_*pi_)*(2.0*a_*pi_)*(
      cos(2.0*a_*pi_*x)
      + cos(2.0*a_*pi_*y)
      + cos(2.0*a_*pi_*z))*v_scv_volume(ip);
  }
}

INSTANTIATE_KERNEL(SteadyThermal3dContactSrcElemKernel);

}  // nalu
}  // sierra