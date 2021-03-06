// This file is part of Eigen, a lightweight C++ template library
// for linear algebra.
//
// Copyright (C) 2014 Navdeep Jaitly <ndjaitly@google.com>
//                    Benoit Steiner <benoit.steiner.goog@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef STORMEIGEN_CXX11_TENSOR_TENSOR_REVERSE_H
#define STORMEIGEN_CXX11_TENSOR_TENSOR_REVERSE_H
namespace StormEigen {

/** \class TensorReverse
  * \ingroup CXX11_Tensor_Module
  *
  * \brief Tensor reverse elements class.
  *
  */
namespace internal {
template<typename ReverseDimensions, typename XprType>
struct traits<TensorReverseOp<ReverseDimensions,
                              XprType> > : public traits<XprType>
{
  typedef typename XprType::Scalar Scalar;
  typedef traits<XprType> XprTraits;
  typedef typename packet_traits<Scalar>::type Packet;
  typedef typename XprTraits::StorageKind StorageKind;
  typedef typename XprTraits::Index Index;
  typedef typename XprType::Nested Nested;
  typedef typename remove_reference<Nested>::type _Nested;
  static const int NumDimensions = XprTraits::NumDimensions;
  static const int Layout = XprTraits::Layout;
};

template<typename ReverseDimensions, typename XprType>
struct eval<TensorReverseOp<ReverseDimensions, XprType>, StormEigen::Dense>
{
  typedef const TensorReverseOp<ReverseDimensions, XprType>& type;
};

template<typename ReverseDimensions, typename XprType>
struct nested<TensorReverseOp<ReverseDimensions, XprType>, 1,
            typename eval<TensorReverseOp<ReverseDimensions, XprType> >::type>
{
  typedef TensorReverseOp<ReverseDimensions, XprType> type;
};

}  // end namespace internal

template<typename ReverseDimensions, typename XprType>
class TensorReverseOp : public TensorBase<TensorReverseOp<ReverseDimensions,
                                          XprType>, WriteAccessors>
{
  public:
  typedef typename StormEigen::internal::traits<TensorReverseOp>::Scalar Scalar;
  typedef typename StormEigen::internal::traits<TensorReverseOp>::Packet Packet;
  typedef typename StormEigen::NumTraits<Scalar>::Real RealScalar;
  typedef typename XprType::CoeffReturnType CoeffReturnType;
  typedef typename XprType::PacketReturnType PacketReturnType;
  typedef typename StormEigen::internal::nested<TensorReverseOp>::type Nested;
  typedef typename StormEigen::internal::traits<TensorReverseOp>::StorageKind
                                                                    StorageKind;
  typedef typename StormEigen::internal::traits<TensorReverseOp>::Index Index;

  STORMEIGEN_DEVICE_FUNC STORMEIGEN_STRONG_INLINE TensorReverseOp(
      const XprType& expr, const ReverseDimensions& reverse_dims)
      : m_xpr(expr), m_reverse_dims(reverse_dims) { }

    STORMEIGEN_DEVICE_FUNC
    const ReverseDimensions& reverse() const { return m_reverse_dims; }

    STORMEIGEN_DEVICE_FUNC
    const typename internal::remove_all<typename XprType::Nested>::type&
    expression() const { return m_xpr; }

    STORMEIGEN_DEVICE_FUNC
    STORMEIGEN_STRONG_INLINE TensorReverseOp& operator = (const TensorReverseOp& other)
    {
      typedef TensorAssignOp<TensorReverseOp, const TensorReverseOp> Assign;
      Assign assign(*this, other);
      internal::TensorExecutor<const Assign, DefaultDevice>::run(assign, DefaultDevice());
      return *this;
    }

    template<typename OtherDerived>
    STORMEIGEN_DEVICE_FUNC
    STORMEIGEN_STRONG_INLINE TensorReverseOp& operator = (const OtherDerived& other)
    {
      typedef TensorAssignOp<TensorReverseOp, const OtherDerived> Assign;
      Assign assign(*this, other);
      internal::TensorExecutor<const Assign, DefaultDevice>::run(assign, DefaultDevice());
      return *this;
    }

  protected:
    typename XprType::Nested m_xpr;
    const ReverseDimensions m_reverse_dims;
};

// Eval as rvalue
template<typename ReverseDimensions, typename ArgType, typename Device>
struct TensorEvaluator<const TensorReverseOp<ReverseDimensions, ArgType>, Device>
{
  typedef TensorReverseOp<ReverseDimensions, ArgType> XprType;
  typedef typename XprType::Index Index;
  static const int NumDims = internal::array_size<ReverseDimensions>::value;
  typedef DSizes<Index, NumDims> Dimensions;

