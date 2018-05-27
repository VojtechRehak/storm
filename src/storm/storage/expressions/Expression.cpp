#include <map>
#include <unordered_map>

#include "storm/storage/expressions/Expression.h"
#include "storm/storage/expressions/ExpressionManager.h"
#include "storm/storage/expressions/SubstitutionVisitor.h"
#include "storm/storage/expressions/LinearityCheckVisitor.h"
#include "storm/storage/expressions/SyntacticalEqualityCheckVisitor.h"
#include "storm/storage/expressions/ChangeManagerVisitor.h"
#include "storm/storage/expressions/CheckIfThenElseGuardVisitor.h"
#include "storm/storage/expressions/Expressions.h"
#include "storm/exceptions/InvalidTypeException.h"
#include "storm/exceptions/InvalidArgumentException.h"
#include "storm/utility/macros.h"

namespace storm {
    namespace expressions {
        /*!
         * Checks whether the two expressions share the same expression manager.
         *
         * @param a The first expression.
         * @param b The second expression.
         * @return True iff the expressions share the same manager.
         */
        static void assertSameManager(BaseExpression const& a, BaseExpression const& b) {
            STORM_LOG_THROW(a.getManager() == b.getManager(), storm::exceptions::InvalidArgumentException, "Expressions are managed by different manager.");
        }
        
        Expression::Expression(std::shared_ptr<BaseExpression const> const& expressionPtr) : expressionPtr(expressionPtr) {
            // Intentionally left empty.
        }
        
        Expression::Expression(Variable const& variable) : expressionPtr(std::shared_ptr<BaseExpression>(new VariableExpression(variable))) {
            // Intentionally left empty.
        }
        
        Expression Expression::changeManager(ExpressionManager const& newExpressionManager) const {
            ChangeManagerVisitor visitor(newExpressionManager);
            return visitor.changeManager(*this);
        }
        
		Expression Expression::substitute(std::map<Variable, Expression> const& identifierToExpressionMap) const {
            return SubstitutionVisitor<std::map<Variable, Expression>>(identifierToExpressionMap).substitute(*this);
        }

		Expression Expression::substitute(std::unordered_map<Variable, Expression> const& identifierToExpressionMap) const {
			return SubstitutionVisitor<std::unordered_map<Variable, Expression>>(identifierToExpressionMap).substitute(*this);
		}

        bool Expression::evaluateAsBool(Valuation const* valuation) const {
            return this->getBaseExpression().evaluateAsBool(valuation);
        }
        
        int_fast64_t Expression::evaluateAsInt(Valuation const* valuation) const {
            return this->getBaseExpression().evaluateAsInt(valuation);
        }
        
        double Expression::evaluateAsDouble(Valuation const* valuation) const {
            return this->getBaseExpression().evaluateAsDouble(valuation);
        }
        
        storm::RationalNumber Expression::evaluateAsRational() const {
            return this->getBaseExpression().evaluateAsRational();
        }

        EventDistributionTypes Expression::getDistributionType() const {
            return this->getBaseExpression().getDistributionType();
        }

        Expression Expression::simplify() const {
            return Expression(this->getBaseExpression().simplify());
        }
        
        OperatorType Expression::getOperator() const {
            return this->getBaseExpression().getOperator();
        }
        
        bool Expression::isFunctionApplication() const {
            return this->getBaseExpression().isFunctionApplication();
        }
        
        uint_fast64_t Expression::getArity() const {
            return this->getBaseExpression().getArity();
        }
        
        Expression Expression::getOperand(uint_fast64_t operandIndex) const {
            return Expression(this->getBaseExpression().getOperand(operandIndex));
        }
        
        std::string const& Expression::getIdentifier() const {
            return this->getBaseExpression().getIdentifier();
        }
        
        bool Expression::containsVariables() const {
            return this->getBaseExpression().containsVariables();
        }
        
        bool Expression::isLiteral() const {
            return this->getBaseExpression().isLiteral();
        }
        
