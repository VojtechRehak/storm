#include "storm/models/sparse/Gsmp.h"
#include "storm/models/sparse/StandardRewardModel.h"
#include "storm/adapters/RationalFunctionAdapter.h"
#include "storm/exceptions/NotImplementedException.h"
#include "storm/exceptions/InvalidArgumentException.h"
#include "storm/utility/constants.h"
#include "storm/storage/expressions/EventDistributionExpression.h"

namespace storm {
    namespace models {
        namespace sparse {
            
            template <typename ValueType, typename RewardModelType>
            Gsmp<ValueType, RewardModelType>::Gsmp(storm::storage::SparseMatrix<ValueType> const& transitionMatrix, storm::models::sparse::StateLabeling const& stateLabeling,
                                  std::unordered_map<std::string, RewardModelType> const& rewardModels)
                    : Gsmp<ValueType, RewardModelType>(storm::storage::sparse::ModelComponents<ValueType, RewardModelType>(transitionMatrix, stateLabeling, rewardModels)) {
                // Intentionally left empty
            }
            
            template <typename ValueType, typename RewardModelType>
            Gsmp<ValueType, RewardModelType>::Gsmp(storm::storage::SparseMatrix<ValueType>&& transitionMatrix, storm::models::sparse::StateLabeling&& stateLabeling,
                                  std::unordered_map<std::string, RewardModelType>&& rewardModels)
                    : Gsmp<ValueType, RewardModelType>(storm::storage::sparse::ModelComponents<ValueType, RewardModelType>(std::move(transitionMatrix), std::move(stateLabeling), std::move(rewardModels))) {
                // Intentionally left empty
            }
            
            template <typename ValueType, typename RewardModelType>
            Gsmp<ValueType, RewardModelType>::Gsmp(storm::storage::sparse::ModelComponents<ValueType, RewardModelType> const& components)
                    : DeterministicModel<ValueType, RewardModelType>(storm::models::ModelType::Gsmp, components), eventVariables(components.eventVariables.get()), eventToStatesMapping(components.eventToStatesMapping.get()), stateToEventsMapping(components.stateToEventsMapping.get()), eventNameToId(components.eventNameToId.get()) {
                // Intentionally left empty
            }
            
            template <typename ValueType, typename RewardModelType>
            Gsmp<ValueType, RewardModelType>::Gsmp(storm::storage::sparse::ModelComponents<ValueType, RewardModelType>&& components)
                    : DeterministicModel<ValueType, RewardModelType>(storm::models::ModelType::Gsmp, std::move(components)), eventVariables(std::move(components.eventVariables.get())), eventToStatesMapping(std::move(components.eventToStatesMapping.get())), stateToEventsMapping(std::move(components.stateToEventsMapping.get())), eventNameToId(std::move(components.eventNameToId.get())) {

                // TODO(Roman): can remove this once we start implementing new functionality
                for (auto const& tt : eventNameToId) {
                    auto const& event = eventVariables[tt.second];
                    std::stringstream st;
                    st << event.arg1;
                    if (event.argc == 2) {
                        st << ", " << event.arg2;
                    }
                    STORM_PRINT_AND_LOG(tt.first << "= " << expressions::EventDistributionExpression::distributionTypeToString.at(event.distributionType) << "(" << st.str() << ")" << std::endl);
                    STORM_PRINT_AND_LOG(std::endl << getTransitionMatrixForEvent(tt.first) << std::endl);
                }
            }
   
            template<typename ValueType, typename RewardModelType>
            void Gsmp<ValueType, RewardModelType>::reduceToStateBasedRewards() {
                for (auto& rewardModel : this->getRewardModels()) {
                    rewardModel.second.reduceToStateBasedRewards(this->getTransitionMatrix(), true);
                }
            }

            template<typename ValueType, typename RewardModelType>
            bool Gsmp<ValueType, RewardModelType>::hasEvent(std::string const& event) {
                return eventNameToId.find(event) != eventNameToId.end();
            }


            template<typename ValueType, typename RewardModelType>
            storm::storage::SparseMatrix<ValueType> Gsmp<ValueType, RewardModelType>::getTransitionMatrixForEvent(std::string const& eventName) const {
                auto id = eventNameToId.at(eventName);
                auto const& transitionMatrix = this->getTransitionMatrix();
                storm::storage::SparseMatrixBuilder<ValueType> matrixBuilder;

                auto it = eventToStatesMapping.find(id);
                if (it == eventToStatesMapping.end()) {
                    // this means that there was no transition matrix created for this event
                    // probable cause is that it is exponentian event and participates only in 
                    // transitions fused with other events, or it as any event which does
                    // not participate in any transitions at all
                    return storm::storage::SparseMatrix<ValueType>();
                }
                auto activeStates = it->second;

                for (auto const& groupRowIdPair : activeStates) {
                    auto const& row = transitionMatrix.getRow(groupRowIdPair.second);
                    for (auto const& entry : row) {
                        matrixBuilder.addNextValue(groupRowIdPair.first, entry.getColumn(), entry.getValue());
                    }
                }
                return matrixBuilder.build();
            }
            
            template class Gsmp<double>;

#ifdef STORM_HAVE_CARL
            template class Gsmp<storm::RationalNumber>;

            template class Gsmp<double, storm::models::sparse::StandardRewardModel<storm::Interval>>;
            template class Gsmp<storm::RationalFunction>;
#endif
        } // namespace sparse
    } // namespace models
} // namespace storm
