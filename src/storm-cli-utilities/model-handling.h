#pragma once

#include "storm/api/storm.h"

#include "storm/utility/resources.h"
#include "storm/utility/file.h"
#include "storm/utility/storm-version.h"
#include "storm/utility/macros.h"

#include "storm/utility/initialize.h"
#include "storm/utility/Stopwatch.h"

#include <type_traits>


#include "storm/storage/SymbolicModelDescription.h"

#include "storm/models/ModelBase.h"

#include "storm/exceptions/OptionParserException.h"

#include "storm/modelchecker/results/SymbolicQualitativeCheckResult.h"

#include "storm/models/sparse/StandardRewardModel.h"
#include "storm/models/symbolic/StandardRewardModel.h"

#include "storm/settings/SettingsManager.h"
#include "storm/settings/modules/ResourceSettings.h"
#include "storm/settings/modules/JitBuilderSettings.h"
#include "storm/settings/modules/DebugSettings.h"
#include "storm/settings/modules/IOSettings.h"
#include "storm/settings/modules/CoreSettings.h"
#include "storm/settings/modules/ResourceSettings.h"
#include "storm/settings/modules/JaniExportSettings.h"

#include "storm/utility/Stopwatch.h"

namespace storm {
    namespace cli {


        struct SymbolicInput {
            // The symbolic model description.
            boost::optional<storm::storage::SymbolicModelDescription> model;

            // The properties to check.
            std::vector<storm::jani::Property> properties;
        };

        void parseSymbolicModelDescription(storm::settings::modules::IOSettings const& ioSettings, SymbolicInput& input) {
            if (ioSettings.isPrismOrJaniInputSet()) {
                if (ioSettings.isPrismInputSet()) {
                    input.model = storm::api::parseProgram(ioSettings.getPrismInputFilename());
                } else {
                    auto janiInput = storm::api::parseJaniModel(ioSettings.getJaniInputFilename());
                    input.model = janiInput.first;
                    auto const& janiPropertyInput = janiInput.second;

                    if (ioSettings.isJaniPropertiesSet()) {
                        for (auto const& propName : ioSettings.getJaniProperties()) {
                            auto propertyIt = janiPropertyInput.find(propName);
                            STORM_LOG_THROW(propertyIt != janiPropertyInput.end(), storm::exceptions::InvalidArgumentException, "No JANI property with name '" << propName << "' is known.");
                            input.properties.emplace_back(propertyIt->second);
                        }
                    }
                }
            }
        }

        void parseProperties(storm::settings::modules::IOSettings const& ioSettings, SymbolicInput& input, boost::optional<std::set<std::string>> const& propertyFilter) {
            if (ioSettings.isPropertySet()) {
                std::vector<storm::jani::Property> newProperties;
                if (input.model) {
                    newProperties = storm::api::parsePropertiesForSymbolicModelDescription(ioSettings.getProperty(), input.model.get(), propertyFilter);
                } else {
                    newProperties = storm::api::parseProperties(ioSettings.getProperty(), propertyFilter);
                }

                input.properties.insert(input.properties.end(), newProperties.begin(), newProperties.end());
            }
        }

        SymbolicInput parseSymbolicInput() {
            auto ioSettings = storm::settings::getModule<storm::settings::modules::IOSettings>();

            // Parse the property filter, if any is given.
            boost::optional<std::set<std::string>> propertyFilter = storm::api::parsePropertyFilter(ioSettings.getPropertyFilter());

            SymbolicInput input;
            parseSymbolicModelDescription(ioSettings, input);
            parseProperties(ioSettings, input, propertyFilter);

            return input;
        }