        bool Expression::isVariable() const {
            return this->getBaseExpression().isVariable();
        }
        
        bool Expression::isTrue() const {
            return this->getBaseExpression().isTrue();
        }
        
        bool Expression::isFalse() const {
            return this->getBaseExpression().isFalse();
        }
        
        bool Expression::areSame(storm::expressions::Expression const& other) const {
            return this->expressionPtr == other.expressionPtr;
        }

		std::set<storm::expressions::Variable> Expression::getVariables() const {
            std::set<storm::expressions::Variable> result;
			this->getBaseExpression().gatherVariables(result);
            return result;
		}
        
        bool Expression::containsVariable(std::set<storm::expressions::Variable> const& variables) const {
            std::set<storm::expressions::Variable> appearingVariables = this->getVariables();
            std::set<storm::expressions::Variable> intersection;
            std::set_intersection(variables.begin(), variables.end(), appearingVariables.begin(), appearingVariables.end(), std::inserter(intersection, intersection.begin()));
            return !intersection.empty();
        }

        bool Expression::containsVariableInITEGuard(std::set<storm::expressions::Variable> const& variables) const {
            CheckIfThenElseGuardVisitor visitor(variables);
            return visitor.check(*this);
        }

        
        bool Expression::isRelationalExpression() const {
            if (!this->isFunctionApplication()) {
                return false;
            }
            
            return this->getOperator() == OperatorType::Equal || this->getOperator() == OperatorType::NotEqual
            || this->getOperator() == OperatorType::Less || this->getOperator() == OperatorType::LessOrEqual
            || this->getOperator() == OperatorType::Greater || this->getOperator() == OperatorType::GreaterOrEqual;
        }
        
        bool Expression::isLinear() const {
            return LinearityCheckVisitor().check(*this);
        }
        
        BaseExpression const& Expression::getBaseExpression() const {
            return *this->expressionPtr;
        }
        
        std::shared_ptr<BaseExpression const> const& Expression::getBaseExpressionPointer() const {
            return this->expressionPtr;
        }
        
        ExpressionManager const& Expression::getManager() const {
            return this->getBaseExpression().getManager();
        }
        
        Type const& Expression::getType() const {
            return this->getBaseExpression().getType();
        }
        
        bool Expression::hasNumericalType() const {
            return this->getBaseExpression().hasNumericalType();
        }
        
        bool Expression::hasRationalType() const {
            return this->getBaseExpression().hasRationalType();
        }
        
        bool Expression::hasBooleanType() const {
            return this->getBaseExpression().hasBooleanType();
        }
        
        bool Expression::hasIntegerType() const {
            return this->getBaseExpression().hasIntegerType();
        }
        
        bool Expression::hasBitVectorType() const {
            return this->getBaseExpression().hasBitVectorType();
        }

        bool Expression::hasDistributionType() const {
            return this->getBaseExpression().hasEventDistributionType();
        }
        
        boost::any Expression::accept(ExpressionVisitor& visitor, boost::any const& data) const {
            return this->getBaseExpression().accept(visitor, data);
        }

        bool Expression::isInitialized() const {
            if(this->getBaseExpressionPointer()) {
                return true;
            }
            return false;
        }
        
        bool Expression::isSyntacticallyEqual(storm::expressions::Expression const& other) const {
            if (this->getBaseExpressionPointer() == other.getBaseExpressionPointer()) {
                return true;
            }
            SyntacticalEqualityCheckVisitor checker;
            return checker.isSyntaticallyEqual(*this, other);
        }

        std::string Expression::toString() const {
            std::stringstream stream;
            stream << *this;
            return stream.str();
        }
        
        std::ostream& operator<<(std::ostream& stream, Expression const& expression) {
            stream << expression.getBaseExpression();
            return stream;
        }
        
