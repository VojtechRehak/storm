#ifndef STORM_GENERATOR_PRISMNEXTSTATEGENERATOR_H_
#define STORM_GENERATOR_PRISMNEXTSTATEGENERATOR_H_

#include <boost/container/flat_set.hpp>

#include "storm/generator/NextStateGenerator.h"

#include "storm/storage/prism/Program.h"

namespace storm {
    namespace generator {
        
        template<typename ValueType, typename StateType = uint32_t>
        class PrismNextStateGenerator : public NextStateGenerator<ValueType, StateType> {
        public:
            typedef typename NextStateGenerator<ValueType, StateType>::StateToIdCallback StateToIdCallback;
            typedef boost::container::flat_set<uint_fast64_t> CommandSet;
            
            PrismNextStateGenerator(storm::prism::Program const& program, NextStateGeneratorOptions const& options = NextStateGeneratorOptions());

            virtual ModelType getModelType() const override;
            virtual bool isDeterministicModel() const override;
            virtual bool isDiscreteTimeModel() const override;
            virtual std::vector<StateType> getInitialStates(StateToIdCallback const& stateToIdCallback) override;

            virtual StateBehavior<ValueType, StateType> expand(StateToIdCallback const& stateToIdCallback) override;

            virtual std::size_t getNumberOfRewardModels() const override;
            virtual storm::builder::RewardModelInformation getRewardModelInformation(uint64_t const& index) const override;
            
            virtual storm::models::sparse::StateLabeling label(storm::storage::BitVectorHashMap<StateType> const& states, std::vector<StateType> const& initialStateIndices = {}, std::vector<StateType> const& deadlockStateIndices = {}) override;

            virtual std::shared_ptr<storm::storage::sparse::ChoiceOrigins> generateChoiceOrigins(std::vector<boost::any>& dataForChoiceOrigins) const override;

            virtual void mapEvents(std::vector<EventVariableInformation<ValueType>>& eventVariables, std::unordered_map<std::string, uint_fast64_t>& eventNamesToId) const override;


        private:
            void checkValid() const;

            /*!
             * A delegate constructor that is used to preprocess the program before the constructor of the superclass is
             * being called. The last argument is only present to distinguish the signature of this constructor from the
             * public one.
             */
            PrismNextStateGenerator(storm::prism::Program const& program, NextStateGeneratorOptions const& options, bool flag);
            
            /*!
             * Applies an update to the state currently loaded into the evaluator and applies the resulting values to
             * the given compressed state.
             * @params state The state to which to apply the new values.
             * @params update The update to apply.
             * @return The resulting state.
             */
            CompressedState applyUpdate(CompressedState const& state, storm::prism::Update const& update);
            
            /*!
             * Retrieves all commands that are labeled with the given label and enabled in the given state, grouped by
             * modules.
             *
             * This function will iterate over all modules and retrieve all commands that are labeled with the given
             * action and active (i.e. enabled) in the current state. The result is a list of lists of commands in which
             * the inner lists contain all commands of exactly one module. If a module does not have *any* (including
             * disabled) commands, there will not be a list of commands of that module in the result. If, however, the
             * module has a command with a relevant label, but no enabled one, nothing is returned to indicate that there
             * is no legal transition possible.
             *
             * @param The program in which to search for active commands.
             * @param state The current state.
             * @param actionIndex The index of the action label to select.
             * @return A list of lists of active commands or nothing.
             */
            boost::optional<std::vector<std::vector<std::reference_wrapper<storm::prism::Command const>>>> getActiveCommandsByActionIndex(uint_fast64_t const& actionIndex);
            
            /*!
             * Retrieves all unlabeled choices possible from the given state.
             *
             * @param state The state for which to retrieve the unlabeled choices.
             * @return The unlabeled choices of the state.
             */
            std::vector<Choice<ValueType>> getUnlabeledChoices(CompressedState const& state, StateToIdCallback stateToIdCallback);
            
            /*!
             * Retrieves all labeled choices possible from the given state.
             *
             * @param state The state for which to retrieve the unlabeled choices.
             * @return The labeled choices of the state.
             */
            std::vector<Choice<ValueType>> getLabeledChoices(CompressedState const& state, StateToIdCallback stateToIdCallback);

            // The program used for the generation of next states.
            storm::prism::Program program;
            
            // The reward models that need to be considered.
            std::vector<std::reference_wrapper<storm::prism::RewardModel const>> rewardModels;

            
            // A flag that stores whether at least one of the selected reward models has state-action rewards.
            bool hasStateActionRewards;
        };
    }
}

#endif /* STORM_GENERATOR_PRISMNEXTSTATEGENERATOR_H_ */
