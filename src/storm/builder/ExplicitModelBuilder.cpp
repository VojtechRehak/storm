#include "storm/builder/ExplicitModelBuilder.h"

#include <map>

#include "storm/models/sparse/Dtmc.h"
#include "storm/models/sparse/Ctmc.h"
#include "storm/models/sparse/Mdp.h"
#include "storm/models/sparse/MarkovAutomaton.h"
#include "storm/models/sparse/StandardRewardModel.h"

#include "storm/storage/expressions/ExpressionManager.h"

#include "storm/settings/modules/CoreSettings.h"
#include "storm/settings/modules/IOSettings.h"

#include "storm/builder/RewardModelBuilder.h"
#include "storm/builder/ChoiceInformationBuilder.h"

#include "storm/generator/PrismNextStateGenerator.h"
#include "storm/generator/JaniNextStateGenerator.h"


#include "storm/storage/jani/Edge.h"
#include "storm/storage/jani/EdgeDestination.h"
#include "storm/storage/jani/Model.h"
#include "storm/storage/jani/Automaton.h"
#include "storm/storage/jani/Location.h"
#include "storm/storage/jani/AutomatonComposition.h"
#include "storm/storage/jani/ParallelComposition.h"
#include "storm/storage/jani/CompositionInformationVisitor.h"

#include "storm/utility/prism.h"
#include "storm/utility/constants.h"
#include "storm/utility/macros.h"
#include "storm/utility/ConstantsComparator.h"
#include "storm/utility/builder.h"

#include "storm/exceptions/WrongFormatException.h"
#include "storm/exceptions/InvalidArgumentException.h"
#include "storm/exceptions/InvalidOperationException.h"
#include "storm/storage/expressions/EventDistributionTypes.h"

namespace storm {
    namespace builder {
                        
        template <typename ValueType, typename RewardModelType, typename StateType>
        ExplicitModelBuilder<ValueType, RewardModelType, StateType>::Options::Options() : explorationOrder(storm::settings::getModule<storm::settings::modules::IOSettings>().getExplorationOrder()) {
            // Intentionally left empty.
        }
        
        template <typename ValueType, typename RewardModelType, typename StateType>
        ExplicitModelBuilder<ValueType, RewardModelType, StateType>::ExplicitModelBuilder(std::shared_ptr<storm::generator::NextStateGenerator<ValueType, StateType>> const& generator, Options const& options) : generator(generator), options(options), stateStorage(generator->getStateSize()) {
            // Intentionally left empty.
        }
        
        template <typename ValueType, typename RewardModelType, typename StateType>
        ExplicitModelBuilder<ValueType, RewardModelType, StateType>::ExplicitModelBuilder(storm::prism::Program const& program, storm::generator::NextStateGeneratorOptions const& generatorOptions, Options const& builderOptions) : ExplicitModelBuilder(std::make_shared<storm::generator::PrismNextStateGenerator<ValueType, StateType>>(program, generatorOptions), builderOptions) {
            // Intentionally left empty.
        }
        
        template <typename ValueType, typename RewardModelType, typename StateType>
        ExplicitModelBuilder<ValueType, RewardModelType, StateType>::ExplicitModelBuilder(storm::jani::Model const& model, storm::generator::NextStateGeneratorOptions const& generatorOptions, Options const& builderOptions) : ExplicitModelBuilder(std::make_shared<storm::generator::JaniNextStateGenerator<ValueType, StateType>>(model, generatorOptions), builderOptions) {
            // Intentionally left empty.
        }
        
        template <typename ValueType, typename RewardModelType, typename StateType>
        std::shared_ptr<storm::models::sparse::Model<ValueType, RewardModelType>> ExplicitModelBuilder<ValueType, RewardModelType, StateType>::build() {
            STORM_LOG_DEBUG("Exploration order is: " << options.explorationOrder);
            
            switch (generator->getModelType()) {
                case storm::generator::ModelType::DTMC:
                    return storm::utility::builder::buildModelFromComponents(storm::models::ModelType::Dtmc, buildModelComponents());
                case storm::generator::ModelType::CTMC:
                    return storm::utility::builder::buildModelFromComponents(storm::models::ModelType::Ctmc, buildModelComponents());
                case storm::generator::ModelType::MDP:
                    return storm::utility::builder::buildModelFromComponents(storm::models::ModelType::Mdp, buildModelComponents());
                case storm::generator::ModelType::MA:
                    return storm::utility::builder::buildModelFromComponents(storm::models::ModelType::MarkovAutomaton, buildModelComponents());
                case storm::generator::ModelType::GSMP:
                    return storm::utility::builder::buildModelFromComponents(storm::models::ModelType::Gsmp, buildModelComponents());    
                default:
                    STORM_LOG_THROW(false, storm::exceptions::WrongFormatException, "Error while creating model: cannot handle this model type.");
            }
            
            return nullptr;
        }
        