        SymbolicInput preprocessSymbolicInput(SymbolicInput const& input) {
            auto ioSettings = storm::settings::getModule<storm::settings::modules::IOSettings>();
            auto coreSettings = storm::settings::getModule<storm::settings::modules::CoreSettings>();

            SymbolicInput output = input;

            // Substitute constant definitions in symbolic input.
            std::string constantDefinitionString = ioSettings.getConstantDefinitionString();
            std::map<storm::expressions::Variable, storm::expressions::Expression> constantDefinitions;
            if (output.model) {
                constantDefinitions = output.model.get().parseConstantDefinitions(constantDefinitionString);
                output.model = output.model.get().preprocess(constantDefinitions);
            }
            if (!output.properties.empty()) {
                output.properties = storm::api::substituteConstantsInProperties(output.properties, constantDefinitions);
            }

            // Check whether conversion for PRISM to JANI is requested or necessary.
            if (input.model && input.model.get().isPrismProgram()) {
                bool transformToJani = ioSettings.isPrismToJaniSet();
                bool transformToJaniForJit = coreSettings.getEngine() == storm::settings::modules::CoreSettings::Engine::Sparse && ioSettings.isJitSet();
                STORM_LOG_WARN_COND(transformToJani || !transformToJaniForJit, "The JIT-based model builder is only available for JANI models, automatically converting the PRISM input model.");
                transformToJani |= transformToJaniForJit;

                if (transformToJani) {
                    storm::prism::Program const& model = output.model.get().asPrismProgram();
                    auto modelAndRenaming = model.toJaniWithLabelRenaming(true);
                    output.model = modelAndRenaming.first;

                    if (!modelAndRenaming.second.empty()) {
                        std::map<std::string, std::string> const& labelRenaming = modelAndRenaming.second;
                        std::vector<storm::jani::Property> amendedProperties;
                        for (auto const& property : output.properties) {
                            amendedProperties.emplace_back(property.substituteLabels(labelRenaming));
                        }
                        output.properties = std::move(amendedProperties);
                    }
                }
            }

            return output;
        }

        void exportSymbolicInput(SymbolicInput const& input) {
            auto ioSettings = storm::settings::getModule<storm::settings::modules::IOSettings>();
            if (input.model && input.model.get().isJaniModel()) {
                storm::storage::SymbolicModelDescription const& model = input.model.get();
                if (ioSettings.isExportJaniDotSet()) {
                    storm::api::exportJaniModelAsDot(model.asJaniModel(), ioSettings.getExportJaniDotFilename());
                }

                if (model.isJaniModel() && storm::settings::getModule<storm::settings::modules::JaniExportSettings>().isJaniFileSet()) {
                    storm::api::exportJaniModel(model.asJaniModel(), input.properties, storm::settings::getModule<storm::settings::modules::JaniExportSettings>().getJaniFilename());
                }
            }
        }

        SymbolicInput parseAndPreprocessSymbolicInput() {
            SymbolicInput input = parseSymbolicInput();
            input = preprocessSymbolicInput(input);
            exportSymbolicInput(input);
            return input;
        }

        std::vector<std::shared_ptr<storm::logic::Formula const>> createFormulasToRespect(std::vector<storm::jani::Property> const& properties) {
            std::vector<std::shared_ptr<storm::logic::Formula const>> result = storm::api::extractFormulasFromProperties(properties);

            for (auto const& property : properties) {
                if (!property.getFilter().getStatesFormula()->isInitialFormula()) {
                    result.push_back(property.getFilter().getStatesFormula());
                }
            }

            return result;
        }

        template <storm::dd::DdType DdType, typename ValueType>
        std::shared_ptr<storm::models::ModelBase> buildModelDd(SymbolicInput const& input) {
            return storm::api::buildSymbolicModel<DdType, ValueType>(input.model.get(), createFormulasToRespect(input.properties), storm::settings::getModule<storm::settings::modules::IOSettings>().isBuildFullModelSet());
        }

        template <typename ValueType>
        std::shared_ptr<storm::models::ModelBase> buildModelSparse(SymbolicInput const& input, storm::settings::modules::IOSettings const& ioSettings) {
            auto counterexampleGeneratorSettings = storm::settings::getModule<storm::settings::modules::CounterexampleGeneratorSettings>();
            storm::builder::BuilderOptions options(createFormulasToRespect(input.properties));
            options.setBuildChoiceLabels(ioSettings.isBuildChoiceLabelsSet());
            options.setBuildStateValuations(ioSettings.isBuildStateValuationsSet());
            options.setBuildChoiceOrigins(counterexampleGeneratorSettings.isMinimalCommandSetGenerationSet());
            options.setBuildAllLabels(ioSettings.isBuildFullModelSet());
            options.setBuildAllRewardModels(ioSettings.isBuildFullModelSet());
            if (ioSettings.isBuildFullModelSet()) {
                options.clearTerminalStates();
            }
            return storm::api::buildSparseModel<ValueType>(input.model.get(), options, ioSettings.isJitSet(), storm::settings::getModule<storm::settings::modules::JitBuilderSettings>().isDoctorSet());
        }