  enum {
    IsAligned = false,
    PacketAccess = TensorEvaluator<ArgType, Device>::PacketAccess,
    Layout = TensorEvaluator<ArgType, Device>::Layout,
    CoordAccess = false,  // to be implemented
  };

  STORMEIGEN_DEVICE_FUNC STORMEIGEN_STRONG_INLINE TensorEvaluator(const XprType& op,
                                                        const Device& device)
      : m_impl(op.expression(), device), m_reverse(op.reverse())
  {
    // Reversing a scalar isn't supported yet. It would be a no-op anyway.
    STORMEIGEN_STATIC_ASSERT(NumDims > 0, YOU_MADE_A_PROGRAMMING_MISTAKE);

    // Compute strides
    m_dimensions = m_impl.dimensions();
    if (static_cast<int>(Layout) == static_cast<int>(ColMajor)) {
      m_strides[0] = 1;
      for (int i = 1; i < NumDims; ++i) {
        m_strides[i] = m_strides[i-1] * m_dimensions[i-1];
      }
    } else {
      m_strides[NumDims-1] = 1;
      for (int i = NumDims - 2; i >= 0; --i) {
        m_strides[i] = m_strides[i+1] * m_dimensions[i+1];
      }
    }
  }

  typedef typename XprType::Scalar Scalar;
  typedef typename XprType::CoeffReturnType CoeffReturnType;
  typedef typename XprType::PacketReturnType PacketReturnType;

  STORMEIGEN_DEVICE_FUNC STORMEIGEN_STRONG_INLINE
  const Dimensions& dimensions() const { return m_dimensions; }

  STORMEIGEN_DEVICE_FUNC STORMEIGEN_STRONG_INLINE bool evalSubExprsIfNeeded(Scalar*) {
    m_impl.evalSubExprsIfNeeded(NULL);
    return true;
  }
  STORMEIGEN_DEVICE_FUNC STORMEIGEN_STRONG_INLINE void cleanup() {
    m_impl.cleanup();
  }

  STORMEIGEN_DEVICE_FUNC STORMEIGEN_STRONG_INLINE Index reverseIndex(
      Index index) const {
    eigen_assert(index < dimensions().TotalSize());
    Index inputIndex = 0;
    if (static_cast<int>(Layout) == static_cast<int>(ColMajor)) {
      for (int i = NumDims - 1; i > 0; --i) {
        Index idx = index / m_strides[i];
        index -= idx * m_strides[i];
        if (m_reverse[i]) {
          idx = m_dimensions[i] - idx - 1;
        }
        inputIndex += idx * m_strides[i] ;
      }
      if (m_reverse[0]) {
        inputIndex += (m_dimensions[0] - index - 1);
      } else {
        inputIndex += index;
      }
    } else {
      for (int i = 0; i < NumDims - 1; ++i) {
        Index idx = index / m_strides[i];
        index -= idx * m_strides[i];
        if (m_reverse[i]) {
          idx = m_dimensions[i] - idx - 1;
        }
        inputIndex += idx * m_strides[i] ;
      }
      if (m_reverse[NumDims-1]) {
        inputIndex += (m_dimensions[NumDims-1] - index - 1);
      } else {
        inputIndex += index;
      }
    }
    return inputIndex;
  }

