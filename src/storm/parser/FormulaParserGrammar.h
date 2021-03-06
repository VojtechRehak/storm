#pragma once

#include <memory>
#include <fstream>

#include "storm/parser/SpiritErrorHandler.h"
#include "storm/exceptions/WrongFormatException.h"
#include "storm/storage/jani/Property.h"
#include "storm/logic/Formulas.h"
#include "storm/parser/ExpressionParser.h"

#include "storm/modelchecker/results/FilterType.h"

#include "storm/storage/expressions/ExpressionEvaluator.h"

namespace storm {
    namespace logic {
        class Formula;
    }
    
    namespace parser {
        
        class FormulaParserGrammar : public qi::grammar<Iterator, std::vector<storm::jani::Property>(), Skipper> {
        public:
            FormulaParserGrammar(std::shared_ptr<storm::expressions::ExpressionManager const> const& manager);
            FormulaParserGrammar(std::shared_ptr<storm::expressions::ExpressionManager> const& manager);
            
            FormulaParserGrammar(FormulaParserGrammar const& other) = default;
            FormulaParserGrammar& operator=(FormulaParserGrammar const& other) = default;
            
            /*!
             * Adds an identifier and the expression it is supposed to be replaced with. This can, for example be used
             * to substitute special identifiers in the formula by expressions.
             *
             * @param identifier The identifier that is supposed to be substituted.
             * @param expression The expression it is to be substituted with.
             */
            void addIdentifierExpression(std::string const& identifier, storm::expressions::Expression const& expression);
            
        private:
            void initialize();
            
            struct keywordsStruct : qi::symbols<char, uint_fast64_t> {
                keywordsStruct() {
                    add
                    ("true", 1)
                    ("false", 2)
                    ("min", 3)
                    ("max", 4)
                    ("F", 5)
                    ("G", 6)
                    ("X", 7)
                    ("multi", 8);
                }
            };
            
            // A parser used for recognizing the keywords.
            keywordsStruct keywords_;
            
            struct relationalOperatorStruct : qi::symbols<char, storm::logic::ComparisonType> {
                relationalOperatorStruct() {
                    add
                    (">=", storm::logic::ComparisonType::GreaterEqual)
                    (">", storm::logic::ComparisonType::Greater)
                    ("<=", storm::logic::ComparisonType::LessEqual)
                    ("<", storm::logic::ComparisonType::Less);
                }
            };
            
            // A parser used for recognizing the operators at the "relational" precedence level.
            relationalOperatorStruct relationalOperator_;
            
            struct binaryBooleanOperatorStruct : qi::symbols<char, storm::logic::BinaryBooleanStateFormula::OperatorType> {
                binaryBooleanOperatorStruct() {
                    add
                    ("&", storm::logic::BinaryBooleanStateFormula::OperatorType::And)
                    ("|", storm::logic::BinaryBooleanStateFormula::OperatorType::Or);
                }
            };
            
            // A parser used for recognizing the operators at the "binary" precedence level.
            binaryBooleanOperatorStruct binaryBooleanOperator_;
            
            struct unaryBooleanOperatorStruct : qi::symbols<char, storm::logic::UnaryBooleanStateFormula::OperatorType> {
                unaryBooleanOperatorStruct() {
                    add
                    ("!", storm::logic::UnaryBooleanStateFormula::OperatorType::Not);
                }
            };
            
            // A parser used for recognizing the operators at the "unary" precedence level.
            unaryBooleanOperatorStruct unaryBooleanOperator_;
            
            struct optimalityOperatorStruct : qi::symbols<char, storm::OptimizationDirection> {
                optimalityOperatorStruct() {
                    add
                    ("min", storm::OptimizationDirection::Minimize)
                    ("max", storm::OptimizationDirection::Maximize);
                }
            };
            
            // A parser used for recognizing the optimality operators.
            optimalityOperatorStruct optimalityOperator_;
            
            struct rewardMeasureTypeStruct : qi::symbols<char, storm::logic::RewardMeasureType> {
                rewardMeasureTypeStruct() {
                    add
                    ("exp", storm::logic::RewardMeasureType::Expectation)
                    ("var", storm::logic::RewardMeasureType::Variance);
                }
            };
            
