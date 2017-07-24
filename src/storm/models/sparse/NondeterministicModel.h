#ifndef STORM_MODELS_SPARSE_NONDETERMINISTICMODEL_H_
#define STORM_MODELS_SPARSE_NONDETERMINISTICMODEL_H_

#include "storm/models/sparse/Model.h"
#include "storm/utility/OsDetection.h"

namespace storm {
    namespace models {
        namespace sparse {
            
            /*!
             * The base class of sparse nondeterministic models.
             */
            template<class ValueType, typename RewardModelType = StandardRewardModel<ValueType>>
            class NondeterministicModel: public Model<ValueType, RewardModelType> {
            public:
                
                
                /*!
                 * Constructs a model from the given data.
                 *
                 * @param modelType the type of this model
                 * @param components The components for this model.
                 */
                NondeterministicModel(ModelType modelType, storm::storage::sparse::ModelComponents<ValueType, RewardModelType> const& components);
                NondeterministicModel(ModelType modelType, storm::storage::sparse::ModelComponents<ValueType, RewardModelType>&& components);

                /*!
                 * Retrieves the number of (nondeterministic) choices in the model.
                 *
                 * @return The number of (nondeterministic) choices in the model.
                 */
                uint_fast64_t getNumberOfChoices() const;
                
                /*!
                 * Retrieves the vector indicating which matrix rows represent non-deterministic choices of a certain state.
                 *
                 * @return The vector indicating which matrix rows represent non-deterministic choices of a certain state.
                 */
                std::vector<uint_fast64_t> const& getNondeterministicChoiceIndices() const;
                
                /*!
                 * @param state State for which we want to know how many choices it has
                 * 
                 * @return The number of non-deterministic choices for the given state
                 */
                uint_fast64_t getNumberOfChoices(uint_fast64_t state) const;
                
                virtual void reduceToStateBasedRewards() override;
                
                virtual void printModelInformationToStream(std::ostream& out) const override;
                
                virtual void writeDotToStream(std::ostream& outStream, bool includeLabeling = true, storm::storage::BitVector const* subsystem = nullptr, std::vector<ValueType> const* firstValue = nullptr, std::vector<ValueType> const* secondValue = nullptr, std::vector<uint_fast64_t> const* stateColoring = nullptr, std::vector<std::string> const* colors = nullptr, std::vector<uint_fast64_t>* scheduler = nullptr, bool finalizeOutput = true) const override;
            };
            
        } // namespace sparse
    } // namespace models
} // namespace storm

#endif /* STORM_MODELS_SPARSE_NONDETERMINISTICMODEL_H_ */