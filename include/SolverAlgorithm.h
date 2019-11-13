/*------------------------------------------------------------------------*/
/*  Copyright 2014 Sandia Corporation.                                    */
/*  This software is released under the license detailed                  */
/*  in the file, LICENSE, which is located in the top-level Nalu          */
/*  directory structure                                                   */
/*------------------------------------------------------------------------*/


#ifndef SolverAlgorithm_h
#define SolverAlgorithm_h

#include <Algorithm.h>
#include <KokkosInterface.h>

#include <stk_mesh/base/Entity.hpp>
#include <stk_ngp/Ngp.hpp>
#include <vector>

namespace sierra{
namespace nalu{

class CoeffApplier;
class EquationSystem;
class Realm;

struct NGPApplyCoeff
{
  NGPApplyCoeff(EquationSystem*);

  KOKKOS_INLINE_FUNCTION
  NGPApplyCoeff() = default;

  KOKKOS_INLINE_FUNCTION
  ~NGPApplyCoeff() = default;

  KOKKOS_FUNCTION
  void operator()(
    unsigned numMeshobjs,
    const ngp::Mesh::ConnectedNodes& symMeshobjs,
    const SharedMemView<int*,DeviceShmem> & scratchIds,
    const SharedMemView<int*,DeviceShmem> & sortPermutation,
    SharedMemView<double*,DeviceShmem> & rhs,
    SharedMemView<double**,DeviceShmem> & lhs,
    const char *trace_tag) const;

  KOKKOS_FUNCTION
  void extract_diagonal(
    const unsigned nEntities,
    const ngp::Mesh::ConnectedNodes& entities,
    SharedMemView<double**, DeviceShmem>& lhs) const;

  KOKKOS_FUNCTION
  void reset_overset_rows(
    const unsigned nEntities,
    const ngp::Mesh::ConnectedNodes& entities,
    SharedMemView<double*, DeviceShmem>&  rhs,
    SharedMemView<double**, DeviceShmem>& lhs) const;

  const ngp::Mesh ngpMesh_;
  mutable ngp::Field<double> diagField_;
  ngp::Field<int> iblankField_;

  CoeffApplier* deviceSumInto_;

  const unsigned nDim_{3};
  const bool hasOverset_{false};
  const bool extractDiagonal_{false};
};

class SolverAlgorithm : public Algorithm
{
public:

  SolverAlgorithm(
    Realm &realm,
    stk::mesh::Part *part,
    EquationSystem *eqSystem);
  virtual ~SolverAlgorithm() {}

  virtual void execute() = 0;
  virtual void initialize_connectivity() = 0;

protected:

  NGPApplyCoeff coeff_applier()
  { return NGPApplyCoeff(eqSystem_); }

  // Need to find out whether this ever gets called inside a modification cycle.
  void apply_coeff(
    const std::vector<stk::mesh::Entity> & sym_meshobj,
    std::vector<int> &scratchIds,
    std::vector<double> &scratchVals,
    std::vector<double> &rhs,
    std::vector<double> &lhs,
    const char *trace_tag=0);
  
  void apply_coeff(
    unsigned numMeshobjs,
    const stk::mesh::Entity* symMeshobjs,
    const SharedMemView<int*> & scratchIds,
    const SharedMemView<int*> & sortPermutation,
    SharedMemView<double*> & rhs,
    SharedMemView<double**> & lhs,
    const char *trace_tag);

  void apply_coeff(
    unsigned numMeshobjs,
    const ngp::Mesh::ConnectedNodes& symMeshobjs,
    const SharedMemView<int*,DeviceShmem> & scratchIds,
    const SharedMemView<int*,DeviceShmem> & sortPermutation,
    const SharedMemView<const double*,DeviceShmem> & rhs,
    const SharedMemView<const double**,DeviceShmem> & lhs,
    const char *trace_tag);

  EquationSystem *eqSystem_;
};

} // namespace nalu
} // namespace Sierra

#endif