        Expression operator+(Expression const& first, Expression const& second) {
            assertSameManager(first.getBaseExpression(), second.getBaseExpression());
            return Expression(std::shared_ptr<BaseExpression>(new BinaryNumericalFunctionExpression(first.getManager(), first.getType().plusMinusTimes(second.getType()), first.getBaseExpressionPointer(), second.getBaseExpressionPointer(), BinaryNumericalFunctionExpression::OperatorType::Plus)));
        }
        
        Expression operator+(Expression const& first, int64_t second) {
            return first + Expression(std::shared_ptr<BaseExpression>(new IntegerLiteralExpression(first.getBaseExpression().getManager(), second)));
        }
        
        Expression operator+(int64_t first, Expression const& second) {
            return second + first;
        }
        
        Expression operator-(Expression const& first, Expression const& second) {
            assertSameManager(first.getBaseExpression(), second.getBaseExpression());
            return Expression(std::shared_ptr<BaseExpression>(new BinaryNumericalFunctionExpression(first.getBaseExpression().getManager(), first.getType().plusMinusTimes(second.getType()), first.getBaseExpressionPointer(), second.getBaseExpressionPointer(), BinaryNumericalFunctionExpression::OperatorType::Minus)));
        }
        
        Expression operator-(Expression const& first, int64_t second) {
            return first - Expression(std::shared_ptr<BaseExpression>(new IntegerLiteralExpression(first.getBaseExpression().getManager(), second)));
        }
        
        Expression operator-(int64_t first, Expression const& second) {
            return Expression(std::shared_ptr<BaseExpression>(new IntegerLiteralExpression(second.getBaseExpression().getManager(), first))) - second;        }
        
        Expression operator-(Expression const& first) {
            return Expression(std::shared_ptr<BaseExpression>(new UnaryNumericalFunctionExpression(first.getBaseExpression().getManager(), first.getType().minus(), first.getBaseExpressionPointer(), UnaryNumericalFunctionExpression::OperatorType::Minus)));
        }
        
        Expression operator*(Expression const& first, Expression const& second) {
            assertSameManager(first.getBaseExpression(), second.getBaseExpression());
            return Expression(std::shared_ptr<BaseExpression>(new BinaryNumericalFunctionExpression(first.getBaseExpression().getManager(), first.getType().plusMinusTimes(second.getType()), first.getBaseExpressionPointer(), second.getBaseExpressionPointer(), BinaryNumericalFunctionExpression::OperatorType::Times)));
        }
        
        Expression operator/(Expression const& first, Expression const& second) {
            assertSameManager(first.getBaseExpression(), second.getBaseExpression());
            return Expression(std::shared_ptr<BaseExpression>(new BinaryNumericalFunctionExpression(first.getBaseExpression().getManager(), first.getType().divide(second.getType()), first.getBaseExpressionPointer(), second.getBaseExpressionPointer(), BinaryNumericalFunctionExpression::OperatorType::Divide)));
        }
        
        Expression operator^(Expression const& first, Expression const& second) {
            assertSameManager(first.getBaseExpression(), second.getBaseExpression());
            return Expression(std::shared_ptr<BaseExpression>(new BinaryNumericalFunctionExpression(first.getBaseExpression().getManager(), first.getType().power(second.getType()), first.getBaseExpressionPointer(), second.getBaseExpressionPointer(), BinaryNumericalFunctionExpression::OperatorType::Power)));
        }
        
        Expression operator&&(Expression const& first, Expression const& second) {
            assertSameManager(first.getBaseExpression(), second.getBaseExpression());
            if (first.isTrue()) {
                STORM_LOG_THROW(second.hasBooleanType(), storm::exceptions::InvalidTypeException, "Operator requires boolean operands.");
                return second;
            }
            if (second.isTrue()) {
                STORM_LOG_THROW(first.hasBooleanType(), storm::exceptions::InvalidTypeException, "Operator requires boolean operands.");
                return first;
            }
            
            return Expression(std::shared_ptr<BaseExpression>(new BinaryBooleanFunctionExpression(first.getBaseExpression().getManager(), first.getType().logicalConnective(second.getType()), first.getBaseExpressionPointer(), second.getBaseExpressionPointer(), BinaryBooleanFunctionExpression::OperatorType::And)));
        }
        