        template <typename ValueType>
        std::shared_ptr<storm::models::ModelBase> buildModelExplicit(storm::settings::modules::IOSettings const& ioSettings) {
            std::shared_ptr<storm::models::ModelBase> result;
            if (ioSettings.isExplicitSet()) {
                result = storm::api::buildExplicitModel<ValueType>(ioSettings.getTransitionFilename(), ioSettings.getLabelingFilename(), ioSettings.isStateRewardsSet() ? boost::optional<std::string>(ioSettings.getStateRewardsFilename()) : boost::none, ioSettings.isTransitionRewardsSet() ? boost::optional<std::string>(ioSettings.getTransitionRewardsFilename()) : boost::none, ioSettings.isChoiceLabelingSet() ? boost::optional<std::string>(ioSettings.getChoiceLabelingFilename()) : boost::none);
            } else if (ioSettings.isExplicitDRNSet()) {
                result = storm::api::buildExplicitDRNModel<ValueType>(ioSettings.getExplicitDRNFilename());
            } else {
                STORM_LOG_THROW(ioSettings.isExplicitIMCASet(), storm::exceptions::InvalidSettingsException, "Unexpected explicit model input type.");
                result = storm::api::buildExplicitIMCAModel<ValueType>(ioSettings.getExplicitIMCAFilename());
            }
            return result;
        }

        template <storm::dd::DdType DdType, typename ValueType>
        std::shared_ptr<storm::models::ModelBase> buildModel(storm::settings::modules::CoreSettings::Engine const& engine, SymbolicInput const& input, storm::settings::modules::IOSettings const& ioSettings) {
            storm::utility::Stopwatch modelBuildingWatch(true);

            std::shared_ptr<storm::models::ModelBase> result;
            if (input.model) {
                if (engine == storm::settings::modules::CoreSettings::Engine::Dd || engine == storm::settings::modules::CoreSettings::Engine::Hybrid) {
                    result = buildModelDd<DdType, ValueType>(input);
                } else if (engine == storm::settings::modules::CoreSettings::Engine::Sparse) {
                    result = buildModelSparse<ValueType>(input, ioSettings);
                }
            } else if (ioSettings.isExplicitSet() || ioSettings.isExplicitDRNSet() || ioSettings.isExplicitIMCASet()) {
                STORM_LOG_THROW(engine == storm::settings::modules::CoreSettings::Engine::Sparse, storm::exceptions::InvalidSettingsException, "Can only use sparse engine with explicit input.");
                result = buildModelExplicit<ValueType>(ioSettings);
            }

            modelBuildingWatch.stop();
            if (result) {
                STORM_PRINT("Time for model construction: " << modelBuildingWatch << "." << std::endl << std::endl);
            }

            return result;
        }

        template <typename ValueType>
        std::shared_ptr<storm::models::sparse::Model<ValueType>> preprocessSparseMarkovAutomaton(std::shared_ptr<storm::models::sparse::MarkovAutomaton<ValueType>> const& model) {
            std::shared_ptr<storm::models::sparse::Model<ValueType>> result = model;
            model->close();
            if (model->hasOnlyTrivialNondeterminism()) {
                STORM_LOG_WARN_COND(false, "Non-deterministic choices in MA seem to be unnecessary. Consider using a CTMC instead.");
                // Activate again if transformation is correct
                //result = model->convertToCTMC();
            }
            return result;
        }

        template <typename ValueType>
        std::shared_ptr<storm::models::sparse::Model<ValueType>> preprocessSparseModelBisimulation(std::shared_ptr<storm::models::sparse::Model<ValueType>> const& model, SymbolicInput const& input, storm::settings::modules::BisimulationSettings const& bisimulationSettings) {
            storm::storage::BisimulationType bisimType = storm::storage::BisimulationType::Strong;
            if (bisimulationSettings.isWeakBisimulationSet()) {
                bisimType = storm::storage::BisimulationType::Weak;
            }

            STORM_LOG_INFO("Performing bisimulation minimization...");
            return storm::api::performBisimulationMinimization<ValueType>(model, createFormulasToRespect(input.properties), bisimType);
        }

        template <typename ValueType>
        std::pair<std::shared_ptr<storm::models::sparse::Model<ValueType>>, bool> preprocessSparseModel(std::shared_ptr<storm::models::sparse::Model<ValueType>> const& model, SymbolicInput const& input) {
        auto generalSettings = storm::settings::getModule<storm::settings::modules::GeneralSettings>();
        auto bisimulationSettings = storm::settings::getModule<storm::settings::modules::BisimulationSettings>();
        auto ioSettings = storm::settings::getModule<storm::settings::modules::IOSettings>();

        std::pair<std::shared_ptr<storm::models::sparse::Model<ValueType>>, bool> result = std::make_pair(model, false);

        if (result.first->isOfType(storm::models::ModelType::MarkovAutomaton)) {
        result.first = preprocessSparseMarkovAutomaton(result.first->template as<storm::models::sparse::MarkovAutomaton<ValueType>>());
        result.second = true;
        }

        if (generalSettings.isBisimulationSet()) {
        result.first = preprocessSparseModelBisimulation(result.first, input, bisimulationSettings);
        result.second = true;
        }

        return result;
        }