  STORMEIGEN_DEVICE_FUNC STORMEIGEN_STRONG_INLINE CoeffReturnType coeff(
      Index index) const  {
    return m_impl.coeff(reverseIndex(index));
  }

  template<int LoadMode>
  STORMEIGEN_DEVICE_FUNC STORMEIGEN_STRONG_INLINE
  PacketReturnType packet(Index index) const
  {
    const int packetSize = internal::unpacket_traits<PacketReturnType>::size;
    STORMEIGEN_STATIC_ASSERT(packetSize > 1, YOU_MADE_A_PROGRAMMING_MISTAKE)
    eigen_assert(index+packetSize-1 < dimensions().TotalSize());

    // TODO(ndjaitly): write a better packing routine that uses
    // local structure.
    STORMEIGEN_ALIGN_MAX typename internal::remove_const<CoeffReturnType>::type
                                                            values[packetSize];
    for (int i = 0; i < packetSize; ++i) {
      values[i] = coeff(index+i);
    }
    PacketReturnType rslt = internal::pload<PacketReturnType>(values);
    return rslt;
  }

  STORMEIGEN_DEVICE_FUNC Scalar* data() const { return NULL; }

 protected:
  Dimensions m_dimensions;
  array<Index, NumDims> m_strides;
  TensorEvaluator<ArgType, Device> m_impl;
  ReverseDimensions m_reverse;
};

// Eval as lvalue

template <typename ReverseDimensions, typename ArgType, typename Device>
struct TensorEvaluator<TensorReverseOp<ReverseDimensions, ArgType>, Device>
    : public TensorEvaluator<const TensorReverseOp<ReverseDimensions, ArgType>,
                             Device> {
  typedef TensorEvaluator<const TensorReverseOp<ReverseDimensions, ArgType>,
                          Device> Base;
  typedef TensorReverseOp<ReverseDimensions, ArgType> XprType;
  typedef typename XprType::Index Index;
  static const int NumDims = internal::array_size<ReverseDimensions>::value;
  typedef DSizes<Index, NumDims> Dimensions;

  enum {
    IsAligned = false,
    PacketAccess = TensorEvaluator<ArgType, Device>::PacketAccess,
    Layout = TensorEvaluator<ArgType, Device>::Layout,
    CoordAccess = false,  // to be implemented
  };
  STORMEIGEN_DEVICE_FUNC STORMEIGEN_STRONG_INLINE TensorEvaluator(const XprType& op,
                                                        const Device& device)
      : Base(op, device) {}

  typedef typename XprType::Scalar Scalar;
  typedef typename XprType::CoeffReturnType CoeffReturnType;
  typedef typename XprType::PacketReturnType PacketReturnType;

  STORMEIGEN_DEVICE_FUNC STORMEIGEN_STRONG_INLINE
  const Dimensions& dimensions() const { return this->m_dimensions; }

  STORMEIGEN_DEVICE_FUNC STORMEIGEN_STRONG_INLINE Scalar& coeffRef(Index index) {
    return this->m_impl.coeffRef(this->reverseIndex(index));
  }

  template <int StoreMode> STORMEIGEN_DEVICE_FUNC STORMEIGEN_STRONG_INLINE
  void writePacket(Index index, const PacketReturnType& x) {
    const int packetSize = internal::unpacket_traits<PacketReturnType>::size;
    STORMEIGEN_STATIC_ASSERT(packetSize > 1, YOU_MADE_A_PROGRAMMING_MISTAKE)
    eigen_assert(index+packetSize-1 < dimensions().TotalSize());

    // This code is pilfered from TensorMorphing.h
    STORMEIGEN_ALIGN_MAX CoeffReturnType values[packetSize];
    internal::pstore<CoeffReturnType, PacketReturnType>(values, x);
    for (int i = 0; i < packetSize; ++i) {
      this->coeffRef(index+i) = values[i];
    }
  }

};


}  // end namespace StormEigen

#endif // STORMEIGEN_CXX11_TENSOR_TENSOR_REVERSE_H