        Expression operator||(Expression const& first, Expression const& second) {
            assertSameManager(first.getBaseExpression(), second.getBaseExpression());
            return Expression(std::shared_ptr<BaseExpression>(new BinaryBooleanFunctionExpression(first.getBaseExpression().getManager(), first.getType().logicalConnective(second.getType()), first.getBaseExpressionPointer(), second.getBaseExpressionPointer(), BinaryBooleanFunctionExpression::OperatorType::Or)));
        }
        
        Expression operator!(Expression const& first) {
            return Expression(std::shared_ptr<BaseExpression>(new UnaryBooleanFunctionExpression(first.getBaseExpression().getManager(), first.getType().logicalConnective(), first.getBaseExpressionPointer(), UnaryBooleanFunctionExpression::OperatorType::Not)));
        }
        
        Expression operator==(Expression const& first, Expression const& second) {
            assertSameManager(first.getBaseExpression(), second.getBaseExpression());
            return Expression(std::shared_ptr<BaseExpression>(new BinaryRelationExpression(first.getBaseExpression().getManager(), first.getType().numericalComparison(second.getType()), first.getBaseExpressionPointer(), second.getBaseExpressionPointer(), BinaryRelationExpression::RelationType::Equal)));
        }
        
        Expression operator!=(Expression const& first, Expression const& second) {
            assertSameManager(first.getBaseExpression(), second.getBaseExpression());
            if (first.hasNumericalType() && second.hasNumericalType()) {
                return Expression(std::shared_ptr<BaseExpression>(new BinaryRelationExpression(first.getBaseExpression().getManager(), first.getType().numericalComparison(second.getType()), first.getBaseExpressionPointer(), second.getBaseExpressionPointer(), BinaryRelationExpression::RelationType::NotEqual)));
            } else {
                return Expression(std::shared_ptr<BaseExpression>(new BinaryBooleanFunctionExpression(first.getBaseExpression().getManager(), first.getType().logicalConnective(second.getType()), first.getBaseExpressionPointer(), second.getBaseExpressionPointer(), BinaryBooleanFunctionExpression::OperatorType::Xor)));
            }
        }

        Expression operator>(Expression const& first, Expression const& second) {
            assertSameManager(first.getBaseExpression(), second.getBaseExpression());
            return Expression(std::shared_ptr<BaseExpression>(new BinaryRelationExpression(first.getBaseExpression().getManager(), first.getType().numericalComparison(second.getType()), first.getBaseExpressionPointer(), second.getBaseExpressionPointer(), BinaryRelationExpression::RelationType::Greater)));
        }
        
        Expression operator>=(Expression const& first, Expression const& second) {
            assertSameManager(first.getBaseExpression(), second.getBaseExpression());
            return Expression(std::shared_ptr<BaseExpression>(new BinaryRelationExpression(first.getBaseExpression().getManager(), first.getType().numericalComparison(second.getType()), first.getBaseExpressionPointer(), second.getBaseExpressionPointer(), BinaryRelationExpression::RelationType::GreaterOrEqual)));
        }
        
        Expression operator<(Expression const& first, Expression const& second) {
            assertSameManager(first.getBaseExpression(), second.getBaseExpression());
            return Expression(std::shared_ptr<BaseExpression>(new BinaryRelationExpression(first.getBaseExpression().getManager(), first.getType().numericalComparison(second.getType()), first.getBaseExpressionPointer(), second.getBaseExpressionPointer(), BinaryRelationExpression::RelationType::Less)));
        }
        
