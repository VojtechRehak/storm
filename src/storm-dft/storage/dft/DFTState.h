#pragma once

#include <sstream>
#include <memory>

#include "storm/storage/BitVector.h"

#include "storm-dft/storage/dft/DFTElementState.h"
#include "storm-dft/builder/DftExplorationHeuristic.h"

namespace storm {
    namespace storage {

        template<typename ValueType>
        class DFT;
        template<typename ValueType>
        class DFTBE;
        template<typename ValueType>
        class DFTElement;
        class DFTStateGenerationInfo;

        template<typename ValueType>
        class DFTState {
            friend struct std::hash<DFTState>;
        private:
            // Status is bitvector where each element has two bits with the meaning according to DFTElementState
            storm::storage::BitVector mStatus;
            size_t mId;
            std::vector<size_t> mCurrentlyFailableBE;
            std::vector<size_t> mFailableDependencies;
            std::vector<size_t> mUsedRepresentants;
            bool mPseudoState;
            bool mValid = true;
            const DFT<ValueType>& mDft;
            const DFTStateGenerationInfo& mStateGenerationInfo;

        public:
            /**
             * Construct the initial state.
             *
             * @param dft                 DFT
             * @param stateGenerationInfo General information for state generation
             * @param id                  State id
             */
            DFTState(DFT<ValueType> const& dft, DFTStateGenerationInfo const& stateGenerationInfo, size_t id);

            /**
             * Construct temporary pseudo state. The actual state is constructed later.
             *
             * @param status              BitVector representing the status of the state.
             * @param dft                 DFT
             * @param stateGenerationInfo General information for state generation
             * @param id                  Pseudo state id
             */
            DFTState(storm::storage::BitVector const& status, DFT<ValueType> const& dft, DFTStateGenerationInfo const& stateGenerationInfo, size_t id);

            /**
             * Construct concerete state from pseudo state by using the underlying bitvector.
             */
            void construct();

            std::shared_ptr<DFTState<ValueType>> copy() const;

            DFTElementState getElementState(size_t id) const;
            
            DFTDependencyState getDependencyState(size_t id) const;

            int getElementStateInt(size_t id) const;

            static int getElementStateInt(storm::storage::BitVector const& state, size_t indexId);
            
            size_t getId() const;

            void setId(size_t id);
            
            bool isOperational(size_t id) const;
            
            bool hasFailed(size_t id) const;
            
            static bool hasFailed(storm::storage::BitVector const& state, size_t indexId);
            
            bool isFailsafe(size_t id) const;

            static bool isFailsafe(storm::storage::BitVector const& state, size_t indexId);

            bool dontCare(size_t id) const;

            bool dependencyTriggered(size_t id) const;

            bool dependencySuccessful(size_t id) const;

            bool dependencyUnsuccessful(size_t id) const;

            void setFailed(size_t id);
            
            void setFailsafe(size_t id);
            
            void setDontCare(size_t id);
            
            void setDependencySuccessful(size_t id);

            void setDependencyUnsuccessful(size_t id);

            void setDependencyDontCare(size_t id);

            void beNoLongerFailable(size_t id);
            
            void activate(size_t repr);
            
            bool isActive(size_t id) const;
            
            void markAsInvalid() {
                mValid = false;
            }
           
            bool isInvalid() const {
                return !mValid;
            }

            bool isPseudoState() const {
                return mPseudoState;
            }

            storm::storage::BitVector const& status() const {
                return mStatus;
            }
           
            /**
             * This method returns the id of the used child for a spare. If no element is used, it returns the given id.
             * @param id Id of the spare
             * @return The id of the currently used child or if non is used (because of spare failure) the id of
             * the spare.
             */
            uint_fast64_t uses(size_t id) const;
            
            /**
             * This method is commonly used to get the usage information for spares. 
             * @param from Starting index where the usage info is.
             * @return The child that currently is used.
             */
            uint_fast64_t extractUses(size_t from) const;
            
            /**
             * Checks whether an element is currently used.
             * @param child The id of the child for which we want to know whether it is currently used.
             * @return true iff it is currently used by any of the spares.
             */
            bool isUsed(size_t child) const;
            
            /**
             * Sets for the spare which child is now used.
             * @param spareId Id of the spare
             * @param child Id of the child which is now used
             */
            void setUses(size_t spareId, size_t child);
            