            // A parser used for recognizing the reward measure types.
            rewardMeasureTypeStruct rewardMeasureType_;
            
            struct filterTypeStruct : qi::symbols<char, storm::modelchecker::FilterType> {
                filterTypeStruct() {
                    add
                    ("min", storm::modelchecker::FilterType::MIN)
                    ("max", storm::modelchecker::FilterType::MAX)
                    ("sum", storm::modelchecker::FilterType::SUM)
                    ("avg", storm::modelchecker::FilterType::AVG)
                    ("count", storm::modelchecker::FilterType::COUNT)
                    ("forall", storm::modelchecker::FilterType::FORALL)
                    ("exists", storm::modelchecker::FilterType::EXISTS)
                    ("argmin", storm::modelchecker::FilterType::ARGMIN)
                    ("argmax", storm::modelchecker::FilterType::ARGMAX)
                    ("values", storm::modelchecker::FilterType::VALUES);
                }
            };

            // A parser used for recognizing the filter type.
            filterTypeStruct filterType_;
            
            // The manager used to parse expressions.
            std::shared_ptr<storm::expressions::ExpressionManager const> constManager;
            std::shared_ptr<storm::expressions::ExpressionManager> manager;
            
            // Parser and manager used for recognizing expressions.
            storm::parser::ExpressionParser expressionParser;
            
            // A symbol table that is a mapping from identifiers that can be used in expressions to the expressions
            // they are to be replaced with.
            qi::symbols<char, storm::expressions::Expression> identifiers_;
            
            qi::rule<Iterator, std::vector<storm::jani::Property>(), Skipper> start;
            
            qi::rule<Iterator, qi::unused_type(), qi::locals<bool>, Skipper> constantDefinition;
            qi::rule<Iterator, std::string(), Skipper> identifier;
            qi::rule<Iterator, std::string(), Skipper> formulaName;
            
            qi::rule<Iterator, storm::logic::OperatorInformation(), qi::locals<boost::optional<storm::OptimizationDirection>, boost::optional<storm::logic::ComparisonType>, boost::optional<storm::expressions::Expression>>, Skipper> operatorInformation;
            qi::rule<Iterator, storm::logic::RewardMeasureType(), Skipper> rewardMeasureType;
            qi::rule<Iterator, std::shared_ptr<storm::logic::Formula const>(), Skipper> probabilityOperator;
            qi::rule<Iterator, std::shared_ptr<storm::logic::Formula const>(), Skipper> rewardOperator;
            qi::rule<Iterator, std::shared_ptr<storm::logic::Formula const>(), Skipper> timeOperator;
            qi::rule<Iterator, std::shared_ptr<storm::logic::Formula const>(), Skipper> longRunAverageOperator;
            
            qi::rule<Iterator, storm::jani::Property(), Skipper> filterProperty;
            qi::rule<Iterator, std::shared_ptr<storm::logic::Formula const>(), Skipper> simpleFormula;
            qi::rule<Iterator, std::shared_ptr<storm::logic::Formula const>(), Skipper> stateFormula;
            qi::rule<Iterator, std::shared_ptr<storm::logic::Formula const>(storm::logic::FormulaContext), Skipper> pathFormula;
            qi::rule<Iterator, std::shared_ptr<storm::logic::Formula const>(storm::logic::FormulaContext), Skipper> pathFormulaWithoutUntil;
            qi::rule<Iterator, std::shared_ptr<storm::logic::Formula const>(), Skipper> simplePathFormula;
            qi::rule<Iterator, std::shared_ptr<storm::logic::Formula const>(), Skipper> atomicStateFormula;
            qi::rule<Iterator, std::shared_ptr<storm::logic::Formula const>(), Skipper> atomicStateFormulaWithoutExpression;
            qi::rule<Iterator, std::shared_ptr<storm::logic::Formula const>(), Skipper> operatorFormula;
            qi::rule<Iterator, std::string(), Skipper> label;
            qi::rule<Iterator, std::string(), Skipper> rewardModelName;
            