        Expression operator<=(Expression const& first, Expression const& second) {
            assertSameManager(first.getBaseExpression(), second.getBaseExpression());
            return Expression(std::shared_ptr<BaseExpression>(new BinaryRelationExpression(first.getBaseExpression().getManager(), first.getType().numericalComparison(second.getType()), first.getBaseExpressionPointer(), second.getBaseExpressionPointer(), BinaryRelationExpression::RelationType::LessOrEqual)));
        }
        
        Expression operator<(Expression const& first, int64_t second) {
            return first < Expression(std::shared_ptr<BaseExpression>(new IntegerLiteralExpression(first.getBaseExpression().getManager(), second)));
        }
        
        Expression operator>(Expression const& first, int64_t second) {
            return first > Expression(std::shared_ptr<BaseExpression>(new IntegerLiteralExpression(first.getBaseExpression().getManager(), second)));
        }
        
        Expression operator<=(Expression const& first, int64_t second) {
            return first <= Expression(std::shared_ptr<BaseExpression>(new IntegerLiteralExpression(first.getBaseExpression().getManager(), second)));
        }
        
        Expression operator>=(Expression const& first, int64_t second) {
            return first >= Expression(std::shared_ptr<BaseExpression>(new IntegerLiteralExpression(first.getBaseExpression().getManager(), second)));
        }

        Expression minimum(Expression const& first, Expression const& second) {
            assertSameManager(first.getBaseExpression(), second.getBaseExpression());
            return Expression(std::shared_ptr<BaseExpression>(new BinaryNumericalFunctionExpression(first.getBaseExpression().getManager(), first.getType().minimumMaximum(second.getType()), first.getBaseExpressionPointer(), second.getBaseExpressionPointer(), BinaryNumericalFunctionExpression::OperatorType::Min)));
        }
        
        Expression maximum(Expression const& first, Expression const& second) {
            assertSameManager(first.getBaseExpression(), second.getBaseExpression());
            return Expression(std::shared_ptr<BaseExpression>(new BinaryNumericalFunctionExpression(first.getBaseExpression().getManager(), first.getType().minimumMaximum(second.getType()), first.getBaseExpressionPointer(), second.getBaseExpressionPointer(), BinaryNumericalFunctionExpression::OperatorType::Max)));
        }
        Expression distribution(EventDistributionTypes type, Expression const& first, boost::optional<Expression> const& second) {
            if (second){
                assertSameManager(first.getBaseExpression(), second.get().getBaseExpression());
                return Expression(std::shared_ptr<BaseExpression>(new EventDistributionExpression(first.getBaseExpression().getManager(), type, first.getBaseExpressionPointer(), second.get().getBaseExpressionPointer())));
            }
            return Expression(std::shared_ptr<BaseExpression>(new EventDistributionExpression(first.getBaseExpression().getManager(), type, first.getBaseExpressionPointer())));
        }

        Expression distribution(EventDistributionTypes type, Expression const& first) {
            return Expression(std::shared_ptr<BaseExpression>(new EventDistributionExpression(first.getBaseExpression().getManager(), type, first.getBaseExpressionPointer())));
        }

        
        Expression ite(Expression const& condition, Expression const& thenExpression, Expression const& elseExpression) {
            assertSameManager(condition.getBaseExpression(), thenExpression.getBaseExpression());
            assertSameManager(thenExpression.getBaseExpression(), elseExpression.getBaseExpression());
            return Expression(std::shared_ptr<BaseExpression>(new IfThenElseExpression(condition.getBaseExpression().getManager(), condition.getType().ite(thenExpression.getType(), elseExpression.getType()), condition.getBaseExpressionPointer(), thenExpression.getBaseExpressionPointer(), elseExpression.getBaseExpressionPointer())));
        }
        
        Expression implies(Expression const& first, Expression const& second) {
            assertSameManager(first.getBaseExpression(), second.getBaseExpression());
            return Expression(std::shared_ptr<BaseExpression>(new BinaryBooleanFunctionExpression(first.getBaseExpression().getManager(), first.getType().logicalConnective(second.getType()), first.getBaseExpressionPointer(), second.getBaseExpressionPointer(), BinaryBooleanFunctionExpression::OperatorType::Implies)));
        }
        