        template <typename ValueType>
        void exportSparseModel(std::shared_ptr<storm::models::sparse::Model<ValueType>> const& model, SymbolicInput const& input) {
            auto ioSettings = storm::settings::getModule<storm::settings::modules::IOSettings>();

            if (ioSettings.isExportExplicitSet()) {
                storm::api::exportSparseModelAsDrn(model, ioSettings.getExportExplicitFilename(), input.model ? input.model.get().getParameterNames() : std::vector<std::string>());
            }

            if (ioSettings.isExportDotSet()) {
                storm::api::exportSparseModelAsDot(model, ioSettings.getExportDotFilename());
            }
        }

        template <storm::dd::DdType DdType, typename ValueType>
        void exportDdModel(std::shared_ptr<storm::models::symbolic::Model<DdType, ValueType>> const& model, SymbolicInput const& input) {
            // Intentionally left empty.
        }

        template <storm::dd::DdType DdType, typename ValueType>
        void exportModel(std::shared_ptr<storm::models::ModelBase> const& model, SymbolicInput const& input) {
            if (model->isSparseModel()) {
                exportSparseModel<ValueType>(model->as<storm::models::sparse::Model<ValueType>>(), input);
            } else {
                exportDdModel<DdType, ValueType>(model->as<storm::models::symbolic::Model<DdType, ValueType>>(), input);
            }
        }

        template <storm::dd::DdType DdType, typename ValueType>
        std::shared_ptr<storm::models::Model<ValueType>> preprocessDdModelBisimulation(std::shared_ptr<storm::models::symbolic::Model<DdType, ValueType>> const& model, SymbolicInput const& input, storm::settings::modules::BisimulationSettings const& bisimulationSettings) {
            STORM_LOG_WARN_COND(!bisimulationSettings.isWeakBisimulationSet(), "Weak bisimulation is currently not supported on DDs. Falling back to strong bisimulation.");
            
            STORM_LOG_INFO("Performing bisimulation minimization...");
            return storm::api::performBisimulationMinimization<DdType, ValueType>(model, createFormulasToRespect(input.properties), storm::storage::BisimulationType::Strong, bisimulationSettings.getSignatureMode());
        }
        
        template <storm::dd::DdType DdType, typename ValueType>
        std::pair<std::shared_ptr<storm::models::ModelBase>, bool> preprocessDdModel(std::shared_ptr<storm::models::symbolic::Model<DdType, ValueType>> const& model, SymbolicInput const& input) {
            auto bisimulationSettings = storm::settings::getModule<storm::settings::modules::BisimulationSettings>();
            auto generalSettings = storm::settings::getModule<storm::settings::modules::GeneralSettings>();
            std::pair<std::shared_ptr<storm::models::Model<ValueType>>, bool> result = std::make_pair(model, false);
            
            if (generalSettings.isBisimulationSet()) {
                result.first = preprocessDdModelBisimulation(model, input, bisimulationSettings);
                result.second = true;
            }
            
            return result;
        }

        template <storm::dd::DdType DdType, typename ValueType>
        std::pair<std::shared_ptr<storm::models::ModelBase>, bool> preprocessModel(std::shared_ptr<storm::models::ModelBase> const& model, SymbolicInput const& input) {
            storm::utility::Stopwatch preprocessingWatch(true);

            std::pair<std::shared_ptr<storm::models::ModelBase>, bool> result = std::make_pair(model, false);
            if (model->isSparseModel()) {
                result = preprocessSparseModel<ValueType>(result.first->as<storm::models::sparse::Model<ValueType>>(), input);
            } else {
                STORM_LOG_ASSERT(model->isSymbolicModel(), "Unexpected model type.");
                result = preprocessDdModel<DdType, ValueType>(result.first->as<storm::models::symbolic::Model<DdType, ValueType>>(), input);
            }
            
            preprocessingWatch.stop();

            if (result.second) {
                STORM_PRINT(std::endl << "Time for model preprocessing: " << preprocessingWatch << "." << std::endl << std::endl);
            }
            return result;
        }

        void printComputingCounterexample(storm::jani::Property const& property) {
            STORM_PRINT("Computing counterexample for property " << *property.getRawFormula() << " ..." << std::endl);
        }