        template <typename ValueType, typename RewardModelType, typename StateType>
        StateType ExplicitModelBuilder<ValueType, RewardModelType, StateType>::getOrAddStateIndex(CompressedState const& state) {
            StateType newIndex = static_cast<StateType>(stateStorage.getNumberOfStates());
            
            // Check, if the state was already registered.
            std::pair<StateType, std::size_t> actualIndexBucketPair = stateStorage.stateToId.findOrAddAndGetBucket(state, newIndex);
            
            if (actualIndexBucketPair.first == newIndex) {
                if (options.explorationOrder == ExplorationOrder::Dfs) {
                    statesToExplore.push_front(state);

                    // Reserve one slot for the new state in the remapping.
                    stateRemapping.get().push_back(storm::utility::zero<StateType>());
                } else if (options.explorationOrder == ExplorationOrder::Bfs) {
                    statesToExplore.push_back(state);
                } else {
                    STORM_LOG_ASSERT(false, "Invalid exploration order.");
                }
            }
            
            return actualIndexBucketPair.first;
        }
        
        template <typename ValueType, typename RewardModelType, typename StateType>
        void ExplicitModelBuilder<ValueType, RewardModelType, StateType>::buildMatrices(storm::storage::SparseMatrixBuilder<ValueType>& transitionMatrixBuilder, std::vector<RewardModelBuilder<typename RewardModelType::ValueType>>& rewardModelBuilders, ChoiceInformationBuilder& choiceInformationBuilder, boost::optional<storm::storage::BitVector>& markovianStates) {
            
            // Create markovian states bit vector, if required.
            if (generator->getModelType() == storm::generator::ModelType::MA) {
                // The bit vector will be resized when the correct size is known.
                markovianStates = storm::storage::BitVector(1000);
            }

            // Create a callback for the next-state generator to enable it to request the index of states.
            std::function<StateType (CompressedState const&)> stateToIdCallback = std::bind(&ExplicitModelBuilder<ValueType, RewardModelType, StateType>::getOrAddStateIndex, this, std::placeholders::_1);
            
            // If the exploration order is something different from breadth-first, we need to keep track of the remapping
            // from state ids to row groups. For this, we actually store the reversed mapping of row groups to state-ids
            // and later reverse it.
            if (options.explorationOrder != ExplorationOrder::Bfs) {
                stateRemapping = std::vector<uint_fast64_t>();
            }
            
            // Let the generator create all initial states.
            this->stateStorage.initialStateIndices = generator->getInitialStates(stateToIdCallback);
            
            // Now explore the current state until there is no more reachable state.
            uint_fast64_t currentRowGroup = 0;
            uint_fast64_t currentRow = 0;

            bool shouldMapEvents = generator->getModelType() == storm::generator::ModelType::GSMP;
            // mapping from event id -> index of row in the matrix if present 
            if (shouldMapEvents) {
                this->eventToStatesMapping = std::unordered_map<uint_fast64_t, std::map<uint_fast64_t, uint_fast64_t>>();
                this->stateToEventsMapping = std::unordered_map<uint_fast64_t, std::vector<uint_fast64_t>>();
                this->eventNameToId = std::unordered_map<std::string, uint_fast64_t>();
                this->eventVariables = std::vector<EventVariableInformation<ValueType>>();
                this->generator->mapEvents(eventVariables.get(), eventNameToId.get());
            }

            auto timeOfStart = std::chrono::high_resolution_clock::now();
            auto timeOfLastMessage = std::chrono::high_resolution_clock::now();
            uint64_t numberOfExploredStates = 0;
            uint64_t numberOfExploredStatesSinceLastMessage = 0;
            
            // Perform a search through the model.
            while (!statesToExplore.empty()) {
                // Get the first state in the queue.
                CompressedState currentState = statesToExplore.front();
                StateType currentIndex = stateStorage.stateToId.getValue(currentState);
                statesToExplore.pop_front();
                
                // If the exploration order differs from breadth-first, we remember that this row group was actually
                // filled with the transitions of a different state.
                if (options.explorationOrder != ExplorationOrder::Bfs) {
                    stateRemapping.get()[currentIndex] = currentRowGroup;
                }

                
                STORM_LOG_TRACE("Exploring state with id " << currentIndex << ".");
                
                generator->load(currentState);
                storm::generator::StateBehavior<ValueType, StateType> behavior = generator->expand(stateToIdCallback);
                
                // If there is no behavior, we might have to introduce a self-loop.
                if (behavior.empty()) {
                    if (!storm::settings::getModule<storm::settings::modules::CoreSettings>().isDontFixDeadlocksSet() || !behavior.wasExpanded()) {
                        // If the behavior was actually expanded and yet there are no transitions, then we have a deadlock state.
                        if (behavior.wasExpanded()) {
                            this->stateStorage.deadlockStateIndices.push_back(currentIndex);
                        }

                        
                        if (markovianStates) {
                            markovianStates.get().grow(currentRowGroup + 1, false);
                            markovianStates.get().set(currentRowGroup);
                        }
                        
                        if (!generator->isDeterministicModel()) {
                            transitionMatrixBuilder.newRowGroup(currentRow);
                        }
                        
                        transitionMatrixBuilder.addNextValue(currentRow, currentIndex, storm::utility::one<ValueType>());

                        if (generator->getModelType() == storm::generator::ModelType::GSMP) {
                            // we need to create a new GSMP event for the deadlock state
                            uint_fast64_t eventId = eventVariables.get().size();
                            eventVariables.get().push_back(EventVariableInformation<ValueType>(storm::utility::one<ValueType>(), storm::expressions::EventDistributionTypes::Exp));
                            std::string new_name = "deadlock_event_" + std::to_string(eventId);
                            eventNameToId.get()[new_name] = eventId;
                            eventToStatesMapping.get()[eventId][currentRowGroup] = currentRow;
                            stateToEventsMapping.get()[currentIndex].push_back(eventId);
                        }
                        
                        for (auto& rewardModelBuilder : rewardModelBuilders) {
                            if (rewardModelBuilder.hasStateRewards()) {
                                rewardModelBuilder.addStateReward(storm::utility::zero<ValueType>());
                            }
                            
                            if (rewardModelBuilder.hasStateActionRewards()) {
                                rewardModelBuilder.addStateActionReward(storm::utility::zero<ValueType>());
                            }
                        }
                        
                        ++currentRow;
                        ++currentRowGroup;
                    } else {
                        STORM_LOG_THROW(false, storm::exceptions::WrongFormatException, "Error while creating sparse matrix from probabilistic program: found deadlock state (" << generator->toValuation(currentState).toString(true) << "). For fixing these, please provide the appropriate option.");
                    }
                } else {
                    // Add the state rewards to the corresponding reward models.
                    auto stateRewardIt = behavior.getStateRewards().begin();
                    for (auto& rewardModelBuilder : rewardModelBuilders) {
                        if (rewardModelBuilder.hasStateRewards()) {
                            rewardModelBuilder.addStateReward(*stateRewardIt);
                        }
                        ++stateRewardIt;
                    }
                    
                    // If the model is nondeterministic, we need to open a row group.
                    if (!generator->isDeterministicModel()) {
                        transitionMatrixBuilder.newRowGroup(currentRow);
                    }
                    
                    // Now add all choices.
                    for (auto const& choice : behavior) {
                        
                        // add the generated choice information
                        if (choice.hasLabels()) {
                            for (auto const& label : choice.getLabels()) {
                                choiceInformationBuilder.addLabel(label, currentRow);
                            }
                        }
                        if (choice.hasOriginData()) {
                            choiceInformationBuilder.addOriginData(choice.getOriginData(), currentRow);
                        }
                        
                        // If we keep track of the Markovian choices, store whether the current one is Markovian.
                        if (markovianStates && choice.isMarkovian()) {
                            markovianStates.get().grow(currentRowGroup + 1, false);
                            markovianStates.get().set(currentRowGroup);
                        }
                        
                        // Add the probabilistic behavior to the matrix.
                        for (auto const& stateProbabilityPair : choice) {
                            transitionMatrixBuilder.addNextValue(currentRow, stateProbabilityPair.first, stateProbabilityPair.second);
                        }

                        if (shouldMapEvents && choice.hasEvents()) {
                            uint_fast64_t eventId;
                            auto& eventVariables = this->eventVariables.get();
                            auto& eventNameToId = this->eventNameToId.get();
                            auto& eventToStatesMapping = this->eventToStatesMapping.get();
                            auto& stateToEventsMapping = this->stateToEventsMapping.get();

                            // if choice has multiple events, we have to create a new event with distribution calculated as product of its current distributions
                            if (choice.hasMultipleEvents()) {
                                std::string new_name;
                                bool first = true;
                                for (std::string const& eventName : choice.getEventNames()) {

                                    STORM_LOG_THROW(eventVariables[eventNameToId[eventName]].distributionType == storm::expressions::EventDistributionTypes::Exp, storm::exceptions::WrongFormatException, "Invalid GSMP operation, non-exponential event \"" << eventName << "\" fusing with exponential events!");

                                    if (first) {
                                        first = false;
                                    } else {
                                        new_name += " X ";
                                    }
                                    new_name += eventName;
                                }
                                auto it = eventNameToId.find(new_name);
                                if (it == eventNameToId.end()) {

                                    ValueType expRate = storm::utility::one<ValueType>();
                                    for (std::string const& eventName : choice.getEventNames()) {
                                        auto const& event = eventVariables.at(eventNameToId.at(eventName));
                                        expRate *= event.arg1;
                                    }

                                    eventId = eventVariables.size();
                                    eventVariables.push_back(EventVariableInformation<ValueType>(expRate, storm::expressions::EventDistributionTypes::Exp));
                                    eventNameToId[new_name] = eventId; 
                                } else {
                                    eventId = it->second;
                                }

                            } else {

                                std::string const& eventName = choice.getEventNames()[0];
                                auto it = eventNameToId.find(eventName);
                                STORM_LOG_THROW(it != eventNameToId.end(), storm::exceptions::WrongFormatException, "internal error, event'" + eventName + "' not found in the map of events");

                                eventId = it->second;
                            }
                                eventToStatesMapping[eventId][currentRowGroup] = currentRow;
                                stateToEventsMapping[currentIndex].push_back(eventId);
                        }
                        
                        // Add the rewards to the reward models.
                        auto choiceRewardIt = choice.getRewards().begin();
                        for (auto& rewardModelBuilder : rewardModelBuilders) {
                            if (rewardModelBuilder.hasStateActionRewards()) {
                                rewardModelBuilder.addStateActionReward(*choiceRewardIt);
                            }
                            ++choiceRewardIt;
                        }
                        ++currentRow;
                    }
                    ++currentRowGroup;
                }
                
                if (generator->getOptions().isShowProgressSet()) {
                    ++numberOfExploredStatesSinceLastMessage;
                    ++numberOfExploredStates;
                    
                    auto now = std::chrono::high_resolution_clock::now();
                    auto durationSinceLastMessage = std::chrono::duration_cast<std::chrono::seconds>(now - timeOfLastMessage).count();
                    if (static_cast<uint64_t>(durationSinceLastMessage) >= generator->getOptions().getShowProgressDelay()) {
                        auto statesPerSecond = numberOfExploredStatesSinceLastMessage / durationSinceLastMessage;
                        auto durationSinceStart = std::chrono::duration_cast<std::chrono::seconds>(now - timeOfStart).count();
                        std::cout << "Explored " << numberOfExploredStates << " states in " << durationSinceStart << " seconds (currently " << statesPerSecond << " states per second)." << std::endl;
                        timeOfLastMessage = std::chrono::high_resolution_clock::now();
                        numberOfExploredStatesSinceLastMessage = 0;
                    }
                }
            }
            
            if (markovianStates) {
                // Since we now know the correct size, cut the bit vector to the correct length.
                markovianStates->resize(currentRowGroup, false);
            }

            // If the exploration order was not breadth-first, we need to fix the entries in the matrix according to
            // (reversed) mapping of row groups to indices.
            if (options.explorationOrder != ExplorationOrder::Bfs) {
                STORM_LOG_ASSERT(stateRemapping, "Unable to fix columns without mapping.");
                std::vector<uint_fast64_t> const& remapping = stateRemapping.get();
                
                // We need to fix the following entities:
                // (a) the transition matrix
                // (b) the initial states
                // (c) the hash map storing the mapping states -> ids
                
                // Fix (a).
                transitionMatrixBuilder.replaceColumns(remapping, 0);

                // Fix (b).
                std::vector<StateType> newInitialStateIndices(this->stateStorage.initialStateIndices.size());
                std::transform(this->stateStorage.initialStateIndices.begin(), this->stateStorage.initialStateIndices.end(), newInitialStateIndices.begin(), [&remapping] (StateType const& state) { return remapping[state]; } );
                std::sort(newInitialStateIndices.begin(), newInitialStateIndices.end());
                this->stateStorage.initialStateIndices = std::move(newInitialStateIndices);
                
                // Fix (c).
                this->stateStorage.stateToId.remap([&remapping] (StateType const& state) { return remapping[state]; } );
            }
        }
        