            /**
             * Sets the use for the spare to a default value to gain consistent states after failures.
             * @param spareId Id of the spare
             */
            void finalizeUses(size_t spareId);

            /**
             * Claim a new spare child for the given spare gate.
             *
             * @param spareId       Id of the spare gate.
             * @param currentlyUses Id of the currently used spare child.
             * @param children      List of children of this spare.
             *
             * @return True, if claiming was successful.
             */
            bool claimNew(size_t spareId, size_t currentlyUses, std::vector<std::shared_ptr<DFTElement<ValueType>>> const& children);
            
            /**
             * Get number of currently failable BEs.
             *
             * @return Number of failable BEs.
             */
            size_t nrFailableBEs() const {
                return mCurrentlyFailableBE.size();
            }

            /**
             * Get the failure rate of the currently failable BE on the given index.
             *
             * @param index           Index of BE in list of currently failable BEs.
             *
             * @return Failure rate of the BE.
             */
            ValueType getFailableBERate(size_t index) const;

            /**
             * Get the current failure rate of the given BE.
             *
             * @param id        Id of BE.
             *
             * @return Failure rate of the BE.
             */
            ValueType getBERate(size_t id) const;

            /** Get number of currently failable dependencies.
             *
             * @return Number of failable dependencies.
             */
            size_t nrFailableDependencies() const {
                return mFailableDependencies.size();
            }
            
            /**
             * Gets the id of the dependency at index in the list of failable dependencies.
             * @param index Index in list of failable dependencies.
             * @return Id of the dependency
             */
            size_t getDependencyId(size_t index) const {
                STORM_LOG_ASSERT(index < nrFailableDependencies(), "Index invalid.");
                return mFailableDependencies[index];
            }

            /**
             * Sets all failable BEs due to dependencies from newly failed element
             * @param id Id of the newly failed element
             * @return true if failable dependent events exist
             */
            bool updateFailableDependencies(size_t id);
            
            /**
             * Sets all dependencies dont care whose dependent event is the newly failed BE.
             * @param id Id of the newly failed element
             */
            void updateDontCareDependencies(size_t id);

            /**
             * Sets the next BE as failed
             * @param index Index in currentlyFailableBE of BE to fail
             * @return Pair of BE which fails and flag indicating if the failure was due to functional dependencies
             */
            std::pair<std::shared_ptr<DFTBE<ValueType> const>, bool> letNextBEFail(size_t index = 0);
            
            /**
             * Sets the dependency as unsuccesful meaning no BE will fail.
             * @param index Index of dependency to fail.
             */
            void letDependencyBeUnsuccessful(size_t index = 0);
            
            /**
             * Order the state in decreasing order using the symmetries.
             * @return True, if elements were swapped, false if nothing changed.
             */
            bool orderBySymmetry();
            
            /**
             * Checks whether operational post seq elements are present
             * @param id
             * @return 
             */
            bool hasOperationalPostSeqElements(size_t id) const;
            
            std::string getCurrentlyFailableString() const {
                std::stringstream stream;
                if (nrFailableDependencies() > 0) {
                    auto it = mFailableDependencies.begin();
                    stream << "{Dependencies: ";
                    if (it != mFailableDependencies.end()) {
                        stream << *it;
                    }
                    ++it;
                    while(it != mFailableDependencies.end()) {
                        stream << ", " << *it;
                        ++it;
                    }
                    stream << "} ";
                } else {
                    auto it = mCurrentlyFailableBE.begin();
                    stream << "{";
                    if(it != mCurrentlyFailableBE.end()) {
                        stream << *it;
                    }
                    ++it;
                    while(it != mCurrentlyFailableBE.end()) {
                        stream << ", " << *it;
                        ++it;
                    }
                    stream << "}";
                }
                return stream.str();
            }

            friend bool operator==(DFTState const& a, DFTState const& b) {
                return a.mStatus == b.mStatus;
            }
            
        private:
            void propagateActivation(size_t representativeId);

        };

    }
}

namespace std {
    template<typename ValueType>
    struct hash<storm::storage::DFTState<ValueType>> {
        size_t operator()(storm::storage::DFTState<ValueType> const& s) const {
            return hash<storm::storage::BitVector>()(s.mStatus);
        }
    };
}