        void printCounterexample(std::shared_ptr<storm::counterexamples::Counterexample> const& counterexample, storm::utility::Stopwatch* watch = nullptr) {
            if (counterexample) {
                STORM_PRINT(*counterexample << std::endl);
                if (watch) {
                    STORM_PRINT("Time for computation: " << *watch << "." << std::endl);
                }
            } else {
                STORM_PRINT(" failed." << std::endl);
            }
        }

        template <typename ValueType>
        void generateCounterexamples(std::shared_ptr<storm::models::ModelBase> const& model, SymbolicInput const& input) {
            STORM_LOG_THROW(false, storm::exceptions::NotSupportedException, "Counterexample generation is not supported for this data-type.");
        }

        template <>
        void generateCounterexamples<double>(std::shared_ptr<storm::models::ModelBase> const& model, SymbolicInput const& input) {
            typedef double ValueType;

            STORM_LOG_THROW(model->isSparseModel(), storm::exceptions::NotSupportedException, "Counterexample generation is currently only supported for sparse models.");
            auto sparseModel = model->as<storm::models::sparse::Model<ValueType>>();

            STORM_LOG_THROW(sparseModel->isOfType(storm::models::ModelType::Mdp), storm::exceptions::NotSupportedException, "Counterexample is currently only supported for MDPs.");
            auto mdp = sparseModel->template as<storm::models::sparse::Mdp<ValueType>>();

            auto counterexampleSettings = storm::settings::getModule<storm::settings::modules::CounterexampleGeneratorSettings>();
            if (counterexampleSettings.isMinimalCommandSetGenerationSet()) {
                STORM_LOG_THROW(input.model && input.model.get().isPrismProgram(), storm::exceptions::NotSupportedException, "Minimal command set counterexamples are only supported for PRISM model input.");
                storm::prism::Program const& program = input.model.get().asPrismProgram();

                bool useMilp = counterexampleSettings.isUseMilpBasedMinimalCommandSetGenerationSet();
                for (auto const& property : input.properties) {
                    std::shared_ptr<storm::counterexamples::Counterexample> counterexample;
                    printComputingCounterexample(property);
                    storm::utility::Stopwatch watch(true);
                    if (useMilp) {
                        counterexample = storm::api::computePrismHighLevelCounterexampleMilp(program, mdp, property.getRawFormula());
                    } else {
                        counterexample = storm::api::computePrismHighLevelCounterexampleMaxSmt(program, mdp, property.getRawFormula());
                    }
                    watch.stop();
                    printCounterexample(counterexample, &watch);
                }
            } else {
                STORM_LOG_THROW(false, storm::exceptions::NotSupportedException, "The selected counterexample formalism is unsupported.");
            }
        }

        template<typename ValueType>
        void printFilteredResult(std::unique_ptr<storm::modelchecker::CheckResult> const& result, storm::modelchecker::FilterType ft) {
            if (result->isQuantitative()) {
                switch (ft) {
                    case storm::modelchecker::FilterType::VALUES:
                        STORM_PRINT(*result);
                        break;
                    case storm::modelchecker::FilterType::SUM:
                        STORM_PRINT(result->asQuantitativeCheckResult<ValueType>().sum());
                        break;
                    case storm::modelchecker::FilterType::AVG:
                        STORM_PRINT(result->asQuantitativeCheckResult<ValueType>().average());
                        break;
                    case storm::modelchecker::FilterType::MIN:
                        STORM_PRINT(result->asQuantitativeCheckResult<ValueType>().getMin());
                        break;
                    case storm::modelchecker::FilterType::MAX:
                        STORM_PRINT(result->asQuantitativeCheckResult<ValueType>().getMax());
                        break;
                    case storm::modelchecker::FilterType::ARGMIN:
                    case storm::modelchecker::FilterType::ARGMAX:
                        STORM_LOG_THROW(false, storm::exceptions::NotSupportedException, "Outputting states is not supported.");
                    case storm::modelchecker::FilterType::EXISTS:
                    case storm::modelchecker::FilterType::FORALL:
                    case storm::modelchecker::FilterType::COUNT:
                        STORM_LOG_THROW(false, storm::exceptions::InvalidArgumentException, "Filter type only defined for qualitative results.");
                }
            } else {
                switch (ft) {
                    case storm::modelchecker::FilterType::VALUES:
                        STORM_PRINT(*result << std::endl);
                        break;
                    case storm::modelchecker::FilterType::EXISTS:
                        STORM_PRINT(result->asQualitativeCheckResult().existsTrue());
                        break;
                    case storm::modelchecker::FilterType::FORALL:
                        STORM_PRINT(result->asQualitativeCheckResult().forallTrue());
                        break;
                    case storm::modelchecker::FilterType::COUNT:
                        STORM_PRINT(result->asQualitativeCheckResult().count());
                        break;
                    case storm::modelchecker::FilterType::ARGMIN:
                    case storm::modelchecker::FilterType::ARGMAX:
                        STORM_LOG_THROW(false, storm::exceptions::NotSupportedException, "Outputting states is not supported.");
                    case storm::modelchecker::FilterType::SUM:
                    case storm::modelchecker::FilterType::AVG:
                    case storm::modelchecker::FilterType::MIN:
                    case storm::modelchecker::FilterType::MAX:
                        STORM_LOG_THROW(false, storm::exceptions::InvalidArgumentException, "Filter type only defined for quantitative results.");
                }
            }
            STORM_PRINT(std::endl);
        }