        template <typename ValueType, typename RewardModelType, typename StateType>
        storm::storage::sparse::ModelComponents<ValueType, RewardModelType> ExplicitModelBuilder<ValueType, RewardModelType, StateType>::buildModelComponents() {
            
            // Determine whether we have to combine different choices to one or whether this model can have more than
            // one choice per state.
            bool deterministicModel = generator->isDeterministicModel();
            
            // Prepare the component builders
            storm::storage::SparseMatrixBuilder<ValueType> transitionMatrixBuilder(0, 0, 0, false, !deterministicModel, 0);
            std::vector<RewardModelBuilder<typename RewardModelType::ValueType>> rewardModelBuilders;
            for (uint64_t i = 0; i < generator->getNumberOfRewardModels(); ++i) {
                rewardModelBuilders.emplace_back(generator->getRewardModelInformation(i));
            }
            ChoiceInformationBuilder choiceInformationBuilder;
            boost::optional<storm::storage::BitVector> markovianStates;
            
            buildMatrices(transitionMatrixBuilder, rewardModelBuilders, choiceInformationBuilder, markovianStates);
            
            // initialize the model components with the obtained information.
            storm::storage::sparse::ModelComponents<ValueType, RewardModelType> modelComponents(transitionMatrixBuilder.build(), buildStateLabeling(), std::unordered_map<std::string, RewardModelType>(), !generator->isDiscreteTimeModel(), std::move(markovianStates));
            modelComponents.eventVariables = std::move(eventVariables);
            modelComponents.eventToStatesMapping = std::move(eventToStatesMapping);
            modelComponents.stateToEventsMapping = std::move(stateToEventsMapping);
            modelComponents.eventNameToId = std::move(eventNameToId);

            // Now finalize all reward models.
            for (auto& rewardModelBuilder : rewardModelBuilders) {
                modelComponents.rewardModels.emplace(rewardModelBuilder.getName(), rewardModelBuilder.build(modelComponents.transitionMatrix.getRowCount(), modelComponents.transitionMatrix.getColumnCount(), modelComponents.transitionMatrix.getRowGroupCount()));
            }
            // Build the choice labeling
            modelComponents.choiceLabeling = choiceInformationBuilder.buildChoiceLabeling(modelComponents.transitionMatrix.getRowCount());
            
            // if requested, build the state valuations and choice origins
            if (generator->getOptions().isBuildStateValuationsSet()) {
                std::vector<storm::expressions::SimpleValuation> valuations(modelComponents.transitionMatrix.getRowGroupCount());
                for (auto const& bitVectorIndexPair : stateStorage.stateToId) {
                    valuations[bitVectorIndexPair.second] = generator->toValuation(bitVectorIndexPair.first);
                }
                modelComponents.stateValuations = storm::storage::sparse::StateValuations(std::move(valuations));
            }
            if (generator->getOptions().isBuildChoiceOriginsSet()) {
                auto originData = choiceInformationBuilder.buildDataOfChoiceOrigins(modelComponents.transitionMatrix.getRowCount());
                modelComponents.choiceOrigins = generator->generateChoiceOrigins(originData);
            }
            
            return modelComponents;
        }
        
        template <typename ValueType, typename RewardModelType, typename StateType>
        storm::models::sparse::StateLabeling ExplicitModelBuilder<ValueType, RewardModelType, StateType>::buildStateLabeling() {
            return generator->label(stateStorage.stateToId, stateStorage.initialStateIndices, stateStorage.deadlockStateIndices);
        }
        
        // Explicitly instantiate the class.
        template class ExplicitModelBuilder<double, storm::models::sparse::StandardRewardModel<double>, uint32_t>;

#ifdef STORM_HAVE_CARL
        template class ExplicitModelBuilder<RationalNumber, storm::models::sparse::StandardRewardModel<RationalNumber>, uint32_t>;
        template class ExplicitModelBuilder<RationalFunction, storm::models::sparse::StandardRewardModel<RationalFunction>, uint32_t>;
        template class ExplicitModelBuilder<double, storm::models::sparse::StandardRewardModel<storm::Interval>, uint32_t>;
#endif
    }
}