            qi::rule<Iterator, std::shared_ptr<storm::logic::Formula const>(), Skipper> andStateFormula;
            qi::rule<Iterator, std::shared_ptr<storm::logic::Formula const>(), Skipper> orStateFormula;
            qi::rule<Iterator, std::shared_ptr<storm::logic::Formula const>(), Skipper> notStateFormula;
            qi::rule<Iterator, std::shared_ptr<storm::logic::Formula const>(), Skipper> labelFormula;
            qi::rule<Iterator, std::shared_ptr<storm::logic::Formula const>(), Skipper> expressionFormula;
            qi::rule<Iterator, std::shared_ptr<storm::logic::Formula const>(), qi::locals<bool>, Skipper> booleanLiteralFormula;
            
            qi::rule<Iterator, std::shared_ptr<storm::logic::Formula const>(storm::logic::FormulaContext), Skipper> conditionalFormula;
            qi::rule<Iterator, std::shared_ptr<storm::logic::Formula const>(storm::logic::FormulaContext), Skipper> eventuallyFormula;
            qi::rule<Iterator, std::shared_ptr<storm::logic::Formula const>(storm::logic::FormulaContext), Skipper> nextFormula;
            qi::rule<Iterator, std::shared_ptr<storm::logic::Formula const>(storm::logic::FormulaContext), Skipper> globallyFormula;
            qi::rule<Iterator, std::shared_ptr<storm::logic::Formula const>(storm::logic::FormulaContext), Skipper> untilFormula;
            
            qi::rule<Iterator, std::tuple<boost::optional<storm::logic::TimeBound>, boost::optional<storm::logic::TimeBound>, boost::optional<std::string>>(), qi::locals<bool, bool>, Skipper> timeBound;
            
            qi::rule<Iterator, std::shared_ptr<storm::logic::Formula const>(), Skipper> rewardPathFormula;
            qi::rule<Iterator, std::shared_ptr<storm::logic::Formula const>(), qi::locals<bool>, Skipper> cumulativeRewardFormula;
            qi::rule<Iterator, std::shared_ptr<storm::logic::Formula const>(), Skipper> totalRewardFormula;
            qi::rule<Iterator, std::shared_ptr<storm::logic::Formula const>(), Skipper> instantaneousRewardFormula;
            qi::rule<Iterator, std::shared_ptr<storm::logic::Formula const>(), Skipper> longRunAverageRewardFormula;
            
            qi::rule<Iterator, std::shared_ptr<storm::logic::Formula const>(), Skipper> multiObjectiveFormula;
            
            // Parser that is used to recognize doubles only (as opposed to Spirit's double_ parser).
            boost::spirit::qi::real_parser<double, boost::spirit::qi::strict_real_policies<double>> strict_double;

            bool areConstantDefinitionsAllowed() const;
            void addConstant(std::string const& name, bool integer);
            void addProperty(std::vector<storm::jani::Property>& properties, boost::optional<std::string> const& name, std::shared_ptr<storm::logic::Formula const> const& formula);

            std::tuple<boost::optional<storm::logic::TimeBound>, boost::optional<storm::logic::TimeBound>, boost::optional<std::string>> createTimeBoundFromInterval(storm::expressions::Expression const& lowerBound, storm::expressions::Expression const& upperBound, boost::optional<std::string> const& rewardName) const;
            std::tuple<boost::optional<storm::logic::TimeBound>, boost::optional<storm::logic::TimeBound>, boost::optional<std::string>> createTimeBoundFromSingleBound(storm::expressions::Expression const& bound, bool upperBound, bool strict, boost::optional<std::string> const& rewardName) const;