        void printModelCheckingProperty(storm::jani::Property const& property) {
            STORM_PRINT(std::endl << "Model checking property " << *property.getRawFormula() << " ..." << std::endl);
        }

        template<typename ValueType>
        void printResult(std::unique_ptr<storm::modelchecker::CheckResult> const& result, storm::jani::Property const& property, storm::utility::Stopwatch* watch = nullptr) {
            if (result) {
                std::stringstream ss;
                ss << "'" << *property.getFilter().getStatesFormula() << "'";
                STORM_PRINT("Result (for " << (property.getFilter().getStatesFormula()->isInitialFormula() ? "initial" : ss.str()) << " states): ");
                printFilteredResult<ValueType>(result, property.getFilter().getFilterType());
                if (watch) {
                    STORM_PRINT("Time for model checking: " << *watch << "." << std::endl);
                }
            } else {
                STORM_PRINT(" failed, property is unsupported by selected engine/settings." << std::endl);
            }
        }

        struct PostprocessingIdentity {
            void operator()(std::unique_ptr<storm::modelchecker::CheckResult> const&) {
                // Intentionally left empty.
            }
        };

        template<typename ValueType>
        void verifyProperties(std::vector<storm::jani::Property> const& properties, std::function<std::unique_ptr<storm::modelchecker::CheckResult>(std::shared_ptr<storm::logic::Formula const> const& formula, std::shared_ptr<storm::logic::Formula const> const& states)> const& verificationCallback, std::function<void(std::unique_ptr<storm::modelchecker::CheckResult> const&)> const& postprocessingCallback = PostprocessingIdentity()) {
        for (auto const& property : properties) {
        printModelCheckingProperty(property);
        storm::utility::Stopwatch watch(true);
        std::unique_ptr<storm::modelchecker::CheckResult> result = verificationCallback(property.getRawFormula(), property.getFilter().getStatesFormula());
        watch.stop();
        postprocessingCallback(result);
        printResult<ValueType>(result, property, &watch);
        }
        }

        template <storm::dd::DdType DdType, typename ValueType>
        void verifyWithAbstractionRefinementEngine(SymbolicInput const& input) {
            STORM_LOG_ASSERT(input.model, "Expected symbolic model description.");
            verifyProperties<ValueType>(input.properties, [&input] (std::shared_ptr<storm::logic::Formula const> const& formula, std::shared_ptr<storm::logic::Formula const> const& states) {
                STORM_LOG_THROW(states->isInitialFormula(), storm::exceptions::NotSupportedException, "Abstraction-refinement can only filter initial states.");
                return storm::api::verifyWithAbstractionRefinementEngine<DdType, ValueType>(input.model.get(), storm::api::createTask<ValueType>(formula, true));
            });
        }

        template <typename ValueType>
        void verifyWithExplorationEngine(SymbolicInput const& input) {
            STORM_LOG_ASSERT(input.model, "Expected symbolic model description.");
            STORM_LOG_THROW((std::is_same<ValueType, double>::value), storm::exceptions::NotSupportedException, "Exploration does not support other data-types than floating points.");
            verifyProperties<ValueType>(input.properties, [&input] (std::shared_ptr<storm::logic::Formula const> const& formula, std::shared_ptr<storm::logic::Formula const> const& states) {
                STORM_LOG_THROW(states->isInitialFormula(), storm::exceptions::NotSupportedException, "Exploration can only filter initial states.");
                return storm::api::verifyWithExplorationEngine<ValueType>(input.model.get(), storm::api::createTask<ValueType>(formula, true));
            });
        }

