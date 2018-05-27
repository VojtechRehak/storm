#include "storm/storage/expressions/RationalLiteralExpression.h"
#include "storm/storage/expressions/ExpressionManager.h"
#include "storm/storage/expressions/ExpressionVisitor.h"
#include "storm/storage/expressions/EventDistributionExpression.h"
#include "storm/exceptions/InvalidAccessException.h"

#include "storm/utility/constants.h"

namespace storm {
    namespace expressions {

            const std::map<EventDistributionTypes, std::string> EventDistributionExpression::distributionTypeToString =
                {{EventDistributionTypes::Exp, "Exponential"}, {EventDistributionTypes::Weibull, "Weibull"},
                {EventDistributionTypes::Uniform, "Uniform"}, {EventDistributionTypes::Dirac, "Dirac"},
                {EventDistributionTypes::Erlang, "Erlang"}};     

            EventDistributionExpression::EventDistributionExpression(ExpressionManager const& manager, EventDistributionTypes type, std::shared_ptr<BaseExpression const> const& param1, std::shared_ptr<BaseExpression const> const& param2) : 
                BaseExpression(manager, manager.getEventDistributionType()), distributionType(type), param1(param1), param2(param2){
                // intentionally left empty
            }

            EventDistributionExpression::EventDistributionExpression(ExpressionManager const& manager, EventDistributionTypes type, std::shared_ptr<BaseExpression const> const& param1) : 
                BaseExpression(manager, manager.getEventDistributionType()), distributionType(type), param1(param1) {
                    // intentionally left empty
                }

             bool EventDistributionExpression::isLiteral() const {
                return param1->isLiteral() && param2->isLiteral();
            }
            void EventDistributionExpression::gatherVariables(std::set<storm::expressions::Variable>& variables) const {
                param1->gatherVariables(variables);
                if (param2){
                    param2->gatherVariables(variables);
                }
            }

            std::shared_ptr<BaseExpression const> EventDistributionExpression::simplify() const {
                auto p1 = param1->simplify();
                if (param2){
                    auto p2 = param2->simplify();
                    return std::shared_ptr<BaseExpression>(new EventDistributionExpression(getManager(), distributionType, p1, p2));
                }
                return std::shared_ptr<BaseExpression>(new EventDistributionExpression(getManager(), distributionType, p1));
            }
            boost::any EventDistributionExpression::accept(ExpressionVisitor& visitor, boost::any const& data) const {
                return visitor.visit(*this, data);
            }
            uint_fast64_t EventDistributionExpression::getArity() const {
                return param2 ? 2 : 1;
            }

            std::shared_ptr<BaseExpression const> EventDistributionExpression::EventDistributionExpression::getParam1() const{
                return param1;
            }
        
            std::shared_ptr<BaseExpression const> EventDistributionExpression::getParam2() const {
                STORM_LOG_THROW(static_cast<bool>(param2), storm::exceptions::InvalidAccessException, "Unable to access operand 2 in distribution expression of arity 1.");
                return param2;
            }

            std::shared_ptr<BaseExpression const> EventDistributionExpression::getOperand(uint_fast64_t operandIndex) const {
                if (operandIndex == 1) {
                    return getParam1();
                }

                if (operandIndex == 2) {
                    return getParam2();
                }
                STORM_LOG_THROW(false, storm::exceptions::InvalidAccessException, "Unable to access operand " << operandIndex << "in expression distribution expression");
            }

            EventDistributionTypes EventDistributionExpression::getDistributionType() const {
                return distributionType;
            }

            // Override base class method.
            void EventDistributionExpression::printToStream(std::ostream& stream) const {
                stream << distributionTypeToString.at(distributionType) << "(" << *param1;
                if (param2){
                    stream << ", " << *param2;
                }
                stream << ")";
            }
        }
    }