        Expression iff(Expression const& first, Expression const& second) {
            return Expression(std::shared_ptr<BaseExpression>(new BinaryBooleanFunctionExpression(first.getBaseExpression().getManager(), first.getType().logicalConnective(second.getType()), first.getBaseExpressionPointer(), second.getBaseExpressionPointer(), BinaryBooleanFunctionExpression::OperatorType::Iff)));
        }

        Expression xclusiveor(Expression const& first, Expression const& second) {
            assertSameManager(first.getBaseExpression(), second.getBaseExpression());
            return Expression(std::shared_ptr<BaseExpression>(new BinaryBooleanFunctionExpression(first.getBaseExpression().getManager(), first.getType().logicalConnective(second.getType()), first.getBaseExpressionPointer(), second.getBaseExpressionPointer(), BinaryBooleanFunctionExpression::OperatorType::Xor)));
        }
        
        Expression floor(Expression const& first) {
            STORM_LOG_THROW(first.hasNumericalType(), storm::exceptions::InvalidTypeException, "Operator 'floor' requires numerical operand.");
            return Expression(std::shared_ptr<BaseExpression>(new UnaryNumericalFunctionExpression(first.getBaseExpression().getManager(), first.getType().floorCeil(), first.getBaseExpressionPointer(), UnaryNumericalFunctionExpression::OperatorType::Floor)));
        }
        
        Expression ceil(Expression const& first) {
            STORM_LOG_THROW(first.hasNumericalType(), storm::exceptions::InvalidTypeException, "Operator 'ceil' requires numerical operand.");
            return Expression(std::shared_ptr<BaseExpression>(new UnaryNumericalFunctionExpression(first.getBaseExpression().getManager(), first.getType().floorCeil(), first.getBaseExpressionPointer(), UnaryNumericalFunctionExpression::OperatorType::Ceil)));
        }

        Expression abs(Expression const& first) {
            STORM_LOG_THROW(first.hasNumericalType(), storm::exceptions::InvalidTypeException, "Abs is only defined for numerical operands");
            return ite(first < 0, -first, first);
        }

        Expression sign(Expression const& first) {
            STORM_LOG_THROW(first.hasNumericalType(), storm::exceptions::InvalidTypeException, "Sign is only defined for numerical operands");
            return ite(first > 0, first.getManager().integer(1), ite(first < 0, first.getManager().integer(0), first.getManager().integer(0)));
        }

        Expression truncate(Expression const& first) {
            STORM_LOG_THROW(first.hasNumericalType(), storm::exceptions::InvalidTypeException, "Truncate is only defined for numerical operands");
            return ite(first < 0, floor(first), ceil(first));
        }

        Expression disjunction(std::vector<storm::expressions::Expression> const& expressions) {
            return apply(expressions, [] (Expression const& e1, Expression const& e2) { return e1 || e2; });
        }

        Expression conjunction(std::vector<storm::expressions::Expression> const& expressions) {
            return apply(expressions, [] (Expression const& e1, Expression const& e2) { return e1 && e2; });
        }
        
        Expression sum(std::vector<storm::expressions::Expression> const& expressions) {
            return apply(expressions, [] (Expression const& e1, Expression const& e2) { return e1 + e2; });
        }
        
        Expression apply(std::vector<storm::expressions::Expression> const& expressions, std::function<Expression (Expression const&, Expression const&)> const& function) {
            STORM_LOG_THROW(!expressions.empty(), storm::exceptions::InvalidArgumentException, "Cannot build disjunction of empty expression list.");
            auto it = expressions.begin();
            auto ite = expressions.end();
            Expression result = *it;
            ++it;
            
            for (; it != ite; ++it) {
                result = function(result, *it);
            }
            
            return result;
        }



    }
}