        template <typename ValueType>
        void verifyWithSparseEngine(std::shared_ptr<storm::models::ModelBase> const& model, SymbolicInput const& input) {
            auto sparseModel = model->as<storm::models::sparse::Model<ValueType>>();
            verifyProperties<ValueType>(input.properties,
                                        [&sparseModel] (std::shared_ptr<storm::logic::Formula const> const& formula, std::shared_ptr<storm::logic::Formula const> const& states) {
                                            bool filterForInitialStates = states->isInitialFormula();
                                            auto task = storm::api::createTask<ValueType>(formula, filterForInitialStates);
                                            std::unique_ptr<storm::modelchecker::CheckResult> result = storm::api::verifyWithSparseEngine<ValueType>(sparseModel, task);

                                            std::unique_ptr<storm::modelchecker::CheckResult> filter;
                                            if (filterForInitialStates) {
                                                filter = std::make_unique<storm::modelchecker::ExplicitQualitativeCheckResult>(sparseModel->getInitialStates());
                                            } else {
                                                filter = storm::api::verifyWithSparseEngine<ValueType>(sparseModel, storm::api::createTask<ValueType>(states, false));
                                            }
                                            if (result && filter) {
                                                result->filter(filter->asQualitativeCheckResult());
                                            }
                                            return result;
                                        });
        }

        template <storm::dd::DdType DdType, typename ValueType>
        void verifyWithHybridEngine(std::shared_ptr<storm::models::ModelBase> const& model, SymbolicInput const& input) {
            verifyProperties<ValueType>(input.properties, [&model] (std::shared_ptr<storm::logic::Formula const> const& formula, std::shared_ptr<storm::logic::Formula const> const& states) {
                bool filterForInitialStates = states->isInitialFormula();
                auto task = storm::api::createTask<ValueType>(formula, filterForInitialStates);

                auto symbolicModel = model->as<storm::models::symbolic::Model<DdType, ValueType>>();
                std::unique_ptr<storm::modelchecker::CheckResult> result = storm::api::verifyWithHybridEngine<DdType, ValueType>(symbolicModel, task);

                std::unique_ptr<storm::modelchecker::CheckResult> filter;
                if (filterForInitialStates) {
                    filter = std::make_unique<storm::modelchecker::SymbolicQualitativeCheckResult<DdType>>(symbolicModel->getReachableStates(), symbolicModel->getInitialStates());
                } else {
                    filter = storm::api::verifyWithHybridEngine<DdType, ValueType>(symbolicModel, storm::api::createTask<ValueType>(states, false));
                }
                if (result && filter) {
                    result->filter(filter->asQualitativeCheckResult());
                }
                return result;
            });
        }

        template <storm::dd::DdType DdType, typename ValueType>
        void verifyWithDdEngine(std::shared_ptr<storm::models::ModelBase> const& model, SymbolicInput const& input) {
            verifyProperties<ValueType>(input.properties, [&model] (std::shared_ptr<storm::logic::Formula const> const& formula, std::shared_ptr<storm::logic::Formula const> const& states) {
                bool filterForInitialStates = states->isInitialFormula();
                auto task = storm::api::createTask<ValueType>(formula, filterForInitialStates);

                auto symbolicModel = model->as<storm::models::symbolic::Model<DdType, ValueType>>();
                std::unique_ptr<storm::modelchecker::CheckResult> result = storm::api::verifyWithDdEngine<DdType, ValueType>(model->as<storm::models::symbolic::Model<DdType, ValueType>>(), storm::api::createTask<ValueType>(formula, true));

                std::unique_ptr<storm::modelchecker::CheckResult> filter;
                if (filterForInitialStates) {
                    filter = std::make_unique<storm::modelchecker::SymbolicQualitativeCheckResult<DdType>>(symbolicModel->getReachableStates(), symbolicModel->getInitialStates());
                } else {
                    filter = storm::api::verifyWithDdEngine<DdType, ValueType>(symbolicModel, storm::api::createTask<ValueType>(states, false));
                }
                if (result && filter) {
                    result->filter(filter->asQualitativeCheckResult());
                }
                return result;
            });
        }

        template <storm::dd::DdType DdType, typename ValueType>
        typename std::enable_if<DdType != storm::dd::DdType::CUDD || std::is_same<ValueType, double>::value, void>::type verifySymbolicModel(std::shared_ptr<storm::models::ModelBase> const& model, SymbolicInput const& input, storm::settings::modules::CoreSettings const& coreSettings) {
            bool hybrid = coreSettings.getEngine() == storm::settings::modules::CoreSettings::Engine::Hybrid;
            if (hybrid) {
                verifyWithHybridEngine<DdType, ValueType>(model, input);
            } else {
                verifyWithDdEngine<DdType, ValueType>(model, input);
            }
        }