                // Methods that actually create the expression objects.
            std::shared_ptr<storm::logic::Formula const> createInstantaneousRewardFormula(storm::expressions::Expression const& timeBound) const;
            std::shared_ptr<storm::logic::Formula const> createCumulativeRewardFormula(storm::expressions::Expression const& timeBound, bool strict) const;
            std::shared_ptr<storm::logic::Formula const> createTotalRewardFormula() const;
            std::shared_ptr<storm::logic::Formula const> createLongRunAverageRewardFormula() const;
            std::shared_ptr<storm::logic::Formula const> createAtomicExpressionFormula(storm::expressions::Expression const& expression) const;
            std::shared_ptr<storm::logic::Formula const> createBooleanLiteralFormula(bool literal) const;
            std::shared_ptr<storm::logic::Formula const> createAtomicLabelFormula(std::string const& label) const;
            std::shared_ptr<storm::logic::Formula const> createEventuallyFormula(boost::optional<std::tuple<boost::optional<storm::logic::TimeBound>, boost::optional<storm::logic::TimeBound>, boost::optional<std::string>>> const& timeBound, storm::logic::FormulaContext context, std::shared_ptr<storm::logic::Formula const> const& subformula) const;
            std::shared_ptr<storm::logic::Formula const> createGloballyFormula(std::shared_ptr<storm::logic::Formula const> const& subformula) const;
            std::shared_ptr<storm::logic::Formula const> createNextFormula(std::shared_ptr<storm::logic::Formula const> const& subformula) const;
            std::shared_ptr<storm::logic::Formula const> createUntilFormula(std::shared_ptr<storm::logic::Formula const> const& leftSubformula, boost::optional<std::tuple<boost::optional<storm::logic::TimeBound>, boost::optional<storm::logic::TimeBound>, boost::optional<std::string>>> const& timeBound, std::shared_ptr<storm::logic::Formula const> const& rightSubformula);
            std::shared_ptr<storm::logic::Formula const> createConditionalFormula(std::shared_ptr<storm::logic::Formula const> const& leftSubformula, std::shared_ptr<storm::logic::Formula const> const& rightSubformula, storm::logic::FormulaContext context) const;
            storm::logic::OperatorInformation createOperatorInformation(boost::optional<storm::OptimizationDirection> const& optimizationDirection, boost::optional<storm::logic::ComparisonType> const& comparisonType, boost::optional<storm::expressions::Expression> const& threshold) const;
            std::shared_ptr<storm::logic::Formula const> createLongRunAverageOperatorFormula(storm::logic::OperatorInformation const& operatorInformation, std::shared_ptr<storm::logic::Formula const> const& subformula) const;
            std::shared_ptr<storm::logic::Formula const> createRewardOperatorFormula(boost::optional<storm::logic::RewardMeasureType> const& rewardMeasureType, boost::optional<std::string> const& rewardModelName, storm::logic::OperatorInformation const& operatorInformation, std::shared_ptr<storm::logic::Formula const> const& subformula) const;
            std::shared_ptr<storm::logic::Formula const> createTimeOperatorFormula(boost::optional<storm::logic::RewardMeasureType> const& rewardMeasureType, storm::logic::OperatorInformation const& operatorInformation, std::shared_ptr<storm::logic::Formula const> const& subformula) const;
            std::shared_ptr<storm::logic::Formula const> createProbabilityOperatorFormula(storm::logic::OperatorInformation const& operatorInformation, std::shared_ptr<storm::logic::Formula const> const& subformula);
            std::shared_ptr<storm::logic::Formula const> createBinaryBooleanStateFormula(std::shared_ptr<storm::logic::Formula const> const& leftSubformula, std::shared_ptr<storm::logic::Formula const> const& rightSubformula, storm::logic::BinaryBooleanStateFormula::OperatorType operatorType);
            std::shared_ptr<storm::logic::Formula const> createUnaryBooleanStateFormula(std::shared_ptr<storm::logic::Formula const> const& subformula, boost::optional<storm::logic::UnaryBooleanStateFormula::OperatorType> const& operatorType);
            std::shared_ptr<storm::logic::Formula const> createMultiObjectiveFormula(std::vector<std::shared_ptr<storm::logic::Formula const>> const& subformulas);
            
            storm::jani::Property createProperty(boost::optional<std::string> const& propertyName, storm::modelchecker::FilterType const& filterType, std::shared_ptr<storm::logic::Formula const> const& formula, std::shared_ptr<storm::logic::Formula const> const& states);
            storm::jani::Property createPropertyWithDefaultFilterTypeAndStates(boost::optional<std::string> const& propertyName, std::shared_ptr<storm::logic::Formula const> const& formula);
            
            // An error handler function.
            phoenix::function<SpiritErrorHandler> handler;
            
            uint64_t propertyCount;
        };

    }
}