        template <storm::dd::DdType DdType, typename ValueType>
        typename std::enable_if<DdType == storm::dd::DdType::CUDD && !std::is_same<ValueType, double>::value, void>::type verifySymbolicModel(std::shared_ptr<storm::models::ModelBase> const& model, SymbolicInput const& input, storm::settings::modules::CoreSettings const& coreSettings) {
            STORM_LOG_THROW(false, storm::exceptions::NotSupportedException, "CUDD does not support the selected data-type.");
        }

        template <storm::dd::DdType DdType, typename ValueType>
        void verifyModel(std::shared_ptr<storm::models::ModelBase> const& model, SymbolicInput const& input, storm::settings::modules::CoreSettings const& coreSettings) {
            if (model->isSparseModel()) {
                verifyWithSparseEngine<ValueType>(model, input);
            } else {
                STORM_LOG_ASSERT(model->isSymbolicModel(), "Unexpected model type.");
                verifySymbolicModel<DdType, ValueType>(model, input, coreSettings);
            }
        }

        template <storm::dd::DdType DdType, typename ValueType>
        std::shared_ptr<storm::models::ModelBase> buildPreprocessExportModelWithValueTypeAndDdlib(SymbolicInput const& input, storm::settings::modules::CoreSettings::Engine engine) {
            auto ioSettings = storm::settings::getModule<storm::settings::modules::IOSettings>();
            std::shared_ptr<storm::models::ModelBase> model;
            if (!ioSettings.isNoBuildModelSet()) {
                model = buildModel<DdType, ValueType>(engine, input, ioSettings);
            }

            if (model) {
                model->printModelInformationToStream(std::cout);
            }

            STORM_LOG_THROW(model || input.properties.empty(), storm::exceptions::InvalidSettingsException, "No input model.");

            if (model) {
                auto preprocessingResult = preprocessModel<DdType, ValueType>(model, input);
                if (preprocessingResult.second) {
                    model = preprocessingResult.first;
                    model->printModelInformationToStream(std::cout);
                }
                exportModel<DdType, ValueType>(model, input);
            }
            return model;
        }

        template <storm::dd::DdType DdType, typename ValueType>
        void processInputWithValueTypeAndDdlib(SymbolicInput const& input) {
            auto coreSettings = storm::settings::getModule<storm::settings::modules::CoreSettings>();

            // For several engines, no model building step is performed, but the verification is started right away.
            storm::settings::modules::CoreSettings::Engine engine = coreSettings.getEngine();
            if (engine == storm::settings::modules::CoreSettings::Engine::AbstractionRefinement) {
                verifyWithAbstractionRefinementEngine<DdType, ValueType>(input);
            } else if (engine == storm::settings::modules::CoreSettings::Engine::Exploration) {
                verifyWithExplorationEngine<ValueType>(input);
            } else {
                std::shared_ptr<storm::models::ModelBase> model = buildPreprocessExportModelWithValueTypeAndDdlib<DdType, ValueType>(input, engine);

                if (model) {
                    if (coreSettings.isCounterexampleSet()) {
                        auto ioSettings = storm::settings::getModule<storm::settings::modules::IOSettings>();
                        generateCounterexamples<ValueType>(model, input);
                    } else {
                        auto ioSettings = storm::settings::getModule<storm::settings::modules::IOSettings>();
                        verifyModel<DdType, ValueType>(model, input, coreSettings);
                    }
                }
            }
        }

        template <typename ValueType>
        void processInputWithValueType(SymbolicInput const& input) {
            auto coreSettings = storm::settings::getModule<storm::settings::modules::CoreSettings>();
            auto generalSettings = storm::settings::getModule<storm::settings::modules::GeneralSettings>();

            if (coreSettings.getDdLibraryType() == storm::dd::DdType::CUDD && coreSettings.isDdLibraryTypeSetFromDefaultValue() && generalSettings.isExactSet()) {
                STORM_LOG_INFO("Switching to DD library sylvan to allow for rational arithmetic.");
                processInputWithValueTypeAndDdlib<storm::dd::DdType::Sylvan, ValueType>(input);
            } else if (coreSettings.getDdLibraryType() == storm::dd::DdType::CUDD) {
                processInputWithValueTypeAndDdlib<storm::dd::DdType::CUDD, ValueType>(input);
            } else {
                STORM_LOG_ASSERT(coreSettings.getDdLibraryType() == storm::dd::DdType::Sylvan, "Unknown DD library.");
                processInputWithValueTypeAndDdlib<storm::dd::DdType::Sylvan, ValueType>(input);
            }
        }

}
}
